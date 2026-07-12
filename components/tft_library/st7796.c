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
#include "st7796.h"

static const char *TAG = "st7796";

#define LCD_HOST SPI2_HOST
#define PIN_NUM_SCLK 18
#define PIN_NUM_MOSI 23
#define PIN_NUM_DC 2
#define PIN_NUM_CS 5
#define PIN_NUM_RST 4

#define ST7796_PHYS_WIDTH 320
#define ST7796_PHYS_HEIGHT 480
#define ST7796_TRANSFER_ROWS 40
#define ST7796_SPI_CLOCK_HZ 10000000

#define ST7796_CMD_SWRESET 0x01
#define ST7796_CMD_SLPOUT 0x11
#define ST7796_CMD_DISPON 0x29
#define ST7796_CMD_DISPOFF 0x28
#define ST7796_CMD_INVON 0x21
#define ST7796_CMD_INVOFF 0x20
#define ST7796_CMD_MADCTL 0x36
#define ST7796_CMD_COLMOD 0x3A
#define ST7796_CMD_CASET 0x2A
#define ST7796_CMD_RASET 0x2B
#define ST7796_CMD_RAMWR 0x2C

static inline uint16_t be16(uint16_t v)
{
    return (uint16_t)((v << 8) | (v >> 8));
}

static inline bool st7796_bounds_ok(uint16_t x, uint16_t y)
{
    return (x < ST7796_PHYS_WIDTH) && (y < ST7796_PHYS_HEIGHT);
}

static void st7796_cmd(TFT_t *dev, uint8_t cmd)
{
    esp_lcd_panel_io_tx_param(dev->io_handle, cmd, NULL, 0);
}

static void st7796_data(TFT_t *dev, uint8_t cmd, const void *data, size_t len)
{
    esp_lcd_panel_io_tx_param(dev->io_handle, cmd, data, len);
}

static void st7796_set_window(TFT_t *dev, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2)
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

    st7796_data(dev, ST7796_CMD_CASET, caset, sizeof(caset));
    st7796_data(dev, ST7796_CMD_RASET, raset, sizeof(raset));
}

void st7796_lcdDrawPixel(TFT_t *dev, uint16_t x, uint16_t y, uint16_t color)
{
    if (!st7796_bounds_ok(x, y)) {
        return;
    }

    uint16_t p = be16(color);
    st7796_set_window(dev, x, y, x, y);
    esp_lcd_panel_io_tx_color(dev->io_handle, ST7796_CMD_RAMWR, &p, sizeof(p));
}

void st7796_lcdDrawMultiPixels(TFT_t *dev, uint16_t x, uint16_t y, uint16_t size, uint16_t *colors)
{
    if (y >= dev->_height || x >= dev->_width || size == 0) {
        return;
    }

    uint16_t max_size = (uint16_t)(dev->_width - x);
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

    st7796_set_window(dev, x, y, (uint16_t)(x + size - 1), y);
    esp_lcd_panel_io_tx_color(dev->io_handle, ST7796_CMD_RAMWR, buf, size * sizeof(uint16_t));
    free(buf);
}

void st7796_lcdDrawFillRect(TFT_t *dev, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color)
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
    if (x1 >= dev->_width || y1 >= dev->_height) {
        return;
    }
    if (x2 >= dev->_width) {
        x2 = (uint16_t)(dev->_width - 1);
    }
    if (y2 >= dev->_height) {
        y2 = (uint16_t)(dev->_height - 1);
    }

    uint16_t width = (uint16_t)(x2 - x1 + 1);
    uint16_t height = (uint16_t)(y2 - y1 + 1);
    uint16_t rows_per_chunk = ST7796_TRANSFER_ROWS;
    if (rows_per_chunk > height) {
        rows_per_chunk = height;
    }

    size_t chunk_pixels = (size_t)width * rows_per_chunk;
    uint16_t px = be16(color);
    uint16_t *chunk = malloc(chunk_pixels * sizeof(uint16_t));
    if (!chunk) {
        ESP_LOGE(TAG, "malloc failed in fill rect");
        return;
    }

    for (size_t i = 0; i < chunk_pixels; i++) {
        chunk[i] = px;
    }

    uint16_t y = y1;
    uint16_t remaining = height;
    while (remaining > 0) {
        uint16_t rows = remaining > rows_per_chunk ? rows_per_chunk : remaining;
        size_t pixels = (size_t)width * rows;

        st7796_set_window(dev, x1, y, x2, (uint16_t)(y + rows - 1));
        esp_lcd_panel_io_tx_color(dev->io_handle, ST7796_CMD_RAMWR, chunk, pixels * sizeof(uint16_t));

        y = (uint16_t)(y + rows);
        remaining = (uint16_t)(remaining - rows);
    }
    free(chunk);
}

void st7796_lcdDisplayOff(TFT_t *dev)
{
    st7796_cmd(dev, ST7796_CMD_DISPOFF);
}

void st7796_lcdDisplayOn(TFT_t *dev)
{
    st7796_cmd(dev, ST7796_CMD_DISPON);
}

void st7796_lcdInversionOff(TFT_t *dev)
{
    st7796_cmd(dev, ST7796_CMD_INVOFF);
}

void st7796_lcdInversionOn(TFT_t *dev)
{
    st7796_cmd(dev, ST7796_CMD_INVON);
}

bool st7796_lcdEnableScroll(TFT_t *dev)
{
    (void)dev;
    return false;
}

