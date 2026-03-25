#include "wifi_analyser.h"
#include "display_driver.h"

uint8_t targetBSSID[6];
uint8_t targetChannel;
int8_t rssiVal;

int8_t prevRSSIAvg = 0;
int8_t prevSNRAvg = 0;

analyser_state_t currentState = STATE_SCANNING;
output_message_t out;

void state_machine_task(void *pvParameters)
{
    esp_err_t ret;
    analyser_event_t event;
    uint64_t delay = 0;

    ESP_LOGI(TAG, "Initialization Complete.");
    analyser_event_t message = {
        .type = EVENT_SCAN_START
    };
    xQueueSendToBack(stateQueue, &message, pdMS_TO_TICKS(STATE_QUEUE_TIMEOUT_MS));

    esp_timer_create_args_t scan_timer_args = {
        .callback = &scan_timer_callback,
        .arg = NULL
    };
    esp_timer_handle_t scan_timer;
    esp_timer_create(&scan_timer_args, &scan_timer);

    bool lockedBefore = false;

    while (1)
    {
        if (xQueueReceive(stateQueue, &event, portMAX_DELAY))
        {
            switch (currentState)
            {   
                case STATE_SCANNING:
                    if (event.type == EVENT_SCAN_START)
                    {
                        memset(&out, 0, sizeof(out));

                        // out.state = STATE_SCANNING;
                        // xQueueSendToBack(outputQueue, &out, pdMS_TO_TICKS(OUT_QUEUE_TIMEOUT_MS));
                        perform_scan();
                        xQueueSendToBack(outputQueue, &out, pdMS_TO_TICKS(OUT_QUEUE_TIMEOUT_MS));
                    } else if (event.type == EVENT_SCAN_SUCCESS)
                    {
                        memcpy(targetBSSID, event.data.scanResult.bssid, 6);
                        targetChannel = event.data.scanResult.channel;
                        rssiVal = event.data.scanResult.rssi;

                        // out.state = STATE_LOCKED;
                        // xQueueSendtoBack(outputQueue, &out, pdMS_TO_TICKS(OUT_QUEUE_TIMEOUT_MS));
                        xQueueReset(outputQueue);
                        xQueueReset(stateQueue);
                        delay = 0;
                        currentState = STATE_LOCKED;
                        memset(&message, 0, sizeof(message));
                        message.type = EVENT_INIT_PROMIS_START;
                        xQueueSendToBack(stateQueue, &message, pdMS_TO_TICKS(STATE_QUEUE_TIMEOUT_MS));
                    } else if (event.type == EVENT_SCAN_FAILED) {
                        
                        delay += 2000;
                        // replace this with a one shot timer
                        esp_timer_start_once(scan_timer, delay * 1000);
                    }
                    break;
                
                case STATE_LOCKED:
                    if (event.type == EVENT_INIT_PROMIS_FAILED || event.type == EVENT_INIT_PROMIS_START)
                    {
                        wifi_promiscuous_filter_t wifi_filter;

                        wifi_filter.filter_mask = WIFI_PROMIS_FILTER_MASK_MGMT;
                        ret = esp_wifi_set_promiscuous_filter(&wifi_filter);
                        ret = esp_wifi_set_promiscuous_rx_cb(&packet_rx_cb);
                        
                        ret = esp_wifi_set_channel(targetChannel, WIFI_SECOND_CHAN_NONE);
  
                        ret = esp_wifi_set_promiscuous(true);
                        if (ret != ESP_OK)
                        {
                            analyser_event_t message;
                            message.type = EVENT_INIT_PROMIS_FAILED;
                            xQueueSendToBack(stateQueue, &message, pdMS_TO_TICKS(STATE_QUEUE_TIMEOUT_MS));
                            break;
                        }
                        analyser_event_t message;
                        message.type = EVENT_INIT_PROMIS_SUCCESS;
                        xQueueSendToBack(stateQueue, &message, pdMS_TO_TICKS(STATE_QUEUE_TIMEOUT_MS));
                        vTaskResume(watchDogTaskHandler);
                        vTaskResume(processingTaskHandler);
                        ESP_LOGI(TAG, "Promiscuous mode set successfully");
                        
                    }
                    else if (event.type == EVENT_SIGNAL_DEGRADED)
                    {
                        currentState = STATE_DEGRADED;
                    }
                    else if (event.type == EVENT_SIGNAL_LOST)
                    {
                        currentState = STATE_LOST;
                        analyser_event_t message = {
                            .type = EVENT_SIGNAL_LOST
                        };
                        xQueueSendToBack(stateQueue, &message, pdMS_TO_TICKS(STATE_QUEUE_TIMEOUT_MS));
                    }
                    break;
                
                case STATE_DEGRADED:
                    if (event.type == EVENT_SIGNAL_RECOVERED)
                    {
                        currentState = STATE_LOCKED;
                    }
                    else if (event.type == EVENT_SIGNAL_LOST)
                    {
                        currentState = STATE_LOST;
                        analyser_event_t message = {
                            .type = EVENT_SIGNAL_LOST
                        };
                        xQueueSendToBack(stateQueue, &message, pdMS_TO_TICKS(STATE_QUEUE_TIMEOUT_MS));
                    }
                    
                    
                    break;

                case STATE_LOST:
                    currentState = STATE_SCANNING;
                    vTaskSuspend(watchDogTaskHandler);
                    vTaskSuspend(processingTaskHandler);
                    esp_wifi_set_promiscuous(false);
                    prevRSSIAvg = 0;
                    prevSNRAvg = 0;
                    xQueueReset(outputQueue);
                    xQueueReset(stateQueue);
                    analyser_event_t message = {
                        .type = EVENT_SCAN_START
                    };
                    xQueueSendToBack(stateQueue, &message, pdMS_TO_TICKS(STATE_QUEUE_TIMEOUT_MS));
                    break;

                default:
                    break;
            }
        }
        
    }
}

