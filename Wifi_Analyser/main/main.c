#include "wifi_analyser.h"
#include "display_driver.h"
#include <stdio.h>
#include "i2c_comms.h"

QueueHandle_t stateQueue = NULL;
QueueHandle_t rawValQueue = NULL;
QueueHandle_t outputQueue = NULL;
SemaphoreHandle_t watchdogMutex = NULL;

TaskHandle_t watchDogTaskHandler;
TaskHandle_t processingTaskHandler;

uint64_t lastSeen = 0;


void app_main(void)
{
    esp_err_t ret;

    ret = initialize_nvs();
    if (ret != ESP_OK)
        return;

    ret = initialize_wifi();
    if (ret != ESP_OK)
        return;
    
    stateQueue = xQueueCreate(10, sizeof(analyser_event_t));
    rawValQueue = xQueueCreate(10, sizeof(analyser_val_t));
    outputQueue = xQueueCreate(10, sizeof(output_message_t));
    if (stateQueue == NULL || rawValQueue == NULL || outputQueue == NULL)
    {
        ESP_LOGE(TAG, "Failed to create the queues");
        return;
    }

    watchdogMutex = xSemaphoreCreateMutex();
    if (watchdogMutex == NULL)
    {
        ESP_LOGE(TAG, "Failed to create the watchdog mutex");
        return;
    }

    i2c_init();
    ESP_ERROR_CHECK(i2c_send_cmd_to_dsp(SSD1306_DISPLAY_OFF, 0));
    
    ESP_ERROR_CHECK(i2c_send_cmd_to_dsp(SSD1306_RESUME_RAM_CONTENT, 0));
    ESP_ERROR_CHECK(i2c_send_cmd_to_dsp(SSD1306_DISPLAY_NORMAL, 0));
    ESP_ERROR_CHECK(i2c_send_cmd_to_dsp(SSD1306_SET_SEGMENT_REMAP, 0));
    ESP_ERROR_CHECK(i2c_send_cmd_to_dsp(SSD1306_SET_COM_SCAN_DIR,  0));
    ESP_ERROR_CHECK(i2c_send_cmd_to_dsp(SSD1306_SET_ADDRESSING_MODE, 1, 0x02));
   

    ESP_ERROR_CHECK(i2c_send_cmd_to_dsp(SSD1306_DISPLAY_ON, 0));

    dd_clear();
    update_oled();

    xTaskCreate(watchdog_task, "Watchdog Task", 1024, NULL, 4, &watchDogTaskHandler);
    vTaskSuspend(watchDogTaskHandler);
    xTaskCreate(state_machine_task, "State machine task", 4096, NULL, 3, NULL);
    xTaskCreate(rssi_process_task, "Processing task", 2048, NULL, 2, &processingTaskHandler);
    vTaskSuspend(processingTaskHandler);
    xTaskCreate(output_task, "Output Task", 2048, NULL, 1, NULL);
}
