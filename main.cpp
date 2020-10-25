#include <SPI.h>
#include <NRFLite.h>

#define PDQ_LIBS
#ifndef PDQ_LIBS
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#endif

// #include <MFRC522.h>
#include "src/rfid/MFRC522.h"

#include "colour.h"
#include "config.h"
#include "debounce.h"
#include "storage.h"
#include "types.h"
#include "packets.h"

const uint8_t PIN_SPEAKER = 3;
const uint8_t PIN_SPI_RESET = 4;
const uint8_t PIN_LED_R = 5;
const uint8_t PIN_LED_G = 6;
const uint8_t PIN_LED_B = 9;
const uint8_t PIN_RADIO_CSN = 7;
const uint8_t PIN_RADIO_CE = 8;

const uint8_t PIN_RFID_SELECT = 10;

const uint8_t PIN_SELECT = A3;
const uint8_t PIN_PREV = A4;
const uint8_t PIN_NEXT = A5;

const uint8_t TFT_DC = A0;
const uint8_t TFT_CS = A1;

// #define KEY_TONES
#ifdef KEY_TONES
#include "beep.h"
#endif

const uint8_t RADIO_ID = 0;
const uint8_t CHANNEL = 0;

NRFLite radio;


MFRC522 mfrc522(PIN_RFID_SELECT, PIN_SPI_RESET);
MFRC522::MIFARE_Key key;
const uint8_t BLOCK = 4;
const uint8_t TRAILER = 7;
const uint8_t PSK[8] = {0x66, 0x75, 0xEC, 0x1C, 0x42, 0x93, 0x73, 0x96};
const uint8_t RFID_BUFFER_LENGTH = 18;


Debounce pinNext(PIN_NEXT);
Debounce pinPrev(PIN_PREV);
Debounce pinSelect(PIN_SELECT);


const uint8_t MAX_NODES = 8;

struct NodeState
{
  millis_t lastUpdate;
  millis_t lastPing;
  millis_t latency;

  float lat;
  float lng;

  team_id team; // who owns this node
};
NodeState nodes[MAX_NODES];
// radio_id graph[MAX_NODES][MAX_NODES];

struct Game
{
  millis_t start;
  millis_t end;
  uint8_t nodes;
  uint8_t teams;
};
Game game = {
  .start = 0,
  .end = 0
};


// networking code

inline bool radio_init()
{
  return radio.init(config::getRadioID(), PIN_RADIO_CE, PIN_RADIO_CSN, NRFLite::BITRATE2MBPS, config::getChannel());
}

/**
 * Relays packets to all nodes except the origin
 * @param packet packet to relay
 */
void relay(Packet& packet)
{
  const radio_id me = config::getRadioID();
  packet.source = me;
  for (uint8_t i = 0; i < MAX_NODES; ++i)
  {
    if (i == me)
      continue;
    if (i == packet.source)
      continue;
    radio.send(i, &packet, sizeof(packet));
  }
}

/**
 * Broadcasts a packet to all radios
 * @param packet packet to broadcast
 */
void broadcast(Packet& packet)
{
  const radio_id me = config::getRadioID();
  for (uint8_t i = 0; i < MAX_NODES; ++i)
  {
    if (i == me)
      continue;
    radio.send(i, &packet, sizeof(packet));
  }
}


