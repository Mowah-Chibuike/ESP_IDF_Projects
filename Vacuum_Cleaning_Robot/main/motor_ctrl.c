#include "motor_ctrl.h"

#define TAG     "MOTOR"

static mcpwm_timer_handle_t timer = NULL;
static mcpwm_cmpr_handle_t left_ctrl_grp = NULL;
static mcpwm_cmpr_handle_t right_ctrl_grp = NULL;


esp_err_t motors_init()
{
    esp_err_t err;

    err = mcpwm_timer_init(&timer);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Timer not started: %d", err);
        return (err);
    }

    mcpwm_timer_enable(timer);
    mcpwm_timer_start_stop(timer, MCPWM_TIMER_START_NO_STOP);

    int gpios[2] = {FRONT_LEFT_PWM, REAR_LEFT_PWM};
    mcpwm_out_grp_config_t left_cfg = {
        .timer = timer,
        .gpios = gpios,
        .num_gpios = 2,
        .group_id = 0
    };

    err = mcpwm_setup_output_grp(&left_cfg, &left_ctrl_grp);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Couldn't create the left control group: %d", err);
        return (err);
    }

    int right_gpios[2] = {FRONT_RIGHT_PWM, REAR_RIGHT_PWM};
    mcpwm_out_grp_config_t right_cfg = {
        .timer = timer,
        .gpios = right_gpios,
        .num_gpios = 2,
        .group_id = 0
    };

    err = mcpwm_setup_output_grp(&right_cfg, &right_ctrl_grp);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Couldn't create the right control group: %d", err);
        return (err);
    }

 

    gpio_config_t gpio_cfg = {
        .pin_bit_mask = OUTPUT_PIN_MASK,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };

    err = gpio_config(&gpio_cfg);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Couldn't setup the output pins: %d", err);
        return (err);
    }


    return (ESP_OK);
}

void drive_motor(int dir, uint8_t pinA, uint8_t pinB)
{
    if (dir > 0)
    {
        gpio_set_level(pinA, 1);
        gpio_set_level(pinB, 0);
    }
    else{
        gpio_set_level(pinA, 0);
        gpio_set_level(pinB, 1);
    }
}


void move_forward(uint8_t speed)
{
    // Adjust IN Pins
    drive_motor(1, FRONT_LEFT_A, FRONT_LEFT_B);
    drive_motor(1, REAR_LEFT_A, REAR_LEFT_B);
    drive_motor(1, FRONT_RIGHT_A, FRONT_RIGHT_B);
    drive_motor(1, REAR_RIGHT_A, REAR_RIGHT_B);

    // Drive PWM to PWM pins
    esp_err_t err = mcpwm_set_grp_duty_cycle(left_ctrl_grp, speed);
    if (err != ESP_OK)
        ESP_LOGE(TAG, "Moto is not moving o");
    mcpwm_set_grp_duty_cycle(right_ctrl_grp, speed);
}

void move_backward(uint8_t speed)
{
    // Adjust IN Pins
    drive_motor(-1, FRONT_LEFT_A, FRONT_LEFT_B);
    drive_motor(-1, REAR_LEFT_A, REAR_LEFT_B);
    drive_motor(-1, FRONT_RIGHT_A, FRONT_RIGHT_B);
    drive_motor(-1, REAR_RIGHT_A, REAR_RIGHT_B);

    // Drive PWM to PWM pins
    mcpwm_set_grp_duty_cycle(left_ctrl_grp, speed);
    mcpwm_set_grp_duty_cycle(right_ctrl_grp, speed);
}

void turn_left(uint8_t speed)
{
    // Adjust IN Pins
    drive_motor(-1, FRONT_LEFT_A, FRONT_LEFT_B);
    drive_motor(-1, REAR_LEFT_A, REAR_LEFT_B);
    drive_motor(1, FRONT_RIGHT_A, FRONT_RIGHT_B);
    drive_motor(1, REAR_RIGHT_A, REAR_RIGHT_B);

    // Drive PWM to PWM pins
    mcpwm_set_grp_duty_cycle(left_ctrl_grp, speed);
    mcpwm_set_grp_duty_cycle(right_ctrl_grp, speed);
}

void turn_right(uint8_t speed)
{
    // Adjust IN Pins
    drive_motor(1, FRONT_LEFT_A, FRONT_LEFT_B);
    drive_motor(1, REAR_LEFT_A, REAR_LEFT_B);
    drive_motor(-1, FRONT_RIGHT_A, FRONT_RIGHT_B);
    drive_motor(-1, REAR_RIGHT_A, REAR_RIGHT_B);

    // Drive PWM to PWM pins
    mcpwm_set_grp_duty_cycle(left_ctrl_grp, speed);
    mcpwm_set_grp_duty_cycle(right_ctrl_grp, speed);
}