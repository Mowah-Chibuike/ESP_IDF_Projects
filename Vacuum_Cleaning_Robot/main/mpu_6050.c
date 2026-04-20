#include "i2c_comms.h"
#include "mpu_6050.h"

#define TAG "MPU6050"

mpu_config_t config = {0};
mpu_raw_data_t mpu_bias = {0};

esp_err_t mpu_verify(void)
{
    esp_err_t err;
    uint8_t data = 0;

    err = i2c_read_mpu_reg(MPU_WHO_AM_I, &data);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "WHO_AM_I read failed");
        return err;
    }

    if (data != MPU6050_I2C_ADDR)
    {
        // ESP_LOGE(TAG, "WHO_AM_I mismatch: expected 0x68, got 0x%02X", data);
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "MPU6050 verified: WHO_AM_I = 0x%02X", data);
    return ESP_OK;
}

esp_err_t mpu_init(void)
{
    esp_err_t err;
    uint8_t verify = 0;

    // Step 1: reset device — clears all registers to default
    err = i2c_write_to_mpu_reg(MPU_PWR_MGMT_1, DEVICE_RESET);
    if (err != ESP_OK)
        return err;
    vTaskDelay(pdMS_TO_TICKS(100)); // wait for reset to complete

    // Step 2: wake from sleep, set stable clock source
    err = i2c_write_to_mpu_reg(MPU_PWR_MGMT_1, CLK_SEL_1);
    if (err != ESP_OK)
        return err;
    vTaskDelay(pdMS_TO_TICKS(50));

    // Step 3: configure DLPF — must be before SMPLRT_DIV
    // sets internal gyro rate to 1kHz, accel/gyro BW to 44/42 Hz
    err = i2c_write_to_mpu_reg(MPU_CONFIG_REG, EXT_SYNC_SET_0 | DLPF_CFG_3);
    if (err != ESP_OK)
        return err;
    err = i2c_read_mpu_reg(MPU_CONFIG_REG, &verify);
    if (err != ESP_OK || verify != (EXT_SYNC_SET_0 | DLPF_CFG_3))
    {
        ESP_LOGE(TAG, "CONFIG verify failed: got 0x%02X", verify);
        return ESP_FAIL;
    }

    // Step 4: set sample rate — 1000/(1+9) = 100 Hz
    err = i2c_write_to_mpu_reg(MPU_SET_SAMPLE_RATE, 9);
    if (err != ESP_OK)
        return err;
    err = i2c_read_mpu_reg(MPU_SET_SAMPLE_RATE, &verify);
    if (err != ESP_OK || verify != 9)
    {
        ESP_LOGE(TAG, "SMPLRT_DIV verify failed: got 0x%02X", verify);
        return ESP_FAIL;
    }

    // Step 5: gyro full scale ±500°/s
    err = i2c_write_to_mpu_reg(MPU_GYRO_CONFIG, GYRO_CONFIG_500);
    if (err != ESP_OK)
        return err;
    err = i2c_read_mpu_reg(MPU_GYRO_CONFIG, &verify);
    if (err != ESP_OK || verify != GYRO_CONFIG_500)
    {
        ESP_LOGE(TAG, "GYRO_CONFIG verify failed: got 0x%02X", verify);
        return ESP_FAIL;
    }
    config.gyro_sensitivity = 65.5f;

    // Step 6: accel full scale ±4g
    err = i2c_write_to_mpu_reg(MPU_ACCEL_CONFIG, ACCEL_CONFIG_4G);
    if (err != ESP_OK)
        return err;
    err = i2c_read_mpu_reg(MPU_ACCEL_CONFIG, &verify);
    if (err != ESP_OK || verify != ACCEL_CONFIG_4G)
    {
        ESP_LOGE(TAG, "ACCEL_CONFIG verify failed: got 0x%02X", verify);
        return ESP_FAIL;
    }
    config.accel_sensitivity = 8192.0f;

    ESP_LOGI(TAG, "MPU6050 init complete");
    return ESP_OK;
}