void network()
{
  // handle any incoming
  while (radio.hasData())
  {
    Packet packet;
    radio.readData(&packet);


    Serial.print("PACKET: opcode; ");
    Serial.print(static_cast<uint8_t>(packet.opcode));
    Serial.print(", source; ");
    Serial.println(packet.source);
    // Serial.print(", target; ");
    // Serial.println(packet.target);

/*
    const bool relay = packet.opcode != OpCode::PING || packet.opcode != OpCode::PONG;
    if (relay && packet.target != config::getRadioID())
    {
      relay(packet);
      continue;
    }
*/

    if (nodes[packet.source].lastUpdate < packet.timestamp)
      nodes[packet.source].lastUpdate = packet.timestamp;

    switch (packet.opcode)
    {
      case OpCode::PING:
      {
        // craft a pong packet back
        Packet pong = {
          .opcode = OpCode::PONG,
          .source = config::getRadioID(),
          // .target = packet.source,
          .timestamp = packet.timestamp
        };
        radio.send(packet.source, &pong, sizeof(pong));
      }
      break;
      case OpCode::PONG:
        nodes[packet.source].latency = millis() - packet.timestamp;
      break;
      case OpCode::GAME_SETUP:
        game.start = millis() - nodes[packet.source].latency;
        game.nodes = packet.nodes;
        game.teams = packet.teams;

        for (uint8_t i = 0; i < MAX_NODES; ++i)
          nodes[i].team = NO_TEAM;
      break;
      case OpCode::CLAIM:
        nodes[packet.source].team = packet.team;
      break;
      case OpCode::WIN:
        nodes[packet.source].team = packet.team;
        game.end = millis() - packet.timestamp;
      default:
        // Serial.print("Unhandled packet type: ");
        // Serial.println(static_cast<uint8_t>(packet.opcode));
      break;
    }
  }

  const millis_t PING_INTERVAL = 1000;
  // ping any potential nodes
  for (uint8_t i = 0; i < MAX_NODES; ++i)
  {
    if (i == config::getRadioID())
      continue;
    if (nodes[i].lastPing + PING_INTERVAL > millis())
      continue;
    nodes[i].lastPing = millis();
    Packet packet = {
      .opcode = OpCode::PING,
      .source = config::getRadioID(),
      // .target = i,
      .timestamp = millis()
    };
    Serial.print("PING: ");
    Serial.print(i);
    Serial.print(", ");
    Serial.println(radio.send(i, &packet, sizeof(packet)));
  }
}


uint8_t nodes_online()
{
  uint8_t c = 0;
  for (uint8_t i = 0; i < MAX_NODES; ++i)
  {
    if (nodes[i].lastUpdate > 0)
      ++c;
  }
  return c + 1;
}


const uint8_t brightness = 31;
void led(const uint32_t colour)
{
  analogWrite(PIN_LED_R, (colour >> 16) & brightness);
  analogWrite(PIN_LED_G, (colour >> 8) & brightness);
  analogWrite(PIN_LED_B, (colour >> 0) & brightness);
}

#ifdef PDQ_LIBS
#include "PDQ_ST7735_config.h"
#include <PDQ_GFX.h>
#include <PDQ_ST7735.h>
PDQ_ST7735 tft = PDQ_ST7735();
#else
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, PIN_SPI_RESET);
#endif

typedef void(*callback_t)();
class Screen;

class Component
{
public:
  Component() : redraw(true)
  {
  }

  virtual void render(bool focus = false) = 0;

  inline virtual bool rerender() const final { return redraw; }
  inline virtual void  markRerender() final { redraw = true; }

  inline virtual bool selectable() const { return false; }

protected:
  bool redraw;
};

void cb_go_screen(Screen* screen);
class Button : public Component
{
public:
  static inline Button gotoScreen(int8_t _x, int8_t _y, int8_t w, int8_t h, const char* str, Screen* _screen)
  {
    Button button = Button(_x, _y, w, h, str);
    button.screen = _screen;
    return button;
  }
  Button(int8_t _x, int8_t _y, int8_t w, int8_t h, const char* str, callback_t cb = nullptr) : Component(), x(_x), y(_y), width(w), height(h), label(str), callback(cb)
  {
  }

  virtual void render(bool focus = false)
  {
    const colour_t BACKGROUND_COLOUR = colour(15, 31, 127);
    const colour_t BORDER_COLOUR = colour(63, 127, 255);
    const colour_t FOCUS_COLOUR = COLOUR_WHITE;
    const colour_t LABEL_COLOUR = COLOUR_WHITE;

    int16_t x1, y1;
    uint16_t x2, y2;

    tft.drawRect(x, y, width, height, focus ? FOCUS_COLOUR : BORDER_COLOUR);
    tft.fillRect(x+1, y+1, width-2, height-2, BACKGROUND_COLOUR);
    tft.getTextBounds(const_cast<char*>(label), 0, 0, &x1, &y1, &x2, &y2);
    tft.setCursor(x + (width - x2) / 2, y + (height - y2) / 2);
    tft.setTextColor(LABEL_COLOUR);
    tft.setTextSize(1);
    tft.print(label);

    redraw = false;
  }

