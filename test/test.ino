#include <SPI.h>
#include <NRFLite.h>

#define ENABLE_RFID

#ifdef ENABLE_RFID
#include <MFRC522.h>
#endif

#include <FastLED.h>

#include "packets.h"

const uint8_t PIN_RADIO_CE = 8;
const uint8_t PIN_RADIO_CSN = 7;
const uint8_t PIN_LED = 4;
const uint8_t PIN_SWITCH_RED = 5;
const uint8_t PIN_SWITCH_BLUE = 6;

#ifdef ENABLE_RFID
const uint8_t PIN_RFID_SELECT = 10;
const uint8_t PIN_RFID_RESET = 9;
#endif

const uint8_t RADIO_ID = 0;
const uint8_t DESTINATION_RADIO_ID_1 = 1;
const uint8_t DESTINATION_RADIO_ID_2 = 2;
const uint8_t CHANNEL = 12;

#ifdef ENABLE_RFID
const uint8_t BLOCK = 4;
const uint8_t TRAILER = 7;
#endif

const uint32_t CORRECTION = 0xFFFFFF;

const uint8_t NUM_NODES = 3;
uint8_t nodes[NUM_NODES] = {0};
const uint8_t TEAM_RED = 1;
const uint8_t TEAM_BLUE = 2;

NRFLite radio;
RadioPacket packet;

MFRC522 mfrc522(PIN_RFID_SELECT, PIN_RFID_RESET);
MFRC522::MIFARE_Key key;

CRGB leds[1];


void indicate()
{
  switch (packet.teamID)
  {
    case 0:
      leds[0] = CRGB::White;
    break;
    case TEAM_RED:
      leds[0] = CRGB::Red;
    break;
    case TEAM_BLUE:
      leds[0] = CRGB::Blue;
    break;
  }

  FastLED.show();
}


void setup()
{
  Serial.begin(9600);

  Serial.println("Starting up SPI");
  SPI.begin();


  // pick the lowest data rate, gives us better range
  Serial.println("Starting up comms");
  if (!radio.init(RADIO_ID, PIN_RADIO_CE, PIN_RADIO_CSN, NRFLite::BITRATE250KBPS, CHANNEL))
  {
    while (true)
    {
      digitalWrite(LED_BUILTIN, 1);
      delay(500);
      digitalWrite(LED_BUILTIN, 0);
      delay(500);
    }
  }

  #ifdef ENABLE_RFID
  Serial.println("Starting up RFID");
  mfrc522.PCD_Init();
  for (uint8_t i = 0; i < 6; ++i)
    key.keyByte[i] = 0xFF;
  #endif

  packet.source = RADIO_ID;

  Serial.println("Configuring LEDS");
  FastLED.addLeds<WS2812, PIN_LED, GRB>(leds, 0, 1).setCorrection(CORRECTION);
  FastLED.setBrightness(127);

  pinMode(PIN_SWITCH_RED, INPUT);
  pinMode(PIN_SWITCH_BLUE, INPUT);


  // reset
  packet.teamID = 0;
  memset(nodes, 0, sizeof(nodes));
  indicate();
  Serial.println("BEGIN");

  uint8_t num = 0;
  const uint32_t start = millis();
  for (uint8_t i = 0; i < 16; ++i)
  {
    if (i == RADIO_ID)
      continue;
    PingPacket packet;
    packet.source = RADIO_ID;
    packet.millis = millis();
    bool success = radio.send(i, &packet, sizeof(packet));
    if (success)
      ++num;
  }
  const uint32_t end = millis();
  Serial.print("Time to scan network: ");
  Serial.println(end - start);
  Serial.print("Found ");
  Serial.print(num, DEC);
  Serial.println(" nodes");
}


void loop()
{
  while (radio.hasData())
  {
    Serial.println("RECV");
    const uint8_t length = radio.hasData();
    RadioPacket data;
    radio.readData(&data);

    nodes[data.source] = data.teamID;
  }

  const uint8_t currentTeam = packet.teamID;
  if (digitalRead(PIN_SWITCH_RED))
    packet.teamID = TEAM_RED;

  #ifndef ENABLE_RFID
  if (digitalRead(PIN_SWITCH_BLUE))
     packet.teamID = TEAM_BLUE;
  #else
  uint8_t buffer[18];
  uint8_t size = sizeof(buffer);
  if (
    mfrc522.PICC_IsNewCardPresent() &&
    mfrc522.PICC_ReadCardSerial() &&
    mfrc522.PICC_GetType(mfrc522.uid.sak) == MFRC522::PICC_TYPE_MIFARE_1K &&
    mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, TRAILER, &key, &(mfrc522.uid)) == MFRC522::STATUS_OK &&
    mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_B, TRAILER, &key, &(mfrc522.uid)) == MFRC522::STATUS_OK &&
    mfrc522.MIFARE_Read(BLOCK, buffer, &size) == MFRC522::STATUS_OK)
  {
    // data is now in buffer, but 'eh, testin'
    packet.teamID = TEAM_BLUE;
  }
  mfrc522.PICC_HaltA();
  mfrc522.PCD_StopCrypto1();
  #endif

  if (currentTeam != packet.teamID)
  {
    Serial.println("SEND");
    nodes[RADIO_ID] = packet.teamID;
    packet.millis = millis();

    indicate();
    radio.send(DESTINATION_RADIO_ID_1, &packet, sizeof(packet));
    radio.send(DESTINATION_RADIO_ID_2, &packet, sizeof(packet));
  }

  if (nodes[0] != 0)
  {
    bool win = true;
    for (uint8_t i = 1; i < NUM_NODES; ++i)
    {
      if (nodes[i] != nodes[0])
      {
        win = false;
        break;
      }
    }

    if (win)
    {
      leds[0] = CRGB::Green;
      FastLED.show();
      while (1);
    }
  }

}
