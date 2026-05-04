#include "motor_ctrl.h"

#define TAG     "MOTOR"

mcpwm_cmpr_handle_t left_ctrl_grp = {0};
mcpwm_cmpr_handle_t right_ctrl_grp = {0};

pcnt_unit_handle_t left_encoder = NULL;
pcnt_unit_handle_t right_encoder = NULL;

static l298n_motor_ctx_t front_left = {0};
static l298n_motor_ctx_t rear_left = {0};
static l298n_motor_ctx_t front_right = {0};
static l298n_motor_ctx_t rear_right = {0};

esp_err_t pcnt_encoder_init(uint8_t chan_a, uint8_t chan_b, pcnt_unit_handle_t *out_handle)
{
    esp_err_t err;

    pcnt_unit_config_t unit_config = {
        .high_limit = ENCODER_PCNT_HIGH_LIMIT,
        .low_limit = ENCODER_PCNT_LOW_LIMIT,
        .flags.accum_count = true, // enable counter accumulation
    };
    pcnt_unit_handle_t pcnt_unit = NULL;
    err = pcnt_new_unit(&unit_config, &pcnt_unit);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Could not create new pcnt unit: %d", err);
        return (err);
    }


    pcnt_glitch_filter_config_t filter_config = {
        .max_glitch_ns = 1000,
    };
    err = pcnt_unit_set_glitch_filter(pcnt_unit, &filter_config);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Could not set glitch filter: %d", err);
        return (err);
    }


    pcnt_chan_config_t chan_a_config = {
        .edge_gpio_num = chan_a,
        .level_gpio_num = chan_b,
    };
    pcnt_channel_handle_t pcnt_chan_a = NULL;
    err = pcnt_new_channel(pcnt_unit, &chan_a_config, &pcnt_chan_a);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Could not create new channel: %d", err);
        return (err);
    }


    pcnt_chan_config_t chan_b_config = {
        .edge_gpio_num = chan_b,
        .level_gpio_num = chan_a,
    };
    pcnt_channel_handle_t pcnt_chan_b = NULL;
    err = pcnt_new_channel(pcnt_unit, &chan_b_config, &pcnt_chan_b);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Could not create new channel: %d", err);
        return (err);
    }


    err = pcnt_channel_set_edge_action(pcnt_chan_a, PCNT_CHANNEL_EDGE_ACTION_DECREASE, PCNT_CHANNEL_EDGE_ACTION_INCREASE);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Could not set edge action on channel A: %d", err);
        return (err);
    }

    err = pcnt_channel_set_level_action(pcnt_chan_a, PCNT_CHANNEL_LEVEL_ACTION_KEEP, PCNT_CHANNEL_LEVEL_ACTION_INVERSE);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Could not set level action on channel A: %d", err);
        return (err);
    }

    err = pcnt_channel_set_edge_action(pcnt_chan_b, PCNT_CHANNEL_EDGE_ACTION_INCREASE, PCNT_CHANNEL_EDGE_ACTION_DECREASE);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Could not set edge action on channel B: %d", err);
        return (err);
    }
    
    err = pcnt_channel_set_level_action(pcnt_chan_b, PCNT_CHANNEL_LEVEL_ACTION_KEEP, PCNT_CHANNEL_LEVEL_ACTION_INVERSE);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Could not set level action on channel B: %d", err);
        return (err);
    }
    
    err = pcnt_unit_add_watch_point(pcnt_unit, ENCODER_PCNT_HIGH_LIMIT);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Could not add watch points: %d", err);
        return (err);
    }

    err = pcnt_unit_add_watch_point(pcnt_unit, ENCODER_PCNT_LOW_LIMIT);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Could not add watch points: %d", err);
        return (err);
    }

    err = pcnt_unit_enable(pcnt_unit);
    err = pcnt_unit_clear_count(pcnt_unit);
    err = pcnt_unit_start(pcnt_unit);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Could not start pcnt unit: %d", err);
        return (err);
    }

    *out_handle = pcnt_unit;

    return (ESP_OK);
}

esp_err_t motor_hardware_start(void)
{
    esp_err_t err;
    mcpwm_timer_handle_t timer = NULL;
    

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

    err = pcnt_encoder_init(LEFT_ENCODER_CHAN_A, LEFT_ENCODER_CHAN_B, &left_encoder);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to setup left encoder");
        return (err);
    }

    err = pcnt_encoder_init(RIGHT_ENCODER_CHAN_A, RIGHT_ENCODER_CHAN_B, &right_encoder);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to setup left encoder");
        return (err);
    }

    return (ESP_OK);
}

