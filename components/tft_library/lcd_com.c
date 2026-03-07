#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "lcd_com.h"

void gpio_digital_write(int gpio_pin, uint8_t data)
{
    gpio_set_level(gpio_pin, data ? 1 : 0);
}

void gpio_lcd_write_data(int dummy1, unsigned char *data, size_t size)
{
    (void)dummy1;
    (void)data;
    (void)size;
}

void reg_lcd_write_data(int dummy1, unsigned char *data, size_t size)
{
    (void)dummy1;
    (void)data;
    (void)size;
}

void lcd_write_table(TFT_t *dev, const void *table, int16_t size)
{
    (void)dev;
    (void)table;
    (void)size;
}

void lcd_write_table16(TFT_t *dev, const void *table, int16_t size)
{
    (void)dev;
    (void)table;
    (void)size;
}

void lcd_write_comm_byte(TFT_t *dev, uint8_t cmd)
{
    (void)dev;
    (void)cmd;
}

void lcd_write_comm_word(TFT_t *dev, uint16_t cmd)
{
    (void)dev;
    (void)cmd;
}

void lcd_write_data_byte(TFT_t *dev, uint8_t data)
{
    (void)dev;
    (void)data;
}

void lcd_write_data_word(TFT_t *dev, uint16_t data)
{
    (void)dev;
    (void)data;
}

void lcd_write_addr(TFT_t *dev, uint16_t addr1, uint16_t addr2)
{
    (void)dev;
    (void)addr1;
    (void)addr2;
}

void lcd_write_color(TFT_t *dev, uint16_t color, uint16_t size)
{
    (void)dev;
    (void)color;
    (void)size;
}

void lcd_write_colors(TFT_t *dev, uint16_t *colors, uint16_t size)
{
    (void)dev;
    (void)colors;
    (void)size;
}

void lcd_delay_ms(int delay_time)
{
    vTaskDelay(pdMS_TO_TICKS(delay_time));
}

void lcd_write_register_word(TFT_t *dev, uint16_t addr, uint16_t data)
{
    (void)dev;
    (void)addr;
    (void)data;
}

void lcd_write_register_byte(TFT_t *dev, uint8_t addr, uint16_t data)
{
    (void)dev;
    (void)addr;
    (void)data;
}

esp_err_t lcd_interface_cfg(TFT_t *dev, int interface)
{
    memset(dev, 0, sizeof(*dev));
    dev->_interface = interface;
    return ESP_OK;
}
