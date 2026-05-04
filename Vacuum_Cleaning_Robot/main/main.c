#include "motor_ctrl.h"

#include "esp_timer.h"


void mission_planner(void *args)
{
    motion_ctx_t mctx;

    motion_ctrl_init(&mctx);
    const esp_timer_create_args_t timer_args = {
        .callback = control_loop_cb,
        .arg      = &mctx,
        .name     = "ctrl_loop"
    };
    esp_timer_handle_t timer;
    ESP_ERROR_CHECK(esp_timer_create(&timer_args, &timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(timer, PID_LOOP_PERIOD_MS * 1000));

    // Move forward 500mm at speed 200 pulses/10ms
    motion_ctrl_start_move(&mctx, 1, 500.0f, 200.0f, xTaskGetCurrentTaskHandle());
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);    // block until done

    // Move forward another 300mm
    motion_ctrl_start_move(&mctx, 1, 300.0f, 150.0f, xTaskGetCurrentTaskHandle());
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

    // Robot has completed its mission
    ESP_LOGI("MAIN", "Mission complete");
    vTaskDelete(NULL);
}

void app_main()
{
    esp_err_t err;

    err = motors_init();
    if (err != ESP_OK)
    {
        ESP_LOGE("MAIN", "Couldn't initialize the motors");
        return;
    }

    err = xTaskCreate(mission_planner, "mission_planner", 5120, NULL, 1, NULL);
    if (err != pdPASS) {
        ESP_LOGE("MAIN", "Error setting up task");
        return;
    }

    while (1)
    {
        ESP_LOGI("MAIN", "I should be working");
        vTaskDelay(pdMS_TO_TICKS(500));
    }

}