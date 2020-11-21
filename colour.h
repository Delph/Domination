#ifndef COLOUR_H_INCLUDE
#define COLOUR_H_INCLUDE

#include <stdint.h>

using colour_t = uint16_t;

const uint8_t R_BITS = 5;
const uint8_t G_BITS = 6;
const uint8_t B_BITS = 5;

constexpr colour_t colour(const uint8_t r, const uint8_t g, const uint8_t b)
{
  return static_cast<colour_t>(static_cast<uint8_t>(r >> (8 - R_BITS)) << 11) |
  static_cast<colour_t>(static_cast<uint8_t>(g >> (8 - G_BITS)) << 5) |
  static_cast<colour_t>(static_cast<uint8_t>(b >> (8 - B_BITS)));
}

constexpr uint8_t channel_red(const colour_t colour)
{
  return ((colour >> 11) & 0b00011111) << 3;
}

constexpr uint8_t channel_green(const colour_t colour)
{
  return ((colour >> 5) & 0b00111111) << 2;
}

constexpr uint8_t channel_blue(const colour_t colour)
{
  return ((colour >> 0) & 0b00011111) << 3;
}


constexpr colour_t COLOUR_BLACK   = 0x0000;
constexpr colour_t COLOUR_BLUE    = 0x001F;
constexpr colour_t COLOUR_RED     = 0xF800;
constexpr colour_t COLOUR_GREEN   = 0x07E0;
constexpr colour_t COLOUR_CYAN    = 0x07FF;
constexpr colour_t COLOUR_MAGENTA = 0xF81F;
constexpr colour_t COLOUR_YELLOW  = 0xFFE0;
constexpr colour_t COLOUR_WHITE   = 0xFFFF;

#endif