  inline virtual bool selectable() const { return screen || callback; }

  virtual void select()
  {
    screen ? cb_go_screen(screen) : callback();
  }

protected:
  uint8_t x, y, width, height;
  const char* label;
  callback_t callback;
  Screen* screen;
};


class Toggle : public Component
{
public:
  Toggle(int8_t _x, int8_t _y, int8_t w, int8_t h, const char* str) : Component(), x(_x), y(_y), width(w), height(h), label(str), state(false)
  {
  }

  virtual void render(bool focus = false)
  {
    const colour_t BACKGROUND_COLOUR = colour(15, 31, 127);
    const colour_t SELECT_COLOUR = colour(31, 63, 127);
    const colour_t BORDER_COLOUR = colour(63, 127, 255);
    const colour_t FOCUS_COLOUR = COLOUR_WHITE;
    const colour_t LABEL_COLOUR = COLOUR_WHITE;

    int16_t x1, y1;
    uint16_t w, h;

    tft.drawRect(x, y, width, height, focus ? FOCUS_COLOUR : BORDER_COLOUR);
    tft.fillRect(x+1, y+1, width-2, height-2, state ? SELECT_COLOUR : BACKGROUND_COLOUR);
    tft.getTextBounds(const_cast<char*>(label), 0, 0, &x1, &y1, &w, &h);
    tft.setCursor(x + (width - w) / 2, y + (height - h) / 2);
    tft.setTextColor(LABEL_COLOUR);
    tft.setTextSize(1);
    tft.print(label);

    redraw = false;
  }


  inline virtual bool selectable() const { return true; }

  virtual void select()
  {
    state = !state;
    redraw = true;
  }
protected:
  uint8_t x, y, width, height;
  const char* label;
  bool state;
};


class Label : public Component
{
public:
  enum class Alignment {
    CENTER,
    LEFT
  };

  Label(int8_t _x, int8_t _y, int8_t w, int8_t h, const char* str) : Component(), x(_x), y(_y), width(w), height(h),
  background(COLOUR_BLACK), border(colour(63, 127, 255)), textColour(COLOUR_WHITE), alignment(Alignment::CENTER), label(str)
  {

  }

  virtual void render(bool focus = false)
  {
    (void)focus;

    int16_t x1, y1;
    uint16_t w, h;

    tft.drawRect(x, y, width, height, border);
    tft.fillRect(x+1, y+1, width-2, height-2, background);
    tft.setTextColor(textColour);
    tft.setTextSize(1);
    tft.setTextWrap(false);

    if (alignment == Alignment::CENTER)
    {
      tft.getTextBounds(const_cast<char*>(label), 160 - width, 0, &x1, &y1, &w, &h);
      tft.setCursor(x + (width - w) / 2, y + (height - h) / 2);
    }
    else
    {
      tft.getTextBounds(const_cast<char*>(label), 160 - width, 0, &x1, &y1, &w, &h);
      tft.setCursor(x + 2, y + (height - h) / 2);
    }

    #ifdef LABEL_LINE_WRAP
    if (w > width)
    {
      size_t pos = 0;
      size_t prev = 0;
      size_t offset = 0;
      uint8_t lines = 0;

      while (pos < strlen(label))
      {
        while (pos < strlen(label) && label[pos] != ' ')
          ++pos;
        char line[pos - offset + 1];
        strncpy(line, &(label[offset]), pos - offset + 1);

        if (alignment == Alignment::CENTER)
          tft.getTextBounds(const_cast<char*>(line), 160-width, 0, &x1, &y1, &w, &h);
        else
          tft.getTextBounds(const_cast<char*>(line), 160-(width + 4), 0, &x1, &y1, &w, &h);

        if (w >= width || pos == strlen(label))
        {
          // if we're here, time to write some text
          if (pos < strlen(label))
          {
            memset(line, 0, pos - offset + 1);
            strncpy(line, &(label[offset]), prev - offset + 1);
          }

          if (alignment == Alignment::CENTER)
            tft.setCursor(x + (width - w) / 2, y + lines * h + 2);
          else
            tft.setCursor(x + 4, y + lines * h + 4);
          tft.print(line);
          ++lines;
          offset = prev + 1;
          if (pos < strlen(label))
            pos = offset;
          continue;
        }
        prev = pos;
        ++pos; // skip the space
      }
    }
    else
    #endif
    {
      tft.print(label);
    }

    redraw = false;
  }

