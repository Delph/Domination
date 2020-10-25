#ifndef PACKETS_H_INCLUDE
#define PACKETS_H_INCLUDE

#include <stdint.h>

#include "types.h"
#include "config.h"

/**
 * packet type enums to prepend to messages
 */
enum class PacketType : uint8_t
{
  PingPacket,
  PongPacket,
  GameTypePacket,
  GameStartPacket,
  NodeStatePacket
};

/**
 * different packet types
 * max packet size is 32 bytes
 * `origin` is always the radio ID of the initial sender
 * `source` is always the radio ID of the sender of the packet
 * `target` is always the radio ID of the target node, not necessarily the node that will receive the packet
 * `timestamp` is always the sender's millis() value
 */

enum class OpCode : uint8_t
{
  PING,
  PONG,
  // GRAPH,
  // LOCATION,
  GAME_SETUP,
  CLAIM,
  WIN
};

struct Packet
{
  OpCode opcode;
  // radio_id origin;
  radio_id source;
  // radio_id target;

  millis_t timestamp;

  // 26 bytes left
  union {
    // struct
    // {
    //   float latitude;
    //   float longitude;
    // };

    // game setup
    struct {
      uint8_t nodes;
      uint8_t teams;
    };

    // claim / win
    struct {
      team_id team;
    };
  };
};



/**
 * Packet for pinging, if we include GPS, then it should include lat and lng
 */
struct PingPacket
{
  const PacketType type = PacketType::PingPacket;
  radio_id origin;
  radio_id source;
  radio_id target;
  millis_t timestamp;

  // float lat;
  // float lng;
};


/**
 * Ping response packet
 * requester is the source from PingPacket
 */
struct PongPacket
{
  const PacketType type = PacketType::PongPacket;
  radio_id source;
  millis_t timestamp;
  radio_id requester;
};


// gameplay packets
/**
 * A packet to describe what the game is
 */
struct GameTypePacket
{
  radio_id source;
  millis_t timestamp;
};


/**
 * A packet to notify the start of a game
 */
struct GameStartPacket
{
  radio_id source;
  millis_t timestamp;
};

/**
 * A packet to provide updates of the node's state
 */
struct NodeStatePacket
{
  radio_id source;
  millis_t timestamp;
  uint8_t teamID;
};

#endif