void st7796_lcdSetScrollArea(TFT_t *dev, uint16_t tfa, uint16_t vsa, uint16_t bfa)
{
    (void)dev;
    (void)tfa;
    (void)vsa;
    (void)bfa;
}

void st7796_lcdResetScrollArea(TFT_t *dev, uint16_t vsa)
{
    (void)dev;
    (void)vsa;
}

void st7796_lcdStartScroll(TFT_t *dev, uint16_t vsp)
{
    (void)dev;
    (void)vsp;
}

void st7796_lcdInit(TFT_t *dev, int width, int height, int offsetx, int offsety)
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
            .max_transfer_sz = ST7796_PHYS_WIDTH * ST7796_TRANSFER_ROWS * sizeof(uint16_t),
        };
        ESP_ERROR_CHECK(spi_bus_initialize(LCD_HOST, &buscfg, SPI_DMA_CH_AUTO));
        bus_inited = true;
    }

    esp_lcd_panel_io_spi_config_t io_config = {
        .dc_gpio_num = PIN_NUM_DC,
        .cs_gpio_num = PIN_NUM_CS,
        .pclk_hz = ST7796_SPI_CLOCK_HZ,
        .lcd_cmd_bits = 8,
        .lcd_param_bits = 8,
        .spi_mode = 0,
        .trans_queue_depth = 10,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)LCD_HOST, &io_config, &dev->io_handle));

    gpio_reset_pin(PIN_NUM_RST);
    gpio_set_direction(PIN_NUM_RST, GPIO_MODE_OUTPUT);
    gpio_set_level(PIN_NUM_RST, 1);
    vTaskDelay(pdMS_TO_TICKS(20));
    gpio_set_level(PIN_NUM_RST, 0);
    vTaskDelay(pdMS_TO_TICKS(20));
    gpio_set_level(PIN_NUM_RST, 1);
    vTaskDelay(pdMS_TO_TICKS(120));

    st7796_cmd(dev, ST7796_CMD_SWRESET);
    vTaskDelay(pdMS_TO_TICKS(150));
    st7796_cmd(dev, ST7796_CMD_SLPOUT);
    vTaskDelay(pdMS_TO_TICKS(120));

    {
        uint8_t data[] = {0xC3};
        st7796_data(dev, 0xF0, data, sizeof(data));
    }
    {
        uint8_t data[] = {0x96};
        st7796_data(dev, 0xF0, data, sizeof(data));
    }
    {
        uint8_t data[] = {0x48};
        st7796_data(dev, ST7796_CMD_MADCTL, data, sizeof(data));
    }
    {
        uint8_t data[] = {0x55};
        st7796_data(dev, ST7796_CMD_COLMOD, data, sizeof(data));
    }
    {
        uint8_t data[] = {0x01};
        st7796_data(dev, 0xB4, data, sizeof(data));
    }
    {
        uint8_t data[] = {0xC6};
        st7796_data(dev, 0xB7, data, sizeof(data));
    }
    {
        uint8_t data[] = {0x80, 0x02, 0x3B};
        st7796_data(dev, 0xB6, data, sizeof(data));
    }
    {
        uint8_t data[] = {0x40, 0x8A, 0x00, 0x00, 0x29, 0x19, 0xA5, 0x33};
        st7796_data(dev, 0xE8, data, sizeof(data));
    }
    {
        uint8_t data[] = {0x06};
        st7796_data(dev, 0xC1, data, sizeof(data));
    }
    {
        uint8_t data[] = {0xA7};
        st7796_data(dev, 0xC2, data, sizeof(data));
    }
    {
        uint8_t data[] = {0x18};
        st7796_data(dev, 0xC5, data, sizeof(data));
    }
    {
        uint8_t data[] = {0xF0, 0x09, 0x0B, 0x06, 0x04, 0x15, 0x2F, 0x54, 0x42, 0x3C, 0x17, 0x14, 0x18, 0x1B};
        st7796_data(dev, 0xE0, data, sizeof(data));
    }
    {
        uint8_t data[] = {0xE0, 0x09, 0x0B, 0x06, 0x04, 0x03, 0x2B, 0x43, 0x42, 0x3B, 0x16, 0x14, 0x17, 0x1B};
        st7796_data(dev, 0xE1, data, sizeof(data));
    }
    {
        uint8_t data[] = {0x3C};
        st7796_data(dev, 0xF0, data, sizeof(data));
    }
    {
        uint8_t data[] = {0x69};
        st7796_data(dev, 0xF0, data, sizeof(data));
    }
    st7796_cmd(dev, ST7796_CMD_INVON);
    st7796_cmd(dev, ST7796_CMD_DISPON);
    vTaskDelay(pdMS_TO_TICKS(100));

    lcdInitDevice(dev, ST7796_PHYS_WIDTH, ST7796_PHYS_HEIGHT, offsetx, offsety);
    dev->_offsetx = (uint16_t)offsetx;
    dev->_offsety = (uint16_t)offsety;

    DrawPixel = st7796_lcdDrawPixel;
    DrawMultiPixels = st7796_lcdDrawMultiPixels;
    DrawFillRect = st7796_lcdDrawFillRect;
    DisplayOff = st7796_lcdDisplayOff;
    DisplayOn = st7796_lcdDisplayOn;
    InversionOff = st7796_lcdInversionOff;
    InversionOn = st7796_lcdInversionOn;
    EnableScroll = st7796_lcdEnableScroll;
    SetScrollArea = st7796_lcdSetScrollArea;
    ResetScrollArea = st7796_lcdResetScrollArea;
    StartScroll = st7796_lcdStartScroll;
}
