#include "uart_drv.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"

static const char *TAG = "uart_drv";
static uart_port_t s_port = UART_NUM_MAX;

esp_err_t uart_drv_init(const uart_drv_cfg_t *cfg)
{
    if (!cfg) return ESP_ERR_INVALID_ARG;

    uart_config_t uc = {
        .baud_rate  = cfg->baud,
        .data_bits  = UART_DATA_8_BITS,
        .parity     = UART_PARITY_DISABLE,
        .stop_bits  = UART_STOP_BITS_1,
        .flow_ctrl  = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    esp_err_t err = uart_driver_install(cfg->port, cfg->rx_buf_size, 0, 0, NULL, 0);
    if (err != ESP_OK) return err;

    err = uart_param_config(cfg->port, &uc);
    if (err != ESP_OK) return err;

    err = uart_set_pin(cfg->port, cfg->tx_pin, cfg->rx_pin,
                       UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    if (err != ESP_OK) return err;

    s_port = cfg->port;
    ESP_LOGI(TAG, "UART%d ready: %d baud, TX=%d RX=%d",
             cfg->port, cfg->baud, cfg->tx_pin, cfg->rx_pin);
    return ESP_OK;
}

int uart_drv_read(uint8_t *buf, size_t len, uint32_t timeout_ms)
{
    return uart_read_bytes(s_port, buf, len, pdMS_TO_TICKS(timeout_ms));
}

int uart_drv_write(const uint8_t *data, size_t len)
{
    return uart_write_bytes(s_port, data, len);
}

void uart_drv_deinit(void)
{
    if (s_port != UART_NUM_MAX) {
        uart_driver_delete(s_port);
        s_port = UART_NUM_MAX;
    }
}