#ifndef MOTOR_CTRL_H
#define MOTOR_CTRL_H

#include "mcpwm_ctrl.h"
#include <inttypes.h>

#define FRONT_LEFT_A        25
#define FRONT_LEFT_B        33
#define FRONT_LEFT_PWM      32

#define REAR_LEFT_A         14
#define REAR_LEFT_B         27
#define REAR_LEFT_PWM       13

#define FRONT_RIGHT_A       19
#define FRONT_RIGHT_B       18
#define FRONT_RIGHT_PWM     23

#define REAR_RIGHT_A        12
#define REAR_RIGHT_B        15
#define REAR_RIGHT_PWM      4

#define OUTPUT_PIN_MASK     ((1ULL << FRONT_LEFT_A) | (1ULL << FRONT_LEFT_B) | (1ULL << REAR_LEFT_A) | (1ULL << REAR_LEFT_B) | (1ULL << FRONT_RIGHT_A) | (1ULL << FRONT_RIGHT_B )| (1ULL << REAR_RIGHT_A) | (1ULL << REAR_RIGHT_B))

esp_err_t motors_init();
void move_forward(uint8_t speed);
void move_backward(uint8_t speed);
void turn_left(uint8_t speed);
void turn_right(uint8_t speed);


#endif /* MOTOR_CTRL_H */