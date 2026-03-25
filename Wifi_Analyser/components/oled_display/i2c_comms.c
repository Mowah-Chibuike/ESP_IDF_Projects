#include "i2c_comms.h"

static const char* TAG = "SSD1306 I2C";

static i2c_master_bus_handle_t bus_handle = NULL;
static i2c_master_dev_handle_t dev_handle = NULL;

void i2c_init(void)
{
    i2c_master_bus_config_t bus_cfg = {
        .i2c_port = I2C_NUM_0,
        .sda_io_num = I2C_BUS_SDA,
        .scl_io_num = I2C_BUS_SCK,
        .clk_source =  I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = 1
    };

    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_cfg, &bus_handle));

    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .scl_speed_hz = 400 * 1000,
        .device_address = SSD1306_I2C_ADDR,
    };

    
    ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handle, &dev_cfg, &dev_handle));

    ESP_LOGI(TAG, "I2C bus created and device added successfully at address: %X", SSD1306_I2C_ADDR);
}

esp_err_t i2c_send_cmd_to_dsp(ssd1306_cmd_t cmd, size_t len, ...)
{
    esp_err_t err;
    size_t buf_len = len + 2; // control byte + cmd byte + optional param bytes
    uint8_t *buf = malloc(sizeof(uint8_t) * buf_len);
    if (buf == NULL) return ESP_ERR_NO_MEM;

    buf[0] = SSD1306_CMD_BYTE;
    buf[1] = (uint8_t)cmd;

    if (len > 0) {
        va_list args;
        va_start(args, len);
        for (size_t i = 0; i < len; i++) {
            buf[2 + i] = (uint8_t)va_arg(args, int); // variadic args promote to int
        }
        va_end(args);
    }

    err = i2c_master_transmit(dev_handle, buf, buf_len, -1);
    free(buf);
    return err;
}

esp_err_t i2c_send_data_to_dsp(uint8_t *data, size_t data_length)
{
    uint8_t *buf = NULL;
    buf = malloc(sizeof(uint8_t) * (data_length + 1));
    if (buf == NULL) return ESP_ERR_NO_MEM;

    buf[0] = SSD1306_DATA_BYTE;
    memcpy(buf + 1, data, data_length);
    
    esp_err_t err = i2c_master_transmit(dev_handle, buf, data_length + 1, -1);
    free(buf);
    return (err);
}

esp_err_t i2c_free_dev()
{
    esp_err_t err = i2c_master_bus_rm_device(dev_handle);
    dev_handle = NULL;
    return (err);
}

esp_err_t i2c_free_bus()
{
    esp_err_t err = i2c_del_master_bus(bus_handle);
    bus_handle = NULL;
    return (err);
}