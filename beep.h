#ifndef BEEPS_H_INCLUDE
#define BEEPS_H_INCLUDE

#include <stdint.h>

enum class Note : uint16_t
{
  B5 = 988u,
  A5 = 880u,
  G5 = 784u,
  F5 = 698u,
  E5 = 659u,
  D5 = 587u,
  C5 = 523u,
  B4 = 494u,
  A4 = 440u,
  G4 = 392u,
  F4 = 349u,
  E4 = 330u,
  D4 = 294u,
  C4 = 262u
};

inline void beep(const uint8_t pin, const Note freq, const uint32_t duration)
{
  tone(pin, static_cast<uint16_t>(freq), duration);
}

#endif
