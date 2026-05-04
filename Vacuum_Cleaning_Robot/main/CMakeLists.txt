idf_component_register(SRCS "main.c" "mcpwm_ctrl.c" "motor_ctrl.c"
                    INCLUDE_DIRS "."
                    REQUIRES driver pid_ctrl)