esp_err_t single_motor_init(gpio_ctrl_t gpio_ctrl, pcnt_unit_handle_t encoder, l298n_motor_ctx_t *out_ctx)
{
    esp_err_t err;

    out_ctx->gpio_ctrl.ctrl_handle = gpio_ctrl.ctrl_handle;
    out_ctx->gpio_ctrl.gpio_ctrl_a = gpio_ctrl.gpio_ctrl_a;
    out_ctx->gpio_ctrl.gpio_ctrl_b = gpio_ctrl.gpio_ctrl_b;

    out_ctx->pcnt_encoder = encoder;

    pid_ctrl_config_t pid_config = {
        .init_param = {
            .kp           = PID_KP,
            .ki           = PID_KI,
            .kd           = PID_KD,
            .cal_type     = PID_CAL_TYPE_INCREMENTAL,
            .max_output   = MCPWM_PERIOD_TICKS - 1,
            .min_output   = 0,
            .max_integral = 1000,
            .min_integral = -1000,
        }
    };
    err = pid_new_control_block(&pid_config, &out_ctx->pid_ctrl);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failure to setup pid ctrl for front left motor");
        return (err);
    }

    out_ctx->last_count    = 0;
    out_ctx->target_speed  = 0;
    out_ctx->report_pulses = 0;
    out_ctx->duty_tick_max = MCPWM_PERIOD_TICKS;

    gpio_set_level(gpio_ctrl.gpio_ctrl_a, 0);
    gpio_set_level(gpio_ctrl.gpio_ctrl_b, 0);

    return (ESP_OK);
}

esp_err_t motors_init()
{
    esp_err_t err;

    err = motor_hardware_start();
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to start required motor hardware");
        return (err);
    }

    gpio_ctrl_t gpio_ctrl_cfg = {
        .ctrl_handle = left_ctrl_grp,
        .gpio_ctrl_a = FRONT_LEFT_A,
        .gpio_ctrl_b = FRONT_LEFT_B,
    };
    err = single_motor_init(gpio_ctrl_cfg, NULL, &front_left);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to setup front left motor: %d", err);
        return (err);
    }

    gpio_ctrl_cfg.ctrl_handle = left_ctrl_grp;
    gpio_ctrl_cfg.gpio_ctrl_a = REAR_LEFT_A;
    gpio_ctrl_cfg.gpio_ctrl_b = REAR_LEFT_B;
    err = single_motor_init(gpio_ctrl_cfg, left_encoder, &rear_left);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to setup rear left motor: %d", err);
        return (err);
    }

    gpio_ctrl_cfg.ctrl_handle = right_ctrl_grp;
    gpio_ctrl_cfg.gpio_ctrl_a = FRONT_RIGHT_A;
    gpio_ctrl_cfg.gpio_ctrl_b = FRONT_RIGHT_B;
    err = single_motor_init(gpio_ctrl_cfg, NULL, &front_right);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to setup front right motor: %d", err);
        return (err);
    }

    gpio_ctrl_cfg.ctrl_handle = right_ctrl_grp;
    gpio_ctrl_cfg.gpio_ctrl_a = REAR_RIGHT_A;
    gpio_ctrl_cfg.gpio_ctrl_b = REAR_RIGHT_B;
    err = single_motor_init(gpio_ctrl_cfg, right_encoder, &rear_right);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to setup front left motor: %d", err);
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


// void move_forward(uint8_t speed)
// {
//     // Adjust IN Pins
//     drive_motor(1, FRONT_LEFT_A, FRONT_LEFT_B);
//     drive_motor(1, REAR_LEFT_A, REAR_LEFT_B);
//     drive_motor(1, FRONT_RIGHT_A, FRONT_RIGHT_B);
//     drive_motor(1, REAR_RIGHT_A, REAR_RIGHT_B);

//     // Drive PWM to PWM pins
//     mcpwm_set_grp_duty_cycle(left_ctrl_grp, speed);
//     mcpwm_set_grp_duty_cycle(right_ctrl_grp, speed);
// }

// void move_backward(uint8_t speed)
// {
//     // Adjust IN Pins
//     drive_motor(-1, FRONT_LEFT_A, FRONT_LEFT_B);
//     drive_motor(-1, REAR_LEFT_A, REAR_LEFT_B);
//     drive_motor(-1, FRONT_RIGHT_A, FRONT_RIGHT_B);
//     drive_motor(-1, REAR_RIGHT_A, REAR_RIGHT_B);

//     // Drive PWM to PWM pins
//     mcpwm_set_grp_duty_cycle(left_ctrl_grp, speed);
//     mcpwm_set_grp_duty_cycle(right_ctrl_grp, speed);
// }

// void turn_left(uint8_t speed)
// {
//     // Adjust IN Pins
//     drive_motor(-1, FRONT_LEFT_A, FRONT_LEFT_B);
//     drive_motor(-1, REAR_LEFT_A, REAR_LEFT_B);
//     drive_motor(1, FRONT_RIGHT_A, FRONT_RIGHT_B);
//     drive_motor(1, REAR_RIGHT_A, REAR_RIGHT_B);

//     // Drive PWM to PWM pins
//     mcpwm_set_grp_duty_cycle(left_ctrl_grp, speed);
//     mcpwm_set_grp_duty_cycle(right_ctrl_grp, speed);
// }

// void turn_right(uint8_t speed)
// {
//     // Adjust IN Pins
//     drive_motor(1, FRONT_LEFT_A, FRONT_LEFT_B);
//     drive_motor(1, REAR_LEFT_A, REAR_LEFT_B);
//     drive_motor(-1, FRONT_RIGHT_A, FRONT_RIGHT_B);
//     drive_motor(-1, REAR_RIGHT_A, REAR_RIGHT_B);

//     // Drive PWM to PWM pins
//     mcpwm_set_grp_duty_cycle(left_ctrl_grp, speed);
//     mcpwm_set_grp_duty_cycle(right_ctrl_grp, speed);
// }