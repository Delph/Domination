#include "colour.h"


const uint8_t R_MASK = 0b11111;
const uint8_t G_MASK = 0b111111;
const uint8_t B_MASK = 0b11111;
const uint8_t R_BITS = 5;
const uint8_t G_BITS = 6;
const uint8_t B_BITS = 5;

// constexpr colour_t colour(const float r, const float g, const float b)
// {
//   return static_cast<colour_t>(static_cast<uint8_t>(r * R_MASK) & R_MASK) << 11 |
//     static_cast<colour_t>(static_cast<uint8_t>(g * G_MASK) & G_MASK) << 5 |
//     static_cast<colour_t>(static_cast<uint8_t>(b * B_MASK) & B_MASK
//   );
// }

constexpr colour_t colour(const uint8_t r, const uint8_t g, const uint8_t b)
{
  return static_cast<colour_t>(static_cast<uint8_t>(r >> (8 - R_BITS)) << 11) |
    static_cast<colour_t>(static_cast<uint8_t>(g >> (8 - G_BITS)) << 5) |
    static_cast<colour_t>(static_cast<uint8_t>(b >> (8 - B_BITS)));
}
