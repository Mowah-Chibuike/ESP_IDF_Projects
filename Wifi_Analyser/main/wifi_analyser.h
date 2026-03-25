#ifndef WIFI_ANALYSER_H
#define WIFI_ANALYSER_H

#include <stdint.h>
#include <string.h>
#include "esp_timer.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "freertos/semphr.h"



#define TAG "Wi-Fi Analyser"

#define TARGET_SSID "Timothy's A16"

#define STATE_QUEUE_TIMEOUT_MS    500      // 0.5s
#define OUT_QUEUE_TIMEOUT_MS      STATE_QUEUE_TIMEOUT_MS
#define WARNING_THRESHOLD_MS      5000     // 5s
#define TIMEOUT_THRESHOLD_MS      80000    // 30s

typedef enum
{
    STATE_INIT,
    STATE_SCANNING,
    STATE_LOCKED,
    STATE_DEGRADED,
    STATE_LOST
} analyser_state_t;

typedef enum {
    EVENT_INIT_DONE,
    EVENT_SCAN_START,
    EVENT_SCAN_SUCCESS,
    EVENT_SCAN_FAILED,
    EVENT_INIT_PROMIS_START,
    EVENT_INIT_PROMIS_SUCCESS,
    EVENT_INIT_PROMIS_FAILED,
    EVENT_SIGNAL_DEGRADED,
    EVENT_SIGNAL_LOST,
    EVENT_SIGNAL_RECOVERED
} analyser_event_type_t;

typedef enum {
    SIGNAL_EXCELLENT,   // SNR > 25 dB  — streaming, low latency fine
    SIGNAL_GOOD,        // SNR > 15 dB  — normal browsing fine
    SIGNAL_FAIR,        // SNR > 10 dB  — basic connectivity, some drops
    SIGNAL_POOR,        // SNR > 5  dB  — frequent drops, high latency
    SIGNAL_UNUSABLE     // SNR <= 5 dB  — effectively disconnected
} signal_quality_t;

typedef struct
{
    uint8_t  bssid[6];
    uint8_t  channel;
    int8_t   rssi;
    int8_t   snr;
    int8_t   noise_floor;
    signal_quality_t quality;  
} scanresult_t;


typedef struct
{
    analyser_event_type_t type;
    union
    {
        scanresult_t scanResult;

        struct {
            int8_t   rssi;
            uint32_t timestamp;
        } signalData;
        
    } data;
    
} analyser_event_t;

typedef struct {
    uint8_t  bssid[6];
    char     ssid[33];
    // uint8_t  channel;
    int8_t   rssi;
    uint16_t beaconInterval;
} beacon_info_t;

typedef struct
{
    int8_t      rssi;
    int8_t      snr;
} analyser_val_t;

typedef struct {
    analyser_state_t    state;

    int8_t              RSSI;
    int8_t              SNR;
    signal_quality_t    quality;
    uint8_t             channel;

    /* For Scanning State */
    uint8_t             networkCount;
    char                foundSSIDs[5][33];
    int8_t              foundRSSIs[5];
    bool                targetFound;

    // uint32_t        timestamp;
    // uint32_t        beaconCount;
    // uint32_t        missedBeacons;
} output_message_t;


extern TaskHandle_t watchDogTaskHandler;
extern TaskHandle_t processingTaskHandler;

extern QueueHandle_t stateQueue;
extern QueueHandle_t rawValQueue;
extern QueueHandle_t outputQueue;
extern SemaphoreHandle_t watchdogMutex;

extern uint8_t targetBSSID[6];
extern uint8_t targetChannel;
extern int8_t rssiVal;
extern uint64_t lastSeen;
extern output_message_t out;

/* Helper functions */
esp_err_t initialize_nvs(void);
esp_err_t initialize_wifi(void);
void scan_timer_callback(void *arg);
void perform_scan(void);
void packet_rx_cb(void *buf, wifi_promiscuous_pkt_type_t type);
signal_quality_t classify_snr(int8_t snr);
const char* quality_to_string(signal_quality_t quality);
void get_bssid_string(char *output, uint8_t bssid[6]);

/* Render functions */
void render_scanning_mode(output_message_t msg);
void render_locked_mode(output_message_t msg);

/* Tasks */
void state_machine_task(void *pvParameters);

void rssi_process_task(void *pvParameters);

void output_task(void *pvParameters);

void watchdog_task(void *pvParameters);

#endif /* WIFI_ANALYSER_H */