  void setBackgroundColour(const colour_t c)
  {
    background = c;
    redraw = true;
  }

  void setBorderColour(const colour_t c)
  {
    border = c;
    redraw = true;
  }

  void setTextColour(const colour_t c)
  {
    textColour = c;
    redraw = true;
  }

  void setAlignment(const Alignment a)
  {
    alignment = a;
    redraw = true;
  }

private:
  uint8_t x, y, width, height;
  colour_t background;
  colour_t border;
  colour_t textColour;
  Alignment alignment;
  const char* label;
};


class Text : public Component
{
public:
  enum class Alignment {
    CENTER,
    LEFT
  };


  Text(int8_t _x, int8_t _y, int8_t w, int8_t h, uint8_t sz, const char* str = nullptr) : Component(), x(_x), y(_y), width(w), height(h),
  background(COLOUR_BLACK), border(colour(63, 127, 255)), textColour(COLOUR_WHITE), alignment(Alignment::CENTER)
  {
    length = sz;
    label = static_cast<char*>(malloc(length));
    if (str)
      strncpy(label, str, length);
  }

  virtual void render(bool focus = false)
  {
    (void)focus;

    int16_t x1, y1;
    uint16_t w, h;

    tft.drawRect(x, y, width, height, border);
    tft.fillRect(x+1, y+1, width-2, height-2, background);
    tft.setTextColor(textColour);
    tft.setTextSize(1);
    tft.setTextWrap(false);

    if (alignment == Alignment::CENTER)
    {
      tft.getTextBounds(const_cast<char*>(label), 160 - width, 0, &x1, &y1, &w, &h);
      tft.setCursor(x + (width - w) / 2, y + (height - h) / 2);
    }
    else
    {
      tft.getTextBounds(const_cast<char*>(label), 160 - width + 4, 0, &x1, &y1, &w, &h);
      tft.setCursor(x + 2, y + (height - h) / 2);
    }

    #ifdef LABEL_LINE_WRAP
    if (w > width)
    {
      size_t pos = 0;
      size_t prev = 0;
      size_t offset = 0;
      uint8_t lines = 0;

      while (pos < strlen(label))
      {
        while (pos < strlen(label) && label[pos] != ' ')
          ++pos;
        char line[pos - offset + 1];
        strncpy(line, &(label[offset]), pos - offset + 1);

        if (alignment == Alignment::CENTER)
          tft.getTextBounds(const_cast<char*>(line), 160-width, 0, &x1, &y1, &w, &h);
        else
          tft.getTextBounds(const_cast<char*>(line), 160-(width + 4), 0, &x1, &y1, &w, &h);

        if (w >= width || pos == strlen(label))
        {
          // if we're here, time to write some text
          if (pos < strlen(label))
          {
            memset(line, 0, pos - offset + 1);
            strncpy(line, &(label[offset]), prev - offset + 1);
          }

          if (alignment == Alignment::CENTER)
            tft.setCursor(x + (width - w) / 2, y + lines * h + 2);
          else
            tft.setCursor(x + 4, y + lines * h + 4);
          tft.print(line);
          ++lines;
          offset = prev + 1;
          if (pos < strlen(label))
            pos = offset;
          continue;
        }
        prev = pos;
        ++pos; // skip the space
      }
    }
    else
    #endif
    {
      tft.print(label);
    }

    redraw = false;
  }

  void setBackgroundColour(const colour_t c)
  {
    background = c;
    redraw = true;
  }

  void setBorderColour(const colour_t c)
  {
    border = c;
    redraw = true;
  }

  void setTextColour(const colour_t c)
  {
    textColour = c;
    redraw = true;
  }

  void setLabel(const char* txt)
  {
    strncpy(label, txt, length);
    redraw = true;
  }

  void setLabel(const uint8_t n)
  {
    snprintf(label, length, "%3d", n);
    redraw = true;
  }

  void setAlignment(const Alignment a)
  {
    alignment = a;
    redraw = true;
  }

private:
  uint8_t x, y, width, height;
  uint8_t length;
  colour_t background;
  colour_t border;
  colour_t textColour;
  Alignment alignment;
  char* label;
};

