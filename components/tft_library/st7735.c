#include <stdlib.h>
#include <string.h>
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_check.h"
#include "esp_log.h"
#include "esp_lcd_io_spi.h"
#include "esp_lcd_panel_io.h"

#include "lcd_com.h"
#include "lcd_lib.h"
#include "st7735.h"

static const char *TAG = "st7735";

// ESP32 SPI wiring for the 1.77" TFT module.
#define LCD_HOST SPI2_HOST
#define PIN_NUM_SCLK 18
#define PIN_NUM_MOSI 23
#define PIN_NUM_DC 2
#define PIN_NUM_CS 5
#define PIN_NUM_RST 4

#define ST7735_PHYS_WIDTH 128
#define ST7735_PHYS_HEIGHT 160

#define ST7735_CMD_SWRESET 0x01
#define ST7735_CMD_SLPOUT 0x11
#define ST7735_CMD_DISPON 0x29
#define ST7735_CMD_DISPOFF 0x28
#define ST7735_CMD_INVON 0x21
#define ST7735_CMD_INVOFF 0x20
#define ST7735_CMD_MADCTL 0x36
#define ST7735_CMD_COLMOD 0x3A
#define ST7735_CMD_CASET 0x2A
#define ST7735_CMD_RASET 0x2B
#define ST7735_CMD_RAMWR 0x2C

static inline uint16_t be16(uint16_t v)
{
    return (uint16_t)((v << 8) | (v >> 8));
}

static inline bool st7735_bounds_ok(uint16_t x, uint16_t y)
{
    return (x < ST7735_PHYS_WIDTH) && (y < ST7735_PHYS_HEIGHT);
}

static void st7735_cmd(TFT_t *dev, uint8_t cmd)
{
    esp_lcd_panel_io_tx_param(dev->io_handle, cmd, NULL, 0);
}

static void st7735_data(TFT_t *dev, uint8_t cmd, const void *data, size_t len)
{
    esp_lcd_panel_io_tx_param(dev->io_handle, cmd, data, len);
}

static void st7735_set_window(TFT_t *dev, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2)
{
    uint16_t sx1 = (uint16_t)(x1 + dev->_offsetx);
    uint16_t sx2 = (uint16_t)(x2 + dev->_offsetx);
    uint16_t sy1 = (uint16_t)(y1 + dev->_offsety);
    uint16_t sy2 = (uint16_t)(y2 + dev->_offsety);

    uint8_t caset[4] = {
        (uint8_t)(sx1 >> 8), (uint8_t)(sx1 & 0xFF),
        (uint8_t)(sx2 >> 8), (uint8_t)(sx2 & 0xFF),
    };
    uint8_t raset[4] = {
        (uint8_t)(sy1 >> 8), (uint8_t)(sy1 & 0xFF),
        (uint8_t)(sy2 >> 8), (uint8_t)(sy2 & 0xFF),
    };
    st7735_data(dev, ST7735_CMD_CASET, caset, sizeof(caset));
    st7735_data(dev, ST7735_CMD_RASET, raset, sizeof(raset));
}

void st7735_lcdDrawPixel(TFT_t *dev, uint16_t x, uint16_t y, uint16_t color)
{
    if (!st7735_bounds_ok(x, y)) {
        return;
    }
    uint16_t p = be16(color);
    st7735_set_window(dev, x, y, x, y);
    esp_lcd_panel_io_tx_color(dev->io_handle, ST7735_CMD_RAMWR, &p, sizeof(p));
}

void st7735_lcdDrawMultiPixels(TFT_t *dev, uint16_t x, uint16_t y, uint16_t size, uint16_t *colors)
{
    if (y >= ST7735_PHYS_HEIGHT || x >= ST7735_PHYS_WIDTH || size == 0) {
        return;
    }
    uint16_t max_size = ST7735_PHYS_WIDTH - x;
    if (size > max_size) {
        size = max_size;
    }

    uint16_t *buf = malloc(size * sizeof(uint16_t));
    if (!buf) {
        ESP_LOGE(TAG, "malloc failed in multipixels");
        return;
    }
    for (uint16_t i = 0; i < size; i++) {
        buf[i] = be16(colors[i]);
    }
    st7735_set_window(dev, x, y, (uint16_t)(x + size - 1), y);
    esp_lcd_panel_io_tx_color(dev->io_handle, ST7735_CMD_RAMWR, buf, size * sizeof(uint16_t));
    free(buf);
}

void st7735_lcdDrawFillRect(TFT_t *dev, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color)
{
    if (x1 > x2) {
        uint16_t t = x1;
        x1 = x2;
        x2 = t;
    }
    if (y1 > y2) {
        uint16_t t = y1;
        y1 = y2;
        y2 = t;
    }
    if (x1 >= ST7735_PHYS_WIDTH || y1 >= ST7735_PHYS_HEIGHT) {
        return;
    }
    if (x2 >= ST7735_PHYS_WIDTH) {
        x2 = ST7735_PHYS_WIDTH - 1;
    }
    if (y2 >= ST7735_PHYS_HEIGHT) {
        y2 = ST7735_PHYS_HEIGHT - 1;
    }

    uint16_t width = (uint16_t)(x2 - x1 + 1);
    uint16_t height = (uint16_t)(y2 - y1 + 1);
    uint16_t px = be16(color);
    uint16_t *line = malloc(width * sizeof(uint16_t));
    if (!line) {
        ESP_LOGE(TAG, "malloc failed in fill rect");
        return;
    }
    for (uint16_t i = 0; i < width; i++) {
        line[i] = px;
    }

    st7735_set_window(dev, x1, y1, x2, y2);
    for (uint16_t row = 0; row < height; row++) {
        esp_lcd_panel_io_tx_color(dev->io_handle, ST7735_CMD_RAMWR, line, width * sizeof(uint16_t));
    }
    free(line);
}

