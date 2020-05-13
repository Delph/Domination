#include <SPI.h>
#include <NRFLite.h>

#include <Adafruit_GFX.h>
#include <XTronical_ST7735.h> // hardware-specific library for the display, replacing the Adafruit one

#include <MFRC522.h>

#include "debounce.h"

const uint8_t PIN_RADIO_CE = 8;
const uint8_t PIN_RADIO_CSN = 7;
const uint8_t PIN_LED = 4;

const uint8_t RADIO_ID = 0;
const uint8_t CHANNEL = 0;

NRFLite radio;

const uint8_t PIN_RFID_SELECT = 10;
const uint8_t PIN_RFID_RESET = 9;

MFRC522 mfrc522(PIN_RFID_SELECT, PIN_RFID_RESET);
MFRC522::MIFARE_Key key;


const uint8_t PIN_NEXT = 6;
const uint8_t PIN_PREV = 5;
const uint8_t PIN_SELECT = A3;
Debounce pinNext(6);
Debounce pinPrev(5);
Debounce pinSelect(A3);


using colour_t = uint16_t;
const uint8_t R_MASK = 0b11111;
const uint8_t G_MASK = 0b111111;
const uint8_t B_MASK = 0b11111;
const uint8_t R_BITS = 5;
const uint8_t G_BITS = 6;
const uint8_t B_BITS = 5;

// constexpr colour_t colour(const float r, const float g, const float b)
// {
//   return static_cast<colour_t>(static_cast<uint8_t>(r * R_MASK) & R_MASK) << 11 |
//     static_cast<colour_t>(static_cast<uint8_t>(g * G_MASK) & G_MASK) << 5 |
//     static_cast<colour_t>(static_cast<uint8_t>(b * B_MASK) & B_MASK
//   );
// }

constexpr colour_t colour(const uint8_t r, const uint8_t g, const uint8_t b)
{
  return static_cast<colour_t>(static_cast<uint8_t>(r >> (8 - R_BITS)) << 11) |
    static_cast<colour_t>(static_cast<uint8_t>(g >> (8 - G_BITS)) << 5) |
    static_cast<colour_t>(static_cast<uint8_t>(b >> (8 - B_BITS)));
}
constexpr colour_t COLOUR_BLACK   = 0x0000;
constexpr colour_t COLOUR_BLUE    = 0x001F;
constexpr colour_t COLOUR_RED     = 0xF800;
constexpr colour_t COLOUR_GREEN   = 0x07E0;
constexpr colour_t COLOUR_CYAN    = 0x07FF;
constexpr colour_t COLOUR_MAGENTA = 0xF81F;
constexpr colour_t COLOUR_YELLOW  = 0xFFE0;
constexpr colour_t COLOUR_WHITE   = 0xFFFF;



const uint8_t TFT_CS = A1;
const uint8_t TFT_RST = 9;
const uint8_t TFT_DC = A0;

Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);

typedef void(*callback_t)();

class Component
{
public:
  Component() : redraw(true)
  {
  }

  virtual void render(bool focus = false) = 0;

  inline virtual bool rerender() const final { return redraw; }

  inline virtual bool selectable() const { return false; }

protected:
  bool redraw;
};


class Button : public Component
{
public:
  Button(int8_t _x, int8_t _y, int8_t w, int8_t h, char* str, callback_t cb = nullptr) : Component(), x(_x), y(_y), width(w), height(h), label(str), callback(cb)
  {

  }

  virtual void render(bool focus)
  {
    const colour_t BACKGROUND_COLOUR = colour(15, 31, 127);
    const colour_t BORDER_COLOUR = colour(63, 127, 255);
    const colour_t FOCUS_COLOUR = COLOUR_WHITE;
    const colour_t LABEL_COLOUR = COLOUR_WHITE;

    int16_t x1, y1;
    uint16_t x2, y2;

    tft.drawRect(x, y, width, height, focus ? FOCUS_COLOUR : BORDER_COLOUR);
    tft.fillRect(x+1, y+1, width-2, height-2, BACKGROUND_COLOUR);
    tft.getTextBounds(label, 0, 0, &x1, &y1, &x2, &y2);
    tft.setCursor(x + (width - x2) / 2, y + (height - y2) / 2);
    tft.setTextColor(LABEL_COLOUR);
    tft.setTextSize(1);
    tft.print(label);

    redraw = false;
  }

  inline virtual bool selectable() const { return callback; }

  virtual void select()
  {
    callback();
  }

private:
  uint8_t x, y, width, height;
  char* label;
  callback_t callback;
};

template <uint8_t TBuffSize>
class Label : public Component
{
public:
  Label(int8_t _x, int8_t _y, int8_t w, int8_t h, char* str = nullptr) : Component(), x(_x), y(_y), width(w), height(h),
  background(COLOUR_BLACK), border(colour(63, 127, 255)), textColour(COLOUR_WHITE)
  {
    if (str)
      strncpy(label, str, TBuffSize);
  }

  virtual void render(bool focus = false)
  {
    int16_t x1, y1;
    uint16_t x2, y2;

    tft.drawRect(x, y, width, height, border);
    tft.fillRect(x+1, y+1, width-2, height-2, background);
    tft.getTextBounds(label, 0, 0, &x1, &y1, &x2, &y2);
    tft.setCursor(x + (width - x2) / 2, y + (height - y2) / 2);
    tft.setTextColor(textColour);
    tft.setTextSize(1);
    tft.print(label);

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
    strncpy(label, txt, TBuffSize);
    redraw = true;
  }

