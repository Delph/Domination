#ifndef PACKETS_H_INCLUDE
#define PACKETS_H_INCLUDE

#include <stdint.h>

struct RadioPacket
{
  uint8_t source;
  uint8_t teamID;
  uint32_t millis;
  uint32_t FailedTxCount;
  uint32_t SuccessTxCount;
};

struct PingPacket
{
  uint8_t source;
  uint32_t millis;
};


#endif
