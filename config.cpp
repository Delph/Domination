#include "config.h"


#include "storage.h"

namespace config
{
  Configuration configuration = {
    .channel = 0,
    .radioID = 0,
    .nodeCount = 5,
  };
  bool loaded = false;

  bool valid()
  {
    uint32_t crc32 = 0;
    storage::read(0, reinterpret_cast<uint8_t*>(&crc32), sizeof(crc32));
    return crc32 == storage::crc32(DATA_ADDRESS, sizeof(Configuration));
  }

  void save()
  {
    uint32_t crc32 = storage::crc32(DATA_ADDRESS, sizeof(Configuration));
    storage::write(CRC_ADDRESS, reinterpret_cast<uint8_t*>(&crc32), sizeof(uint32_t));
    storage::write(DATA_ADDRESS, reinterpret_cast<uint8_t*>(&configuration), sizeof(Configuration));
  }

  void load()
  {
    if (loaded)
      return;

    if (!valid())
    {
      save();
      return;
    }
    storage::read(DATA_ADDRESS, reinterpret_cast<uint8_t*>(&configuration), sizeof(Configuration));
    loaded = true;
  }

  channel_t getChannel()
  {
    load();
    return configuration.channel;
  }
  void setChannel(const channel_t c)
  {
    load();
    configuration.channel = c;
    save();
  }

  radio_id getRadioID()
  {
    load();
    return configuration.radioID;
  }
  void setRadioID(const radio_id id)
  {
    load();
    configuration.radioID = id;
    save();
  }

  uint8_t getNodeCount()
  {
    load();
    return configuration.nodeCount;
  }

  void setNodeCount(const uint8_t count)
  {
    load();
    configuration.nodeCount = count;
    save();
  }
}
