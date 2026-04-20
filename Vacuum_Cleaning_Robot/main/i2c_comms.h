#ifndef I2C_COMMS_H
#define I2C_COMMS_H

#include <driver/i2c_master.h>
#include <esp_log.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define I2C_BUS_SDA         GPIO_NUM_21
#define I2C_BUS_SCK         GPIO_NUM_22

#define MPU6050_I2C_ADDR    0x68

#include "mpu_6050_types.h"

esp_err_t i2c_init(void);
esp_err_t i2c_write_to_mpu_reg(mpu6050_reg_t reg, const uint8_t data);
esp_err_t i2c_read_mpu_reg(mpu6050_reg_t reg, uint8_t *data);
esp_err_t i2c_mpu_burst_read(mpu6050_reg_t reg, uint8_t *buf, size_t len);
esp_err_t i2c_free_dev();
esp_err_t i2c_free_bus();

#endif /* I2C_COMMS_H */