#ifndef I2C_COMMS_H
#define I2C_COMMS_H

#include <driver/i2c_master.h>
#include <esp_log.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#define I2C_BUS_SDA         GPIO_NUM_21
#define I2C_BUS_SCK         GPIO_NUM_22

#define SSD1306_I2C_ADDR    0x3C
#define SSD1306_CMD_BYTE    0x00
#define SSD1306_DATA_BYTE   0x40

typedef enum
{
    SSD1306_SET_CONTRAST        =      0x81,
    SSD1306_RESUME_RAM_CONTENT  =      0xA4,
    SSD1306_STOP_RAM_CONTENT    =      0xA5,
    SSD1306_DISPLAY_NORMAL      =      0xA6,
    SSD1306_DISPLAY_INVERTED    =      0xA7,
    SSD1306_DISPLAY_OFF         =      0xAE,
    SSD1306_DISPLAY_ON          =      0xAF,
    SSD1306_SET_LOWER_COL       =      0x00,
    SSD1306_SET_UPPER_COL       =      0X10,
    SSD1306_SET_ADDRESSING_MODE =      0x20,
    SSD1306_SET_PAGE_ADDRESS_0  =      0xB0,
    SSD1306_SET_SEGMENT_NOREMAP =      0xA0,
    SSD1306_SET_SEGMENT_REMAP   =      0xA1,
    SSD1306_SET_COM_SCAN_DIR    =      0xC8
} ssd1306_cmd_t;

void i2c_init(void);
esp_err_t i2c_send_cmd_to_dsp(ssd1306_cmd_t cmd, size_t len, ...);
esp_err_t i2c_send_data_to_dsp(uint8_t *data, size_t data_length);
esp_err_t i2c_free_dev();
esp_err_t i2c_free_bus();

#endif /* I2C_COMMS_H */