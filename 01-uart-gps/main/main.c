#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "uart_drv.h"

#define LINE_MAX_LEN 128

static const char *TAG = "GPS";

typedef struct {
    bool   valid;
    int    fix_quality;
    int    sats;
    double lat, lon;     // decimal degrees
    float  hdop, alt;
    char   utc[11];
} gps_fix_t;

// NMEA checksum: XOR of everything between '$' and '*'
static bool nmea_checksum_ok(const char *s)
{
    if (s[0] != '$') return false;
    uint8_t cs = 0;
    const char *p = s + 1;
    while (*p && *p != '*') cs ^= (uint8_t)*p++;
    if (*p != '*') return false;
    return cs == (uint8_t)strtol(p + 1, NULL, 16);
}

// Split on commas, keeping empty fields (strtok would skip them)
static int split_fields(char *s, char *fields[], int max)
{
    int n = 0;
    fields[n++] = s;
    for (char *p = s; *p && n < max; p++) {
        if (*p == ',' || *p == '*') {
            char c = *p;
            *p = '\0';
            fields[n++] = p + 1;
            if (c == '*') break;
        }
    }
    return n;
}

// NMEA ddmm.mmmm -> decimal degrees
static double nmea_to_deg(const char *s, char hemi)
{
    if (!s[0]) return 0.0;
    double v = atof(s);
    int deg = (int)(v / 100);
    double d = deg + (v - deg * 100) / 60.0;
    return (hemi == 'S' || hemi == 'W') ? -d : d;
}

static bool parse_gga(char *line, gps_fix_t *fix)
{
    char *f[16];
    int n = split_fields(line, f, 16);
    if (n < 10) return false;

    strncpy(fix->utc, f[1], sizeof(fix->utc) - 1);
    fix->fix_quality = atoi(f[6]);
    fix->sats        = atoi(f[7]);
    fix->hdop        = atof(f[8]);
    fix->alt         = atof(f[9]);
    fix->valid       = fix->fix_quality > 0;
    if (fix->valid) {
        fix->lat = nmea_to_deg(f[2], f[3][0]);
        fix->lon = nmea_to_deg(f[4], f[5][0]);
    }
    return true;
}

static void gps_task(void *arg)
{
    uint8_t buf[64];
    char line[LINE_MAX_LEN];
    size_t idx = 0;

    for (;;) {
        int n = uart_drv_read(buf, sizeof(buf), 100);
        for (int i = 0; i < n; i++) {
            char c = (char)buf[i];

            if (c == '$') idx = 0;                 // new sentence, resync
            if (idx < LINE_MAX_LEN - 1) line[idx++] = c;
            else idx = 0;                          // too long, drop

            if (c == '\n') {
                line[idx] = '\0';
                idx = 0;

                if (!nmea_checksum_ok(line)) {
                    ESP_LOGW(TAG, "bad checksum");
                    continue;
                }
                if (strncmp(line + 3, "GGA", 3) == 0) {
                    gps_fix_t fix = {0};
                    if (parse_gga(line, &fix)) {
                        if (fix.valid) {
                            ESP_LOGI(TAG, "FIX lat=%.6f lon=%.6f sats=%d hdop=%.1f alt=%.1fm utc=%s",
                                     fix.lat, fix.lon, fix.sats, fix.hdop, fix.alt, fix.utc);
                        } else {
                            ESP_LOGI(TAG, "No fix yet (sats used: %d)", fix.sats);
                        }
                    }
                }
            }
        }
    }
}

void app_main(void)
{
    uart_drv_cfg_t cfg = {
        .port        = UART_NUM_2,
        .tx_pin      = 17,
        .rx_pin      = 16,
        .baud        = 9600,
        .rx_buf_size = 2048,
    };
    ESP_ERROR_CHECK(uart_drv_init(&cfg));
    xTaskCreate(gps_task, "gps_task", 4096, NULL, 5, NULL);
}