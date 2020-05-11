#ifndef COLOUR_H_INCLUDE
#define COLOUR_H_INCLUDE

#include <stdint.h>

using colour_t = uint16_t;

constexpr colour_t colour(const uint8_t r, const uint8_t g, const uint8_t b);

constexpr colour_t COLOUR_BLACK   = 0x0000;
constexpr colour_t COLOUR_BLUE    = 0x001F;
constexpr colour_t COLOUR_RED     = 0xF800;
constexpr colour_t COLOUR_GREEN   = 0x07E0;
constexpr colour_t COLOUR_CYAN    = 0x07FF;
constexpr colour_t COLOUR_MAGENTA = 0xF81F;
constexpr colour_t COLOUR_YELLOW  = 0xFFE0;
constexpr colour_t COLOUR_WHITE   = 0xFFFF;

#endif
