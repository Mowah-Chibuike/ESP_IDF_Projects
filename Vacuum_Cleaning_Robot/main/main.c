#include <stdio.h>
#include "mpu_6050.h"
#include "i2c_comms.h"

#define TAG                 "MAIN"
#define READ_FAIL_THRESHOLD 5

void app_main(void)
{
    esp_err_t err;

    // issue 2: check i2c_init return value
    i2c_init();

    // issue 3: verify returns esp_err_t, not bool
    // esp_err_to_name(err)err = mpu_verify();
    // if (err != ESP_OK)
    // {
    //     ESP_LOGE(TAG, "MPU6050 not found: %s", esp_err_to_name(err));
    //     i2c_free_dev();
    //     i2c_free_bus();
    //     return;
    // }

    err = mpu_init();
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "MPU6050 init failed: %s", esp_err_to_name(err));
        i2c_free_dev();
        i2c_free_bus();
        return;
    }

    // issue 7: stabilisation delay before calibration
    vTaskDelay(pdMS_TO_TICKS(100));

    err = mpu_calibrate();
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "MPU6050 calibration failed: %s", esp_err_to_name(err));
        i2c_free_dev();
        i2c_free_bus();
        return;
    }

    // issue 5: declare once, zero-initialise
    mpu_readings_t data = {0};
    uint8_t consecutive_failures = 0;
    uint32_t log_counter = 0;

    // issue 1 note: acceptable for prototype — refactor to named task for robot firmware
    // issue 6 note: log every 5th sample (every 1 second at 200 ms interval) for prototype
    while (1)
    {
        err = get_mpu_data(&data);
        if (err != ESP_OK)
        {
            consecutive_failures++;
            ESP_LOGE(TAG, "Read failed (%d/%d): %s",
                     consecutive_failures,
                     READ_FAIL_THRESHOLD,
                     esp_err_to_name(err));

            // issue 4: consecutive failure recovery
            if (consecutive_failures >= READ_FAIL_THRESHOLD)
            {
                ESP_LOGE(TAG, "Sensor unresponsive — halting");
                i2c_free_dev();
                i2c_free_bus();
                return;
            }
            vTaskDelay(pdMS_TO_TICKS(10));
            continue;
        }

        consecutive_failures = 0;

        // log every 5th iteration to reduce UART load
        if (++log_counter % 5 == 0)
        {
            ESP_LOGI(TAG, "aX:%.3f aY:%.3f aZ:%.3f gX:%.3f gY:%.3f gZ:%.3f",
                     data.accelX, data.accelY, data.accelZ,
                     data.gyroX,  data.gyroY,  data.gyroZ);
        }

        vTaskDelay(pdMS_TO_TICKS(200));
    }
}