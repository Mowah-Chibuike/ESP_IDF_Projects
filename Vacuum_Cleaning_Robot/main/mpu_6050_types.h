#ifndef MPU_TYPES_H
#define MPU_TYPES_H

typedef enum
{
    MPU_SET_SAMPLE_RATE = 0x19,
    MPU_CONFIG_REG 	    = 0x1A,
    MPU_GYRO_CONFIG     = 0x1B,
    MPU_ACCEL_CONFIG    = 0x1C,
    MPU_FIFO_EN 	    = 0x23,
    MPU_ACCEL_XOUT_H    = 0x3B,
    MPU_ACCEL_XOUT_L    = 0x3C,
    MPU_ACCEL_YOUT_H    = 0x3D,
    MPU_ACCEL_YOUT_L    = 0x3E,
    MPU_ACCEL_ZOUT_H    = 0x3F,
    MPU_ACCEL_ZOUT_L    = 0x40,
    MPU_GYRO_XOUT_H	    = 0x43,
    MPU_GYRO_XOUT_L	    = 0x44,
    MPU_GYRO_YOUT_H	    = 0x45,
    MPU_GYRO_YOUT_L	    = 0x46,
    MPU_GYRO_ZOUT_H	    = 0x47,
    MPU_GYRO_ZOUT_L	    = 0x48,
    MPU_PWR_MGMT_1	    = 0x6B,
    MPU_PWR_MGMT_2      = 0x6C,
    MPU_WHO_AM_I        = 0x75
} mpu6050_reg_t;

// FSYNC config
#define EXT_SYNC_SET_0  (0 << 3)
#define EXT_SYNC_SET_1  (1 << 3)
#define EXT_SYNC_SET_2  (2 << 3)
#define EXT_SYNC_SET_3  (3 << 3)
#define EXT_SYNC_SET_4  (4 << 3)
#define EXT_SYNC_SET_5  (5 << 3)
#define EXT_SYNC_SET_6  (6 << 3)
#define EXT_SYNC_SET_7  (7 << 3)    

// DLPF config
#define DLPF_CFG_0  0x00
#define DLPF_CFG_1  0x01
#define DLPF_CFG_2  0x02
#define DLPF_CFG_3  0x03
#define DLPF_CFG_4  0x04
#define DLPF_CFG_5  0x05
#define DLPF_CFG_6  0x06

// CLK_SEL config 
#define CLK_SEL_0   0x00
#define CLK_SEL_1   0x01
#define CLK_SEL_2   0x02
#define CLK_SEL_3   0x03
#define CLK_SEL_4   0x04
#define CLK_SEL_5   0x05
#define CLK_SEL_7   0x07

// Sleep mode config
#define SLEEP_ON    (1 << 6)

// Cycle mode config
#define CYCLE_ON    (1 << 5)

// Temperature sensor config
#define TEMP_DIS    (1 << 3)

// Reset config
#define DEVICE_RESET (1 << 7)

// Wake control
#define LP_WAKE_CTRL_0  (0x00)
#define LP_WAKE_CTRL_1  (1 << 6)
#define LP_WAKE_CTRL_2  (2 << 6)
#define LP_WAKE_CTRL_3  (3 << 6)

// GYRO_CONFIG
#define GYRO_CONFIG_250     (0x00)
#define GYRO_CONFIG_500     (1 << 3)
#define GYRO_CONFIG_1000    (2 << 3)
#define GYRO_CONFIG_2000    (3 << 3)

// ACCEL CONFIG
#define ACCEL_CONFIG_2G     (0x00)
#define ACCEL_CONFIG_4G     (1 << 3)
#define ACCEL_CONFIG_8G     (2 << 3)
#define ACCEL_CONFIG_16G    (3 << 3)

// ACCEL Sensitivity
#define ACCEL_2G    16384
#define ACCEL_4G    8192
#define ACCEL_8G    4096
#define ACCEL_16G   2048

// GYRO Sensitivity
#define GYRO_250    131.0f
#define GYRO_500    65.5f
#define GYRO_1000   32.8f
#define GYRO_2000   16.4f

typedef struct
{
    float accel_sensitivity;
    float gyro_sensitivity;
} mpu_config_t;

typedef struct
{
    int16_t accelX;
    int16_t accelY;
    int16_t accelZ;
    int16_t gyroX;
    int16_t gyroY;
    int16_t gyroZ;
} mpu_raw_data_t;

typedef struct
{
    float accelX;
    float accelY;
    float accelZ;
    float gyroX;
    float gyroY;
    float gyroZ;
} mpu_readings_t;



#endif /* MPU_TYPES_H */