#ifndef MPU_6050_H
#define MPU_6050_H

#include "esp_err.h"
#include "esp_log.h"

#include "mpu_6050_types.h"

esp_err_t mpu_verify(void);
esp_err_t mpu_init(void);
esp_err_t mpu_calibrate(void);
esp_err_t get_mpu_data(mpu_readings_t *output);

#endif /* MPU_6050_H */