void st7735_lcdDisplayOff(TFT_t *dev)
{
    st7735_cmd(dev, ST7735_CMD_DISPOFF);
}

void st7735_lcdDisplayOn(TFT_t *dev)
{
    st7735_cmd(dev, ST7735_CMD_DISPON);
}

void st7735_lcdInversionOff(TFT_t *dev)
{
    st7735_cmd(dev, ST7735_CMD_INVOFF);
}

void st7735_lcdInversionOn(TFT_t *dev)
{
    st7735_cmd(dev, ST7735_CMD_INVON);
}

bool st7735_lcdEnableScroll(TFT_t *dev)
{
    (void)dev;
    return false;
}

void st7735_lcdSetScrollArea(TFT_t *dev, uint16_t tfa, uint16_t vsa, uint16_t bfa)
{
    (void)dev;
    (void)tfa;
    (void)vsa;
    (void)bfa;
}

void st7735_lcdResetScrollArea(TFT_t *dev, uint16_t vsa)
{
    (void)dev;
    (void)vsa;
}

void st7735_lcdStartScroll(TFT_t *dev, uint16_t vsp)
{
    (void)dev;
    (void)vsp;
}

void st7735_lcdInit(TFT_t *dev, int width, int height, int offsetx, int offsety)
{
    (void)width;
    (void)height;

    static bool bus_inited = false;
    if (!bus_inited) {
        spi_bus_config_t buscfg = {
            .sclk_io_num = PIN_NUM_SCLK,
            .mosi_io_num = PIN_NUM_MOSI,
            .miso_io_num = -1,
            .quadwp_io_num = -1,
            .quadhd_io_num = -1,
            .max_transfer_sz = ST7735_PHYS_WIDTH * 32 * sizeof(uint16_t),
        };
        ESP_ERROR_CHECK(spi_bus_initialize(LCD_HOST, &buscfg, SPI_DMA_CH_AUTO));
        bus_inited = true;
    }

    esp_lcd_panel_io_spi_config_t io_config = {
        .dc_gpio_num = PIN_NUM_DC,
        .cs_gpio_num = PIN_NUM_CS,
        .pclk_hz = 26000000,
        .lcd_cmd_bits = 8,
        .lcd_param_bits = 8,
        .spi_mode = 0,
        .trans_queue_depth = 10,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)LCD_HOST, &io_config, &dev->io_handle));

    // Hardware reset sequence for modules that expose RES pin.
    gpio_reset_pin(PIN_NUM_RST);
    gpio_set_direction(PIN_NUM_RST, GPIO_MODE_OUTPUT);
    gpio_set_level(PIN_NUM_RST, 1);
    vTaskDelay(pdMS_TO_TICKS(20));
    gpio_set_level(PIN_NUM_RST, 0);
    vTaskDelay(pdMS_TO_TICKS(20));
    gpio_set_level(PIN_NUM_RST, 1);
    vTaskDelay(pdMS_TO_TICKS(120));

    // Basic ST7735 init sequence.
    st7735_cmd(dev, ST7735_CMD_SWRESET);
    vTaskDelay(pdMS_TO_TICKS(150));
    st7735_cmd(dev, ST7735_CMD_SLPOUT);
    vTaskDelay(pdMS_TO_TICKS(120));

    uint8_t colmod = 0x05; // RGB565
    st7735_data(dev, ST7735_CMD_COLMOD, &colmod, 1);

    uint8_t madctl = 0xC8; // portrait + BGR
    st7735_data(dev, ST7735_CMD_MADCTL, &madctl, 1);

    st7735_cmd(dev, ST7735_CMD_INVON);
    st7735_cmd(dev, ST7735_CMD_DISPON);
    vTaskDelay(pdMS_TO_TICKS(100));

    // Keep offsets configurable for cases where the active area is shifted.
    lcdInitDevice(dev, ST7735_PHYS_WIDTH, ST7735_PHYS_HEIGHT, offsetx, offsety);
    dev->_offsetx = (uint16_t)offsetx;
    dev->_offsety = (uint16_t)offsety;

    DrawPixel = st7735_lcdDrawPixel;
    DrawMultiPixels = st7735_lcdDrawMultiPixels;
    DrawFillRect = st7735_lcdDrawFillRect;
    DisplayOff = st7735_lcdDisplayOff;
    DisplayOn = st7735_lcdDisplayOn;
    InversionOff = st7735_lcdInversionOff;
    InversionOn = st7735_lcdInversionOn;
    EnableScroll = st7735_lcdEnableScroll;
    SetScrollArea = st7735_lcdSetScrollArea;
    ResetScrollArea = st7735_lcdResetScrollArea;
    StartScroll = st7735_lcdStartScroll;
}
