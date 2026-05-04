#include "motor_ctrl.h"

#define TAG     "MOTOR"

mcpwm_cmpr_handle_t left_ctrl_grp = {0};
mcpwm_cmpr_handle_t right_ctrl_grp = {0};

pcnt_unit_handle_t left_encoder = NULL;
pcnt_unit_handle_t right_encoder = NULL;

l298n_motor_ctx_t front_left = {0};
l298n_motor_ctx_t rear_left = {0};
l298n_motor_ctx_t front_right = {0};
l298n_motor_ctx_t rear_right = {0};

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

void drive(int dir, l298n_motor_ctx_t *motor)
{
    if (dir > 0)
    {
        gpio_set_level(motor->gpio_ctrl.gpio_ctrl_a, 1);
        gpio_set_level(motor->gpio_ctrl.gpio_ctrl_b, 0);
    }
    else{
        gpio_set_level(motor->gpio_ctrl.gpio_ctrl_a, 0);
        gpio_set_level(motor->gpio_ctrl.gpio_ctrl_b, 1);
    }
}

void motor_set_brake(l298n_motor_ctx_t *motor)
{
    gpio_set_level(motor->gpio_ctrl.gpio_ctrl_a, 1);
    gpio_set_level(motor->gpio_ctrl.gpio_ctrl_b, 1);
    mcpwm_comparator_set_compare_value(motor->gpio_ctrl.ctrl_handle, 0);
}

void pid_motor_ctrl(l298n_motor_ctx_t *motor)
{
    // 1. Measure actual speed
    int current_count = 0;
    pcnt_unit_get_count(motor->pcnt_encoder, &current_count);
    int raw_counts = current_count - motor->last_count;
    int real_pulses   = abs(raw_counts);
    motor->last_count       = current_count;
    motor->report_pulses    = raw_counts;

    // 2. Compute error against target set by Layer 3
    float error     = motor->target_speed - (float)real_pulses;
    float new_speed = 0;

    // 3. PID compute
    pid_compute(motor->pid_ctrl, error, &new_speed);

    // 4. Clamp and apply
    if (new_speed < 0) new_speed = 0;
    if (new_speed > motor->duty_tick_max - 1) new_speed = motor->duty_tick_max - 1;

    mcpwm_comparator_set_compare_value(motor->gpio_ctrl.ctrl_handle, (uint32_t)new_speed);
}

void motion_ctrl_init(motion_ctx_t *mctx)
{
    mctx->front_left_motor       = &front_left;
    mctx->front_right_motor      = &front_right;
    mctx->rear_left_motor       = &rear_left;
    mctx->rear_right_motor      = &rear_right;
    mctx->state            = MOTION_STATE_IDLE;
    mctx->base_speed       = 0;
    mctx->target_counts    = 0;
    mctx->decel_threshold  = (int)(DECEL_THRESHOLD_MM * COUNTS_PER_MM);
    mctx->min_speed        = MIN_SPEED_PULSES;
    mctx->correction_gain  = STRAIGHT_CORRECTION;
    mctx->waiting_task     = NULL;
}

void motion_ctrl_start_move(motion_ctx_t *mctx,
                       int8_t        dir, 
                       float         distance_mm,
                       float         speed_pulses_per_10ms,
                       TaskHandle_t  calling_task)
{
    mctx->direction = dir;
    // Convert distance to encoder counts
    mctx->target_counts = (int32_t)(distance_mm * COUNTS_PER_MM);
    mctx->base_speed    = speed_pulses_per_10ms;
    mctx->waiting_task  = calling_task;

    // Snapshot encoder positions at move start
    int left_count  = 0;
    int right_count = 0;
    pcnt_unit_get_count(mctx->rear_left_motor->pcnt_encoder,  &left_count);
    pcnt_unit_get_count(mctx->rear_right_motor->pcnt_encoder, &right_count);
    mctx->left_start_count  = left_count;
    mctx->right_start_count = right_count;

    // Reset PIDs — critical, clears integral from previous move
    pid_reset_ctrl_block(mctx->front_left_motor->pid_ctrl);
    pid_reset_ctrl_block(mctx->front_right_motor->pid_ctrl);
    pid_reset_ctrl_block(mctx->rear_left_motor->pid_ctrl);
    pid_reset_ctrl_block(mctx->rear_right_motor->pid_ctrl);

    // Set direction
    drive(dir, mctx->front_left_motor);
    drive(dir, mctx->front_right_motor);
    drive(dir, mctx->rear_left_motor);
    drive(dir, mctx->rear_right_motor);

    // Set initial speed targets
    mctx->front_left_motor->target_speed  = speed_pulses_per_10ms;
    mctx->front_right_motor->target_speed = speed_pulses_per_10ms;
    mctx->rear_left_motor->target_speed  = speed_pulses_per_10ms;
    mctx->rear_right_motor->target_speed = speed_pulses_per_10ms;

    mctx->state = MOTION_STATE_MOVING;
}

