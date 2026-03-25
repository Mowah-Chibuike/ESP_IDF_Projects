idf_component_register(SRCS "font.c" "helpers.c" "i2c_comms.c"
                    INCLUDE_DIRS "include"
                    PRIV_REQUIRES esp_driver_i2c)
