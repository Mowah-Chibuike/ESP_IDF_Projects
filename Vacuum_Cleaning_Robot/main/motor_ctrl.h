#ifndef MOTOR_CTRL_H
#define MOTOR_CTRL_H

#include "mcpwm_ctrl.h"

#define FRONT_LEFT_A        
#define FRONT_LEFT_B
#define FRONT_LEFT_PWM

#define REAR_LEFT_A
#define REAR_LEFT_B
#define REAR_LEFT_PWM

#define FRONT_RIGHT_A
#define FRONT_RIGHT_B
#define FRONT_RIGHT_PWM

#define REAR_RIGHT_A
#define REAR_RIGHT_B
#define REAR_RIGHT_PWM

esp_err_t motors_init();
void move_forward(uint8_t speed);
void move_backward(uint8_t speed);
void turn_left(uint8_t speed);
void turn_right(uint8_t speed);


#endif /* MOTOR_CTRL_H */