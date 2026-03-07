#ifndef __ST7735_H__
#define __ST7735_H__

#include "lcd_com.h"

void st7735_lcdInit(TFT_t *dev, int width, int height, int offsetx, int offsety);
void st7735_lcdDrawPixel(TFT_t *dev, uint16_t x, uint16_t y, uint16_t color);
void st7735_lcdDrawMultiPixels(TFT_t *dev, uint16_t x, uint16_t y, uint16_t size, uint16_t *colors);
void st7735_lcdDrawFillRect(TFT_t *dev, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color);
void st7735_lcdDisplayOff(TFT_t *dev);
void st7735_lcdDisplayOn(TFT_t *dev);
void st7735_lcdInversionOff(TFT_t *dev);
void st7735_lcdInversionOn(TFT_t *dev);
bool st7735_lcdEnableScroll(TFT_t *dev);
void st7735_lcdSetScrollArea(TFT_t *dev, uint16_t tfa, uint16_t vsa, uint16_t bfa);
void st7735_lcdResetScrollArea(TFT_t *dev, uint16_t vsa);
void st7735_lcdStartScroll(TFT_t *dev, uint16_t vsp);

#endif
