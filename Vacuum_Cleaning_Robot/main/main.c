#include <stdio.h>
#include <inttypes.h>
#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"

#define LEFT_ENCODER_A      GPIO_NUM_34
#define LEFT_ENCODER_B      GPIO_NUM_35
#define RIGHT_ENCODER_A     GPIO_NUM_36
#define RIGHT_ENCODER_B     GPIO_NUM_39

#define GPIO_INPUT_MASK     ((1ULL << LEFT_ENCODER_A) | (1ULL << LEFT_ENCODER_B) | (1ULL << RIGHT_ENCODER_A) | (1ULL << RIGHT_ENCODER_B))

#define TAG "MAIN"

static volatile int64_t left_encoder_count = 0;
static volatile int64_t right_encoder_count = 0;

static void IRAM_ATTR count_left_encoder_ticks(void* arg)
{
    if (gpio_get_level(LEFT_ENCODER_B))
        left_encoder_count--;
    else
        left_encoder_count++;
    
}

static void IRAM_ATTR count_right_encoder_ticks(void* arg)
{
    if (gpio_get_level(RIGHT_ENCODER_B))
        right_encoder_count++;
    else
        right_encoder_count--;
    
}

void app_main(void)
{
    esp_err_t err;
    gpio_config_t io_config = {
        .pin_bit_mask = GPIO_INPUT_MASK,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };

    err = gpio_config(&io_config);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "GPIO config - Invalid arguments passed");
        return;
    }

    err = gpio_set_intr_type(LEFT_ENCODER_A, GPIO_INTR_POSEDGE);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Couldn't set interrupt type on pin: %d", LEFT_ENCODER_A);
        return;
    }

    err = gpio_set_intr_type(RIGHT_ENCODER_A, GPIO_INTR_POSEDGE);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Couldn't set interrupt type on pin: %d", RIGHT_ENCODER_A);
        return;
    }

    err = gpio_intr_enable(LEFT_ENCODER_A);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "GPIO intr enable on pin: %d - Invalid arguments passed", LEFT_ENCODER_A);
        return;
    }

    err = gpio_intr_enable(RIGHT_ENCODER_A);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "GPIO intr enable on pin: %d - Invalid arguments passed", RIGHT_ENCODER_A);
        return;
    }

    err = gpio_install_isr_service(0);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Couldn't install isr service: %d", err);
        return;
    }

    err = gpio_isr_handler_add(LEFT_ENCODER_A, count_left_encoder_ticks, NULL);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Couldn't add isr handler for pin %d: %d", LEFT_ENCODER_A, err);
        return;
    }

    err = gpio_isr_handler_add(RIGHT_ENCODER_A, count_right_encoder_ticks, NULL);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Couldn't add isr handler for pin %d: %d", RIGHT_ENCODER_A, err);
        return;
    }

    while (1)
    {
        ESP_LOGI(TAG, "Left encoder count: %" PRId64, left_encoder_count);
        ESP_LOGI(TAG, "Right encoder count: %" PRId64, right_encoder_count);
        vTaskDelay(pdMS_TO_TICKS(500));
    }
   
}