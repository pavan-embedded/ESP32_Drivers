#pragma once
#include <stddef.h>
#include <stdint.h>
#include "esp_err.h"
#include "driver/uart.h"

typedef struct {
    uart_port_t port;
    int         tx_pin;
    int         rx_pin;
    int         baud;
    size_t      rx_buf_size;
} uart_drv_cfg_t;

esp_err_t uart_drv_init(const uart_drv_cfg_t *cfg);
int       uart_drv_read(uint8_t *buf, size_t len, uint32_t timeout_ms);
int       uart_drv_write(const uint8_t *data, size_t len);
void      uart_drv_deinit(void);