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


esp_err_t mcpwm_timer_init(mcpwm_timer_handle_t *handle);
esp_err_t mcpwm_setup_output_grp(mcpwm_out_grp_config_t *cfg, mcpwm_cmpr_handle_t *ctrl_handle);
esp_err_t mcpwm_set_grp_duty_cycle(mcpwm_cmpr_handle_t grp_comparator, uint8_t percentage_speed);

#endif /* MCPWM_CTRL_H */