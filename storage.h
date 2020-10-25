#ifndef STORAGE_H_INCLUDE
#define STORAGE_H_INCLUDE


#include <EEPROM.h>

namespace storage
{
  static uint8_t read(const size_t index)
  {
    return EEPROM.read(index);
  }

  static void read(const size_t index, uint8_t* const buf, const size_t len)
  {
    for (size_t i = 0; i < len; ++i)
      buf[i] = storage::read(index + i);
  }

  static void write(const size_t index, uint8_t data)
  {
    // use update instead of write to prevent unnecessary writes
    EEPROM.update(index, data);
  }

  static void write(const size_t index, uint8_t* const buf, const size_t len)
  {
    for (size_t i = 0; i < len; ++i)
      storage::write(index + i, buf[i]);
  }

  static uint32_t crc32(const size_t start = 0, const size_t end = EEPROM.length())
  {
    const uint32_t crc_table[16] = {
      0x00000000, 0x1db71064, 0x3b6e20c8, 0x26d930ac,
      0x76dc4190, 0x6b6b51f4, 0x4db26158, 0x5005713c,
      0xedb88320, 0xf00f9344, 0xd6d6a3e8, 0xcb61b38c,
      0x9b64c2b0, 0x86d3d2d4, 0xa00ae278, 0xbdbdf21c
    };

    uint32_t crc = ~0L;

    for (size_t i = start; i < end; ++i)
    {
      crc = crc_table[(crc ^ storage::read(i)) & 0x0f] ^ (crc >> 4);
      crc = crc_table[(crc ^ (storage::read(i) >> 4)) & 0x0f] ^ (crc >> 4);
      crc = ~crc;
    }
    return crc;
  }
};

#endif
