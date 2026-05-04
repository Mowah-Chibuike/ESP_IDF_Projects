#ifndef MOTOR_CTRL_H
#define MOTOR_CTRL_H

#include "mcpwm_ctrl.h"
#include <inttypes.h>
#include "driver/pulse_cnt.h"
#include "pid_ctrl.h"

#define FRONT_LEFT_A        33
#define FRONT_LEFT_B        25
#define FRONT_LEFT_PWM      32

#define REAR_LEFT_A         27
#define REAR_LEFT_B         14
#define REAR_LEFT_PWM       12

#define FRONT_RIGHT_A       19
#define FRONT_RIGHT_B       18
#define FRONT_RIGHT_PWM     23

#define REAR_RIGHT_A        13
#define REAR_RIGHT_B        15
#define REAR_RIGHT_PWM      4

#define OUTPUT_PIN_MASK     ((1ULL << FRONT_LEFT_A) | (1ULL << FRONT_LEFT_B) | (1ULL << REAR_LEFT_A) | (1ULL << REAR_LEFT_B) | (1ULL << FRONT_RIGHT_A) | (1ULL << FRONT_RIGHT_B )| (1ULL << REAR_RIGHT_A) | (1ULL << REAR_RIGHT_B))

#define LEFT_ENCODER_CHAN_A     34
#define LEFT_ENCODER_CHAN_B     35

#define RIGHT_ENCODER_CHAN_A    39
#define RIGHT_ENCODER_CHAN_B    36

#define ENCODER_PPR             275
#define WHEEL_CIRCUM            204.2f // mm
#define PULSES_PER_REV          (WHEEL_CIRCUM / ENCODER_PPR)

#define ENCODER_PCNT_HIGH_LIMIT   1000
#define ENCODER_PCNT_LOW_LIMIT    -1000

#define PID_KP                  1.0f
#define PID_KI                  0.0f
#define PID_KD                  0.0f
#define PID_LOOP_PERIOS_MS      10

typedef struct
{
    mcpwm_cmpr_handle_t ctrl_handle;
    uint8_t gpio_ctrl_a;
    uint8_t gpio_ctrl_b;
} gpio_ctrl_t;

typedef struct
{
    gpio_ctrl_t gpio_ctrl;

    pcnt_unit_handle_t pcnt_encoder;

    pid_ctrl_block_handle_t pid_ctrl;
    
    int32_t                 last_count;
    int32_t                 report_pulses; 
    float                   target_speed;
    uint32_t                duty_tick_max;
} l298n_motor_ctx_t;

typedef enum {
    MOTION_STATE_IDLE,
    MOTION_STATE_MOVING,
    MOTION_STATE_DECELERATING,
    MOTION_STATE_DONE,
} motion_state_t;

typedef struct {
    // Pointers to motors
    l298n_motor_ctx_t         *front_left_motor;
    l298n_motor_ctx_t         *front_right_motor;
    l298n_motor_ctx_t         *rear_left_motor;
    l298n_motor_ctx_t         *rear_right_motor;

    // Motion state machine
    motion_state_t      state;

    // Distance tracking
    int32_t             left_start_count;
    int32_t             right_start_count;
    int32_t             target_counts;       // in encoder counts

    // Speed
    float               base_speed;          // pulses/10ms
    float               min_speed;           // floor during deceleration

    // Deceleration
    int32_t             decel_threshold;     // counts before target to start slowing

    // Straight line correction
    float               correction_gain;

    // Completion signal to mission layer
    TaskHandle_t        waiting_task;        // task to notify on completion
} motion_ctx_t;

esp_err_t pcnt_encoder_init(uint8_t chan_a, uint8_t chan_b, pcnt_unit_handle_t *out_handle);
esp_err_t motors_init();
void move_forward(uint8_t speed);
void move_backward(uint8_t speed);
void turn_left(uint8_t speed);
void turn_right(uint8_t speed);


#endif /* MOTOR_CTRL_H */