  void setLabel(const uint8_t n)
  {
    snprintf(label, TBuffSize, "%3d", n);
    redraw = true;
  }

private:
  uint8_t x, y, width, height;
  colour_t background;
  colour_t border;
  colour_t textColour;
  char label[TBuffSize];
};

class Screen
{
public:
  virtual void render() = 0;

  inline virtual bool rerender() const = 0;

  virtual void next() = 0;
  virtual void prev() = 0;
  virtual void select() = 0;
  virtual void idle() {};
};

template <uint8_t TComponentCount>
class ScreenCommon : public Screen
{
public:
  ScreenCommon() : Screen(), redraw(true), focusIndex(0)
  {
  }

  virtual void render()
  {
    if (redraw)
    {
      const colour_t BACKGROUND_COLOUR = COLOUR_BLACK;
      tft.fillScreen(BACKGROUND_COLOUR);
    }

    for (uint8_t i = 0; i < TComponentCount; ++i)
    {
      if (redraw || components[i]->rerender())
        components[i]->render(i == focusIndex);
    }

    redraw = false;
  }

  inline virtual bool rerender() const final { return redraw; }


  virtual void next()
  {
    focusIndex = (focusIndex + 1) % TComponentCount;

    redraw = true;
  }

  virtual void prev()
  {
    focusIndex = (focusIndex - 1 + TComponentCount) % TComponentCount;

    redraw = true;
  }

  virtual void select()
  {
    Component* component = components[focusIndex];

    if (component->selectable() == false)
      return;

    Button* button = static_cast<Button*>(component);
    button->select();
  }


protected:
  bool redraw;
  uint8_t focusIndex;
  Component* components[TComponentCount];
};

void cb_go_scanner();
void cb_go_radio();
void cb_go_back();
class ScreenHome : public ScreenCommon<3>
{
public:
  ScreenHome() : ScreenCommon(),
  games(Button(14, 8, 100, 20, "Games")),
  channel(Button(14, 36, 100, 20, "Channel Scanner", cb_go_scanner)),
  radios(Button(14, 64, 100, 20, "Configure Radios", cb_go_radio))
  {
    components[0] = &games;
    components[1] = &channel;
    components[2] = &radios;
  }

private:
  Button games;
  Button channel;
  Button radios;
};

class ScreenScanner : public ScreenCommon<1>
{
public:
  ScreenScanner() : ScreenCommon(),
  back(Button(108, 4, 16, 16, "X", cb_go_back)),
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

    scan[channel] = radio.scanChannel(channel);

//    const uint8_t prev = channel > 0 ? channel - 1 : 124;

//    tft.drawLine(prev + 1, GRAPH_BASE - scan[prev] - 1, prev + 1, GRAPH_BASE - 1, COLOUR_YELLOW);
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

void cb_radio_up();
void cb_radio_down();
class ScreenRadio : public ScreenCommon<4>
{
public:
  ScreenRadio() : ScreenCommon(),
  channel(Label<4>(32, 32, 32, 16)),
  up(Button(64, 32, 16, 16, "^", cb_radio_up)),
  down(Button(80, 32, 16, 16, "v", cb_radio_down)),
  cancel(Button(108, 4, 16, 16, "X", cb_go_back)),
  newChannel(CHANNEL)
  {
    components[0] = &channel;
    components[1] = &up;
    components[2] = &down;
    components[3] = &cancel;

    channel.setLabel(newChannel);
  }

  inline uint8_t getChannel() const { return newChannel; }
  inline void setChannel(const uint8_t ch)
  {
    newChannel = ch;
    channel.setLabel(newChannel);
  }
private:
  Label<4> channel;
  Button up;
  Button down;
  Button cancel;
  uint8_t newChannel;
};

const uint8_t NUM_SCREENS = 2;
ScreenHome home;
ScreenScanner scanner;
ScreenRadio screenRadio;
Screen* screenStack[NUM_SCREENS] = {&home};
uint8_t screenIndex = 0;

void cb_go_scanner()
{
  screenStack[++screenIndex] = &scanner;

  // force a render
  screenStack[screenIndex]->render();
}
void cb_go_radio()
{
  screenStack[++screenIndex] = &screenRadio;

  // force a render
  screenStack[screenIndex]->render();
}

void cb_radio_up()
{
  screenRadio.setChannel((screenRadio.getChannel() + 1) % 125);
}

void cb_radio_down()
{
  screenRadio.setChannel((screenRadio.getChannel() + 124) % 125);
}

void cb_go_back()
{
  --screenIndex;

  // force a render
  screenStack[screenIndex]->render();
}


void gui_update()
{
  Screen* screen = screenStack[screenIndex];

  // update pins
  pinNext.update();
  pinPrev.update();
  pinSelect.update();

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
    // do stuff
  }
  // process actions
  if (pinNext.is_released())
    screen->next();
  else if (pinPrev.is_released())
    screen->prev();
  else if (pinSelect.is_released())
    screen->select();

  // run any idle actions
  screen->idle();

  // redraw whatever is needed
  screen->render();
}


void setup()
{
  Serial.begin(9600);

  pinMode(PIN_NEXT, INPUT);
  pinMode(PIN_PREV, INPUT);
  pinMode(PIN_SELECT, INPUT);

  SPI.begin();

  // gui init
  tft.init();
  tft.setRotation(3);
  tft.fillScreen(COLOUR_BLACK);

  if (!radio.init(RADIO_ID, PIN_RADIO_CE, PIN_RADIO_CSN, NRFLite::BITRATE2MBPS, CHANNEL))
  {
    Label<13> error(16, 54, 96, 20, "RADIO FAILED");
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
}

void loop()
{
  gui_update();
}