void motion_ctrl_update(motion_ctx_t *mctx)
{
    if (mctx->state == MOTION_STATE_IDLE) return;

    // --- 1. Calculate distance traveled on each wheel ---
    int left_current  = 0;
    int right_current = 0;
    pcnt_unit_get_count(mctx->rear_left_motor->pcnt_encoder,  &left_current);
    pcnt_unit_get_count(mctx->rear_right_motor->pcnt_encoder, &right_current);

    int left_traveled  = abs(left_current  - mctx->left_start_count);
    int right_traveled = abs(right_current - mctx->right_start_count);
    int avg_traveled   = (left_traveled + right_traveled) / 2;
    int remaining      = mctx->target_counts - avg_traveled;

    // --- 2. Check completion ---
    if (remaining <= 0) {
        motor_set_brake(mctx->front_left_motor);
        motor_set_brake(mctx->front_right_motor);
        motor_set_brake(mctx->rear_left_motor);
        motor_set_brake(mctx->rear_right_motor);
        mctx->front_left_motor->target_speed  = 0;
        mctx->front_right_motor->target_speed = 0;
        mctx->rear_left_motor->target_speed  = 0;
        mctx->rear_right_motor->target_speed = 0;
        mctx->state = MOTION_STATE_IDLE;

        // Notify mission layer
        if (mctx->waiting_task != NULL) {
            xTaskNotifyGive(mctx->waiting_task);
            mctx->waiting_task = NULL;
        }
        return;
    }

    // --- 3. Deceleration ramp ---
    float speed = mctx->base_speed;

    if (mctx->state == MOTION_STATE_MOVING &&
        remaining < mctx->decel_threshold)
    {
        mctx->state = MOTION_STATE_DECELERATING;
    }

    if (mctx->state == MOTION_STATE_DECELERATING) {
        // Linear ramp from base_speed down to min_speed
        float ratio = (float)remaining / (float)mctx->decel_threshold;
        speed = mctx->min_speed + (mctx->base_speed - mctx->min_speed) * ratio;
        if (speed < mctx->min_speed) speed = mctx->min_speed;
    }

    // --- 4. Straight line correction ---
    // Positive drift means left is ahead of right → slow left, speed up right
    float drift = (float)(left_traveled - right_traveled);
    float left_speed  = speed - (drift * mctx->correction_gain * mctx->direction);
    float right_speed = speed + (drift * mctx->correction_gain * mctx->direction);

    // Clamp corrected speeds
    if (left_speed  < mctx->min_speed) left_speed  = mctx->min_speed;
    if (right_speed < mctx->min_speed) right_speed = mctx->min_speed;

    // --- 5. Push new targets down to Layer 2 ---
    mctx->front_left_motor->target_speed  = left_speed;
    mctx->front_right_motor->target_speed = right_speed;
    mctx->rear_left_motor->target_speed  = left_speed;
    mctx->rear_right_motor->target_speed = right_speed;
}

void control_loop_cb(void *args)
{
    motion_ctx_t *mctx = (motion_ctx_t *)args;

    // Layer 2 — PID on encoder-equipped motors only
    pid_motor_ctrl(mctx->rear_left_motor);
    pid_motor_ctrl(mctx->rear_right_motor);

    // Layer 3 — distance tracking and speed target updates
    motion_ctrl_update(mctx);
    ESP_LOGI(TAG, "Control");
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