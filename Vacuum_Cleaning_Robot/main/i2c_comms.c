#include "i2c_comms.h"

static const char *TAG = "I2C";
static i2c_master_bus_handle_t bus_handle    = NULL;
static i2c_master_dev_handle_t mpu_dev_handle = NULL;

esp_err_t i2c_init(void)
{
    i2c_master_bus_config_t bus_cfg = {
        .i2c_port             = I2C_NUM_0,
        .sda_io_num           = I2C_BUS_SDA,
        .scl_io_num           = I2C_BUS_SCK,
        .clk_source           = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt    = 7,
        .flags.enable_internal_pullup = 0,
    };

    esp_err_t err = i2c_new_master_bus(&bus_cfg, &bus_handle);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Bus creation failed: %s", esp_err_to_name(err));
        return err;
    }

    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .scl_speed_hz    = 400 * 1000,
        .device_address  = MPU6050_I2C_ADDR,
    };

    err = i2c_master_bus_add_device(bus_handle, &dev_cfg, &mpu_dev_handle);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Add device failed: %s", esp_err_to_name(err));
        i2c_del_master_bus(bus_handle);
        bus_handle = NULL;
        return err;
    }

    ESP_LOGI(TAG, "I2C init complete, MPU6050 at 0x%02X", MPU6050_I2C_ADDR);
    return ESP_OK;
}

esp_err_t i2c_write_to_mpu_reg(mpu6050_reg_t reg, const uint8_t data)
{
    if (mpu_dev_handle == NULL)
    {
        ESP_LOGE(TAG, "Device handle NULL — call i2c_init first");
        return ESP_ERR_INVALID_STATE;
    }

    uint8_t buff[2] = {(uint8_t)reg, data};
    esp_err_t err = i2c_master_transmit(mpu_dev_handle, buff, 2,
                                         pdMS_TO_TICKS(100));
    if (err != ESP_OK)
        ESP_LOGE(TAG, "Write to reg 0x%02X failed: %s",
                 reg, esp_err_to_name(err));
    return err;
}

esp_err_t i2c_read_mpu_reg(mpu6050_reg_t reg, uint8_t *data)
{
    if (mpu_dev_handle == NULL)
    {
        ESP_LOGE(TAG, "Device handle NULL — call i2c_init first");
        return ESP_ERR_INVALID_STATE;
    }
    if (data == NULL)
        return ESP_ERR_INVALID_ARG;

    uint8_t send_buff[1] = {(uint8_t)reg};
    return i2c_master_transmit_receive(mpu_dev_handle,
                                        send_buff, 1,
                                        data, 1,
                                        pdMS_TO_TICKS(100));
}

esp_err_t i2c_mpu_burst_read(mpu6050_reg_t reg, uint8_t *buf, size_t len)
{
    if (mpu_dev_handle == NULL)
    {
        ESP_LOGE(TAG, "Device handle NULL — call i2c_init first");
        return ESP_ERR_INVALID_STATE;
    }
    if (buf == NULL || len == 0)
        return ESP_ERR_INVALID_ARG;

    uint8_t send_buff[1] = {(uint8_t)reg};
    return i2c_master_transmit_receive(mpu_dev_handle,
                                        send_buff, 1,
                                        buf, len,
                                        pdMS_TO_TICKS(100));
}

esp_err_t i2c_free_dev(void)
{
    if (mpu_dev_handle == NULL)
        return ESP_OK;
    esp_err_t err = i2c_master_bus_rm_device(mpu_dev_handle);
    mpu_dev_handle = NULL;
    return err;
}

esp_err_t i2c_free_bus(void)
{
    if (bus_handle == NULL)
        return ESP_OK;
    esp_err_t err = i2c_del_master_bus(bus_handle);
    bus_handle = NULL;
    return err;
}