#ifndef DEBOUNCE_H_INCLUDE
#define DEBOUNCE_H_INCLUDE

#include <Arduino.h>

class Debounce
{
public:
  Debounce(const uint8_t p, const bool low = false) : pin(p), active_low(low), state(0) {}

  void init()
  {
    pinMode(pin, INPUT);
  }

  inline uint8_t getPin() const { return pin; }

  void update() {
    state = state << 1 | (active_low ? !digitalRead(pin) : digitalRead(pin));
  }

  bool is_pressed()
  {
    const uint8_t MASK = 0b11000111;
    if ((state & MASK) == 0b00000111)
    {
      state = 0b11111111;
      return true;
    }
    return false;
  }

  bool is_released()
  {
    const uint8_t MASK = 0b11000111;
    if ((state & MASK) == 0b11000000)
    {
      state = 0b00000000;
      return true;
    }
    return false;
  }

  bool is_down() const
  {
    return state == 0b11111111;
  }
  bool is_up() const
  {
    return state == 0b00000000;
  }

private:
  const uint8_t pin;
  const bool active_low;
  uint8_t state;
};

#endif
