#ifndef DISPLAY_H_INCLUDE
#define DISPLAY_H_INCLUDE


#define PDQ_LIBS

#ifdef PDQ_LIBS
#include "PDQ_ST7735_config.h"

#include <PDQ_GFX.h>
#include <PDQ_ST7735.h>
static PDQ_ST7735 tft = PDQ_ST7735();

#else

#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
static Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, PIN_SPI_RESET);

#endif

#endif