class Screen
{
public:
  Screen() : redraw(true), focusIndex(0) {}

  virtual void render()
  {
    if (redraw)
    {
      const colour_t BACKGROUND_COLOUR = COLOUR_BLACK;
      tft.fillScreen(BACKGROUND_COLOUR);
    }

    Component* const* components = getComponents();
    for (uint8_t i = 0; i < getComponentCount(); ++i)
    {
      if (redraw || components[i]->rerender())
        components[i]->render(i == focusIndex);
    }

    redraw = false;
  }

  inline bool rerender() const { return redraw; }
  inline void  markRerender() { redraw = true; }

  virtual void next()
  {
    Component* const* components = getComponents();

    components[focusIndex]->markRerender();
    int8_t max = getComponentCount();
    do {
      focusIndex = (focusIndex + 1) % getComponentCount();
    }
    while (components[focusIndex]->selectable() == false && --max > 0);
    components[focusIndex]->markRerender();
  }

  virtual void prev()
  {
    Component* const* components = getComponents();

    components[focusIndex]->markRerender();
    int8_t max = getComponentCount();
    do {
      focusIndex = (focusIndex - 1 + getComponentCount()) % getComponentCount();
    }
    while (components[focusIndex]->selectable() == false && --max > 0);
    components[focusIndex]->markRerender();
  }

  virtual void select()
  {
    Component* const* components = getComponents();

    Component* component = components[focusIndex];

    if (component->selectable() == false)
      return;

    Button* button = static_cast<Button*>(component);
    button->select();
  }

  virtual void idle() {};

protected:
  virtual uint8_t getComponentCount() const;
  virtual Component* const* getComponents() const;

  bool redraw;
  uint8_t focusIndex;
};

template <uint8_t TComponentCount>
class ScreenCommon : public Screen
{
public:
  ScreenCommon() : Screen()
  {
  }

protected:
  virtual uint8_t getComponentCount() const { return TComponentCount; }
  virtual Component* const* getComponents() const { return components; }
  Component* components[TComponentCount];
};

void cb_make_node();
void cb_play_classic();
void cb_go_back();


void cb_nodes_up();
void cb_nodes_down();

class ScreenGameSetup : public ScreenCommon<6>
{
public:
  ScreenGameSetup() : ScreenCommon(),
  nodesLabel(Label(16, 16, 64, 16, "Nodes")),
  nodes(Text(80, 16, 32, 16, 4)),
  nodesUp(Button(112, 16, 16, 16, "+", cb_nodes_up)),
  nodesDown(Button(128, 16, 16, 16, "-", cb_nodes_down)),

  classic(Button(16, 40, 128, 20, "Classic", cb_play_classic)),
  back(Button(16, 100, 128, 20, "Back", cb_go_back))
  {
    components[0] = &classic;
    components[1] = &nodesLabel;
    components[2] = &nodes;
    components[3] = &nodesUp;
    components[4] = &nodesDown;
    components[5] = &back;

    nodesLabel.setAlignment(Label::Alignment::LEFT);
    nodes.setLabel(config::getNodeCount());
  }

  virtual void idle()
  {
    network();
  }

  void setNodeCount(uint8_t count)
  {
    if (count == config::getNodeCount())
      return;

    config::setNodeCount(count);
    nodes.setLabel(count);
    nodes.setTextColour(nodes_online() == count ? COLOUR_GREEN : COLOUR_RED);
  }

private:
  Label nodesLabel;
  Text nodes;
  Button nodesUp;
  Button nodesDown;

  Button classic;
  Button back;
};

class ScreenGameplay : public ScreenCommon<2>
{
public:
  ScreenGameplay() : ScreenCommon(),
  status(Text(8, 8, tft.height() - 16, 20, 20)),
  nodeCount(Text(8, 36, tft.height() - 16, 20, 4))
  {
    components[0] = &status;
    components[1] = &nodeCount;

    nodeCount.setLabel("0");
  }

