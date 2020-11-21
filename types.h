#ifndef TYPES_H_INCLUDE
#define TYPES_H_INCLUDE

#include <stdint.h>

using channel_t = uint8_t;
using radio_id = uint8_t;
using millis_t = uint32_t;
using team_id = int8_t;
using player_id = int8_t;

static const team_id NO_TEAM = -1;

#endif
