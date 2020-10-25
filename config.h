#ifndef CONFIG_H_INCLUDE
#define CONFIG_H_INCLUDE

#include <stdint.h>

#include "storage.h"
#include "types.h"

namespace config
{
  const uint16_t CRC_ADDRESS = 0;
  const uint16_t DATA_ADDRESS = 8;

  // hard coded configuration stuff

  // pre-shared key for checking NFC tags
  const uint8_t PSK[8] = {0x66, 0x75, 0xEC, 0x1C, 0x42, 0x93, 0x73, 0x96};

  struct Configuration
  {
    channel_t channel;
    radio_id radioID;
    uint8_t nodeCount;
  };

  channel_t getChannel();
  void setChannel(const channel_t c);

  radio_id getRadioID();
  void setRadioID(const radio_id id);

  uint8_t getNodeCount();
  void setNodeCount(const uint8_t count);
};


#endif