  virtual void idle()
  {
    // process any network message
    network();

    static uint8_t lastCount = 255;
    const uint8_t count = nodes_online();

    // game not started yet
    if (game.start == 0)
    {
      status.setLabel("Waiting");
      if (count != lastCount)
      {
        nodeCount.setLabel(count);
        lastCount = count;
      }
    }
    // game running
    else if (game.end == 0)
    {
      status.setLabel("In progress");
      if (count != lastCount)
      {
        nodeCount.setLabel(count);
        lastCount = count;
      }
    }
    else if (game.end > game.start)
    {
      status.setLabel("Game over");
      nodeCount.setLabel(nodes[config::getRadioID()].team);
    }
  }

private:
  Text status;
  Text nodeCount;
};
ScreenGameplay play;


void cb_exit_scanner()
{
  radio_init();

  cb_go_back();
}

class ScreenScanner : public ScreenCommon<1>
{
public:
  ScreenScanner() : ScreenCommon(),
  back(Button(108, 4, 16, 16, "X", cb_exit_scanner)),
  scan{0},
  channel(0)
  {
    components[0] = &back;
  }

  virtual void render()
  {
    ScreenCommon<1>::render();

    tft.drawLine(1, GRAPH_BASE, 126, GRAPH_BASE, COLOUR_WHITE);
    for (uint8_t i = 1; i <= 126; i += 25)
      tft.drawLine(i, GRAPH_BASE, i, GRAPH_BASE + 6, COLOUR_WHITE);
    for (uint8_t i = 1; i <= 126; i += 5)
      tft.drawLine(i, GRAPH_BASE, i, GRAPH_BASE + 4, COLOUR_WHITE);
  }

  virtual void idle()
  {
    const uint8_t old = scan[channel];

    scan[channel] = radio.scanChannel(channel, 10);

    if (old > scan[channel])
      tft.drawLine(channel + 1, GRAPH_BASE - scan[channel] - 1, channel + 1, GRAPH_BASE - old - 1, COLOUR_BLACK);
    tft.drawLine(channel + 1, GRAPH_BASE - scan[channel] - 1, channel + 1, GRAPH_BASE - 1, COLOUR_YELLOW);

    if (++channel > 125)
      channel = 0;
  }

private:
  Button back;
  uint8_t scan[125];
  uint8_t channel;

  static const uint8_t GRAPH_BASE = 112;
};


void cb_channel_up();
void cb_channel_down();
void cb_radio_id_up();
void cb_radio_id_down();
void cb_radio_save();
void cb_radio_cancel();
class ScreenRadio : public ScreenCommon<10>
{
public:
  ScreenRadio() : ScreenCommon(),
  back(Button(140, 4, 16, 16, "X", cb_radio_cancel)),
  channelLabel(Label(16, 32, 64, 16, "Channel")),
  channel(Text(80, 32, 32, 16, 4)),
  channelUp(Button(112, 32, 16, 16, "+", cb_channel_up)),
  channelDown(Button(128, 32, 16, 16, "-", cb_channel_down)),
  radioLabel(Label(16, 64, 64, 16, "Radio ID")),
  radioID(Text(80, 64, 32, 16, 4)),
  radioIDUp(Button(112, 64, 16, 16, "+", cb_radio_id_up)),
  radioIDDown(Button(128, 64, 16, 16, "-", cb_radio_id_down)),
  save(Button(16, 100, 128, 20, "Save", cb_radio_save)),
  newChannel(config::getChannel()),
  newRadioID(config::getRadioID())
  {
    components[0] = &back;
    components[1] = &channelLabel;
    components[2] = &channel;
    components[3] = &channelUp;
    components[4] = &channelDown;
    components[5] = &radioLabel;
    components[6] = &radioID;
    components[7] = &radioIDUp;
    components[8] = &radioIDDown;
    components[9] = &save;

    channelLabel.setAlignment(Label::Alignment::LEFT);
    radioLabel.setAlignment(Label::Alignment::LEFT);

    channel.setLabel(newChannel);
    radioID.setLabel(newRadioID);
  }

  inline channel_t getChannel() const { return newChannel; }
  inline void setChannel(const channel_t ch)
  {
    newChannel = ch;
    channel.setLabel(newChannel);
  }

