#include "mcpwm_ctrl.h"

#define TAG "MCPWM_COMP"

esp_err_t mcpwm_timer_init(mcpwm_timer_handle_t *handle)
{
    mcpwm_timer_config_t timer_cfg = {
        .clk_src = MCPWM_TIMER_CLK_SRC_DEFAULT,
        .resolution_hz = MCPWM_TIMER_RESOLUTION,
        .count_mode = MCPWM_TIMER_COUNT_MODE_UP,
        .period_ticks = MCPWM_PERIOD_TICKS,
    };

    return (mcpwm_new_timer(&timer_cfg, handle));
}

esp_err_t mcpwm_setup_output_grp(mcpwm_out_grp_config_t *cfg, mcpwm_cmpr_handle_t *ctrl_handle)
{
    esp_err_t err;

    ESP_LOGI(TAG, "Setting up the output group with id: %u", cfg->group_id);

    mcpwm_operator_config_t op_cfg = {
        .group_id = cfg->group_id,
    };
    mcpwm_oper_handle_t operator = NULL;

    err = mcpwm_new_operator(&op_cfg, &operator);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Something went wrong while initializing the operator: (%u)", err);
        return (err);
    }

    ESP_LOGI(TAG, "Successfully created the operator");

    err = mcpwm_operator_connect_timer(operator, cfg->timer);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Something went wrong while connecting the operator to the timer: (%u)", err);
        mcpwm_del_operator(operator);
        return (err);
    }

    ESP_LOGI(TAG, "Successfully connected the timer to the operator");

    mcpwm_cmpr_handle_t comparator = NULL;
    mcpwm_comparator_config_t comp_config = {
        .flags.update_cmp_on_tez = true
    };
    err = mcpwm_new_comparator(operator, &comp_config, &comparator);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Something went wrong while creating the comparator: (%u)", err);
        mcpwm_del_operator(operator);
        return (err);
    }
    ESP_LOGI(TAG, "Successfully created the comparator");

    mcpwm_comparator_set_compare_value(comparator, 0);

    mcpwm_gen_handle_t generators[2] = {0};
    mcpwm_generator_config_t gen_config = {};

    for (int i = 0; i < 2; i++)
    {
        gen_config.gen_gpio_num = cfg->gpios[i];
        err = mcpwm_new_generator(operator, &gen_config, &generators[i]);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Couldn't create generator on GPIO pin %u", cfg->gpios[i]);
            mcpwm_del_operator(operator);
            return (err);
        }
        else
            ESP_LOGI(TAG, "Attached generator on GPIO pin %u", cfg->gpios[i]);
    }

    for (int i = 0; i < 2; i++)
    {
        err = mcpwm_generator_set_action_on_timer_event(generators[i],MCPWM_GEN_TIMER_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP, MCPWM_TIMER_EVENT_EMPTY, MCPWM_GEN_ACTION_HIGH));
        if (err != ESP_OK)
        {
            ESP_LOGE(TAG, "Error setting actions on [%d] timer events: %u", i, err);
            mcpwm_del_operator(operator);
            if (i == 1)
                mcpwm_del_generator(generators[0]);
            return (err);
        }

        err = mcpwm_generator_set_actions_on_compare_event(generators[i],MCPWM_GEN_COMPARE_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP, comparator, MCPWM_GEN_ACTION_LOW));

        if (err != ESP_OK)
        {
            ESP_LOGE(TAG, "Error setting actions on [%d] compare events: %u", i, err);
            mcpwm_del_operator(operator);
            if (i == 1)
                mcpwm_del_generator(generators[0]);
            return (err);
        }

        if (i == 1)
            break;
    }

    *ctrl_handle = comparator;


    return (ESP_OK);
}

esp_err_t mcpwm_set_grp_duty_cycle(mcpwm_cmpr_handle_t grp_comparator, uint8_t percentage_speed)
{
    if (grp_comparator == NULL)
    {
        ESP_LOGE(TAG, "Error setting duty cycle. Comparator not initialized");
        return (ESP_FAIL);
    }

    if (percentage_speed > 100)
        percentage_speed = 100;

    return (mcpwm_comparator_set_compare_value(grp_comparator, (percentage_speed * MCPWM_PERIOD_TICKS / 100)));
}