void rssi_process_task(void *pvParameters)
{
    analyser_val_t raw;
    int8_t currentRSSIAvg = 0;
    int8_t currentSNRAvg = 0;
    
    float alpha = 0.2;

    while (1)
    {
        if (xQueueReceive(rawValQueue, &raw, portMAX_DELAY) == pdTRUE)
        {
            // ESP_LOGI(TAG, "SSID: %s | Raw RSSI: %d | Raw SNR: %d", TARGET_SSID, raw.rssi, raw.snr);
            output_message_t out =  {0};

            currentRSSIAvg = (raw.rssi * alpha) + ((1 - alpha) * prevRSSIAvg);
            currentSNRAvg = (raw.snr * alpha) + ((1 - alpha) * prevSNRAvg);

            // ESP_LOGI(TAG, "SSID: %s | RSSI: %d | SNR: %d", TARGET_SSID, raw.rssi, raw.snr);
            out.state = currentState;
            out.channel = targetChannel;
            out.RSSI = currentRSSIAvg;
            out.SNR = currentSNRAvg;
            out.quality = classify_snr(currentSNRAvg);

            xQueueSendToBack(outputQueue, &out, pdMS_TO_TICKS(100));

            prevRSSIAvg = currentRSSIAvg;
            prevSNRAvg = currentSNRAvg;
        }
    }
    
}

void output_task(void *pvParameters)
{
    output_message_t msg;

    // uint32_t lineCount    = 0;
    // uint32_t lastSummary  = 0;
    // #define SUMMARY_INTERVAL_MS 10000

    /*
     * If scanning... Show scanning and identified networks and use their corresponding rssi values to
     * indicate signal strength
     */

    /*
     * If state locked:
     * -> Show SSID and BSSID
     * -> Display RSSI values
     * -> Indicate Signal Strength
    */
    // 
    // 
    while (1)
    {
        if (xQueueReceive(outputQueue, &msg, portMAX_DELAY) == pdTRUE)
        {
            dd_clear();
            switch (msg.state)
            {
                case STATE_SCANNING:
                    render_scanning_mode(msg);
                    break;
                
                case STATE_LOCKED:
                    render_locked_mode(msg);
                    break;
                
                default:
                    break;
            }
            update_oled();
        }

    }
}

void watchdog_task(void *pvParameters)
{
    uint64_t now;

    while (1)
    {
        now = esp_timer_get_time();

        if (now - lastSeen >= (TIMEOUT_THRESHOLD_MS * 1000))
        {
            analyser_event_t message = {
                .type = EVENT_SIGNAL_LOST
            };
            xQueueSendToBack(stateQueue, &message, pdMS_TO_TICKS(STATE_QUEUE_TIMEOUT_MS));
        }
        else if (now - lastSeen >= (WARNING_THRESHOLD_MS * 1000))
        {
            analyser_event_t message = {
                .type = EVENT_SIGNAL_DEGRADED
            };
            xQueueSendToBack(stateQueue, &message, pdMS_TO_TICKS(STATE_QUEUE_TIMEOUT_MS));
        } 
        else if (now - lastSeen < (WARNING_THRESHOLD_MS * 1000) && currentState == STATE_DEGRADED)
        {
            analyser_event_t message = {
                .type = EVENT_SIGNAL_RECOVERED
            };
            xQueueSendToBack(stateQueue, &message, pdMS_TO_TICKS(STATE_QUEUE_TIMEOUT_MS));
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}