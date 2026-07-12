#ifndef _ST7796_H_
#define _ST7796_H_

#include "lcd_com.h"

void st7796_lcdInit(TFT_t *dev, int width, int height, int offsetx, int offsety);
void st7796_lcdDrawPixel(TFT_t *dev, uint16_t x, uint16_t y, uint16_t color);
void st7796_lcdDrawMultiPixels(TFT_t *dev, uint16_t x, uint16_t y, uint16_t size, uint16_t *colors);
void st7796_lcdDrawFillRect(TFT_t *dev, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color);
void st7796_lcdDisplayOff(TFT_t *dev);
void st7796_lcdDisplayOn(TFT_t *dev);
void st7796_lcdInversionOff(TFT_t *dev);
void st7796_lcdInversionOn(TFT_t *dev);
bool st7796_lcdEnableScroll(TFT_t *dev);
void st7796_lcdSetScrollArea(TFT_t *dev, uint16_t tfa, uint16_t vsa, uint16_t bfa);
void st7796_lcdResetScrollArea(TFT_t *dev, uint16_t vsa);
void st7796_lcdStartScroll(TFT_t *dev, uint16_t vsp);

#endif