  inline radio_id getRadioID() const { return newRadioID; }
  inline void setRadioID(const radio_id id)
  {
    newRadioID = id;
    radioID.setLabel(newRadioID);
  }

private:
  Button back;
  Label channelLabel;
  Text channel;
  Button channelUp;
  Button channelDown;
  Label radioLabel;
  Text radioID;
  Button radioIDUp;
  Button radioIDDown;
  Button save;
  channel_t newChannel;
  radio_id newRadioID;
};


ScreenRadio screenRadio;
class ScreenConfig : public ScreenCommon<2>
{
public:
  ScreenConfig() : ScreenCommon(),
  radios(Button::gotoScreen(16, 8, tft.height() - 32, 20, "Configure Radios", &screenRadio)),
  // master(Button(16, 36, tft.height() - 32, 20, "Master Tag", cb_go_mastertag)),
  back(Button(16, 100, tft.height() - 32, 20, "Back", cb_go_back))
  {
    components[0] = &radios;
    // components[1] = &master;
    components[1] = &back;
  }

private:
  Button radios;
  // Button master;
  Button back;
};


/*
class ScreenMaster : public ScreenCommon<2>
{
public:
  ScreenMaster() : ScreenCommon(),
  back(Button(16, 100, tft.height() - 32, 20, "Back", cb_go_back)),
  text(Label(16, 8, tft.height() - 32, 84, "Scan your tag to create a master tag. This can be used to authenticate with other nodes to perform master level operations."))
  {
    text.setAlignment(Label::Alignment::LEFT);
    components[0] = &back;
    components[1] = &text;
  }

private:
  Button back;
  Label text;
};
*/
// ScreenMaster screenMaster;


ScreenGameSetup gamesetup;
ScreenScanner scanner;
ScreenConfig screenConfig;
class ScreenHome : public ScreenCommon<4>
{
public:
  ScreenHome() : ScreenCommon(),
  node(Button(16, 8, tft.height() - 32, 20, "Node", cb_make_node)),
  games(Button::gotoScreen(16, 36, tft.height() - 32, 20, "Games", &gamesetup)),
  channel(Button::gotoScreen(16, 64, tft.height() - 32, 20, "Channel Scanner", &scanner)),
  radios(Button::gotoScreen(16, 92, tft.height() - 32, 20, "Configure", &screenConfig))
  {
    components[0] = &node;
    components[1] = &games;
    components[2] = &channel;
    components[3] = &radios;
  }

private:
  Button node;
  Button games;
  Button channel;
  Button radios;
};


const uint8_t MAX_DEPTH = 3;
ScreenHome home;
Screen* screenStack[MAX_DEPTH] = {&home};
uint8_t screenIndex = 0;

inline void cb_go_screen(Screen* screen)
{
  screenStack[++screenIndex] = screen;
  screenStack[screenIndex]->markRerender();
}

void cb_make_node()
{
  screenStack[++screenIndex] = &play;
  screenStack[screenIndex]->markRerender();
}

void cb_nodes_up()
{
  uint8_t count = config::getNodeCount();
  count = (count + 1) % MAX_NODES;
  gamesetup.setNodeCount(count);
}
void cb_nodes_down()
{
  uint8_t count = config::getNodeCount();
  count = (count + MAX_NODES - 1) % MAX_NODES;
  gamesetup.setNodeCount(count);
}

void cb_play_classic()
{
  // start the game
  Packet game = {
    .opcode = OpCode::GAME_SETUP,
    .source = config::getRadioID(),
    .timestamp = millis(),
  };
  game.nodes = config::getNodeCount();
  game.teams = 2;

  broadcast(game);

  screenStack[++screenIndex] = &play;
  screenStack[screenIndex]->markRerender();
}

void cb_channel_up()
{
  screenRadio.setChannel((screenRadio.getChannel() + 1) % (NRFLite::MAX_NRF_CHANNEL + 1));
}

void cb_channel_down()
{
  screenRadio.setChannel((screenRadio.getChannel() + NRFLite::MAX_NRF_CHANNEL - 1) % (NRFLite::MAX_NRF_CHANNEL + 1));
}

void cb_radio_id_up()
{
  screenRadio.setRadioID((screenRadio.getRadioID() + 1) % MAX_NODES);
}

void cb_radio_id_down()
{
  screenRadio.setRadioID((screenRadio.getRadioID() + MAX_NODES-1) % MAX_NODES);
}