esp_err_t mpu_calibrate(void)
{
    esp_err_t err;
    uint8_t data[14];

    int32_t accelXSum = 0;
    int32_t accelYSum = 0;
    int32_t accelZSum = 0;
    int32_t gyroXSum = 0;
    int32_t gyroYSum = 0;
    int32_t gyroZSum = 0;

    memset(&mpu_bias, 0, sizeof(mpu_bias));

    ESP_LOGI(TAG, "Calibrating sensor, please stay still");
    for (int i = 0; i < 500; i++)
    {
        
        err = i2c_mpu_burst_read(MPU_ACCEL_XOUT_H, data, 14);

        if (err != ESP_OK)
            return (err);
        
        int16_t accelX = (int16_t)(data[0] << 8) | (data[1]);
        int16_t accelY = (int16_t)(data[2] << 8) | (data[3]);
        int16_t accelZ = (int16_t)(data[4] << 8) | (data[5]);
        
        int16_t gyroX = (int16_t)(data[8] << 8) | (data[9]);
        int16_t gyroY = (int16_t)(data[10] << 8) | (data[11]);
        int16_t gyroZ = (int16_t)(data[12] << 8) | (data[13]);

        accelXSum += accelX;
        accelYSum += accelY;
        accelZSum += accelZ;

        gyroXSum += gyroX;
        gyroYSum += gyroY;
        gyroZSum += gyroZ;

        vTaskDelay(pdMS_TO_TICKS(10));
    }

    mpu_bias.accelX = (int16_t)(accelXSum / 500);
    mpu_bias.accelY = (int16_t)(accelYSum / 500);
    mpu_bias.accelZ = (int16_t)((accelZSum / 500) - 8192);
    mpu_bias.gyroX = (int16_t)(gyroXSum / 500);
    mpu_bias.gyroY = (int16_t)(gyroYSum / 500);
    mpu_bias.gyroZ = (int16_t)(gyroZSum / 500);

    ESP_LOGI(TAG, "Calibration complete!");
    ESP_LOGI(TAG, "Bias — aX:%d aY:%d aZ:%d gX:%d gY:%d gZ:%d",
             mpu_bias.accelX, mpu_bias.accelY, mpu_bias.accelZ,
             mpu_bias.gyroX,  mpu_bias.gyroY,  mpu_bias.gyroZ);

    return (ESP_OK);
}

esp_err_t get_mpu_data(mpu_readings_t *output)
{
    esp_err_t err;
    uint8_t data[14];

    err = i2c_mpu_burst_read(MPU_ACCEL_XOUT_H, data, 14);
    if (err != ESP_OK)
        return (err);
    
    int16_t accelX = (int16_t)((data[0] << 8) | data[1]);;
    int16_t accelY = (int16_t)((data[2] << 8) | (data[3]));
    int16_t accelZ = (int16_t)((data[4] << 8) | (data[5]));
    
    int16_t gyroX = (int16_t)((data[8] << 8) | (data[9]));
    int16_t gyroY = (int16_t)((data[10] << 8) | (data[11]));
    int16_t gyroZ = (int16_t)((data[12] << 8) | (data[13]));

    output->accelX = (accelX - mpu_bias.accelX) / config.accel_sensitivity;
    output->accelY = (accelY - mpu_bias.accelY) / config.accel_sensitivity;
    output->accelZ = (accelZ - mpu_bias.accelZ) / config.accel_sensitivity;

    output->gyroX = (gyroX - mpu_bias.gyroX) / config.gyro_sensitivity;
    output->gyroY = (gyroY - mpu_bias.gyroY) / config.gyro_sensitivity;
    output->gyroZ = (gyroZ - mpu_bias.gyroZ) / config.gyro_sensitivity;

    return (ESP_OK); 
}