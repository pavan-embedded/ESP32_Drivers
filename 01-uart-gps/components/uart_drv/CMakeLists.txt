idf_component_register(
    SRCS "uart_drv.c"
    INCLUDE_DIRS "include"
    REQUIRES esp_driver_uart
    PRIV_REQUIRES esp_driver_gpio
)