void cb_radio_save()
{
  config::setChannel(screenRadio.getChannel());
  config::setRadioID(screenRadio.getRadioID());

  radio_init();

  cb_go_back();
}

void cb_radio_cancel()
{
  screenRadio.setChannel(config::getChannel());
  screenRadio.setRadioID(config::getRadioID());

  cb_go_back();
}


void cb_go_back()
{
  --screenIndex;
  screenStack[screenIndex]->markRerender();
}

bool nfcEnabled()
{
  if (screenStack[screenIndex] == &screenRadio)
    return true;
  return false;
}



uint32_t last = 0;
void gui_update()
{
  uint32_t now = millis();
  last = now;
  Screen* screen = screenStack[screenIndex];

  // update pins
  pinNext.update();
  pinPrev.update();
  pinSelect.update();

  if (nfcEnabled())
  {
    uint8_t buffer[RFID_BUFFER_LENGTH];
    uint8_t size = sizeof(buffer);

    if (mfrc522.PICC_IsNewCardPresent() &&
      mfrc522.PICC_ReadCardSerial() &&
      mfrc522.PICC_GetType(mfrc522.uid.sak) == MFRC522::PICC_TYPE_MIFARE_1K &&
      mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, TRAILER, &key, &(mfrc522.uid)) == MFRC522::STATUS_OK &&
      mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_B, TRAILER, &key, &(mfrc522.uid)) == MFRC522::STATUS_OK &&
      mfrc522.MIFARE_Read(BLOCK, buffer, &size) == MFRC522::STATUS_OK)
    {
      if (screenStack[screenIndex] == &screenRadio)
      {
        // copy the pre shared key
        uint8_t data[16] = {0};
        memcpy(data, PSK, 8);
        data[9] = screenRadio.getChannel();
        data[10] = 0;
        MFRC522::StatusCode status = mfrc522.MIFARE_Write(BLOCK, data, 16);
        if (status == MFRC522::STATUS_OK)
        {
          cb_go_back();
        }
      }
    }
    mfrc522.PICC_HaltA();
    mfrc522.PCD_StopCrypto1();
  }

  // process actions
  if (pinNext.is_released())
  {
    screen->next();
    #ifdef KEY_TONES
    beep(PIN_SPEAKER, Note::C5, 200);
    #endif
  }
  else if (pinPrev.is_released())
  {
    screen->prev();
    #ifdef KEY_TONES
    beep(PIN_SPEAKER, Note::C5, 200);
    #endif
  }
  else if (pinSelect.is_released())
  {
    screen->select();
    #ifdef KEY_TONES
    beep(PIN_SPEAKER, Note::E5, 200);
    #endif
  }

  // run any idle actions
  screen->idle();

  // redraw whatever is needed
  screen->render();
}


void reset()
{
  for (uint8_t i = 0; i < MAX_NODES; ++i)
  {
    nodes[i].lastUpdate = 0;
    nodes[i].lastPing = 0;

    // clear the graph
    // for (uint8_t j = 0; j < MAX_NODES; ++j)
    //   graph[i][j] = 0;
  }
}


void setup()
{
  pinMode(PIN_NEXT, INPUT);
  pinMode(PIN_PREV, INPUT);
  pinMode(PIN_SELECT, INPUT);

  SPI.begin();

  // gui init
  #ifdef PDQ_LIBS
  tft.initR(ST7735_INITR_BLACKTAB);
  #else
  tft.initR(INITR_BLACKTAB);
  #endif
  tft.setRotation(1);
  tft.fillScreen(COLOUR_BLACK);

  Serial.begin(115200);
  Serial.println(config::getChannel());
  Serial.println(config::getRadioID());

  if (!radio_init())
  {
    Label error(16, 54, tft.height() - 16, 20, "RADIO FAILED");
    error.setBackgroundColour(COLOUR_BLACK);
    error.setBorderColour(COLOUR_RED);
    error.setTextColour(COLOUR_RED);

    error.render();
    while (true)
    {
    }
  }

  // nfc init
  mfrc522.PCD_Init();
  for (uint8_t i = 0; i < 6; ++i)
    key.keyByte[i] = 0xFF;

  reset();
}

void loop()
{
  gui_update();
}
