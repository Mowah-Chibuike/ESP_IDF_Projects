#ifndef MCPWM_CTRL_H
#define MCPWM_CTRL_H

#include "esp_err.h"
#include "esp_log.h"
#include "driver/mcpwm_prelude.h"   
#include "driver/mcpwm_timer.h"
#include "driver/gpio.h"

#define MCPWM_TIMER_RESOLUTION      10000000
#define MCPWM_PERIOD_TICKS          1000

typedef struct
{
    mcpwm_timer_handle_t timer;
    gpio_num_t *gpios;
    size_t num_gpios;
    uint8_t group_id;
} mcpwm_out_grp_config_t;

#endif /* MCPWM_CTRL_H */