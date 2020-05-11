#include <SPI.h>

#include <Adafruit_GFX.h>
#include <XTronical_ST7735.h> // hardware-specific library for the display, replacing the Adafruit one

const uint8_t PIN_NEXT = 6;
const uint8_t PIN_PREV = 5;
const uint8_t PIN_SELECT = A3;

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

  virtual void render(bool focus) = 0;

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


class Screen
{
public:
  Screen() : redraw(true), focusIndex(0)
  {
  }

  virtual void render() = 0;

  inline virtual bool rerender() const final { return redraw; }


  virtual void next() = 0;
  virtual void prev() = 0;
  virtual void select() = 0;

protected:
  bool redraw;
  size_t focusIndex;
};

void cb_go_scanner();
void cb_go_back();
class ScreenHome : public Screen
{
public:
  ScreenHome() : Screen(),
      games(Button(14, 8, 100, 20, "Games")),
      channel(Button(14, 36, 100, 20, "Channel Scanner", cb_go_scanner)),
      radios(Button(14, 64, 100, 20, "Configure Radios")),
    components{
      &games,
      &channel,
      &radios
    }
  {
  }

  virtual void render()
  {
    const colour_t BACKGROUND_COLOUR = COLOUR_BLACK;
    tft.fillScreen(BACKGROUND_COLOUR);

    for (size_t i = 0; i < NUM_COMPONENTS; ++i)
      components[i]->render(i == focusIndex);

    redraw = false;
  }

  virtual void next()
  {
    focusIndex = (focusIndex + 1) % NUM_COMPONENTS;

    redraw = true;
  }

  virtual void prev()
  {
    focusIndex = (focusIndex - 1 + NUM_COMPONENTS) % NUM_COMPONENTS;

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

private:
  Button games, channel, radios;
  const static size_t NUM_COMPONENTS = 3;
  Component* components[NUM_COMPONENTS];
};

class ScreenScanner : public Screen
{
public:
  ScreenScanner() : Screen(),
  back(Button(108, 4, 16, 16, "X", cb_go_back)),
  components{&back}
  {
  }

  virtual void render()
  {
    const colour_t BACKGROUND_COLOUR = COLOUR_BLACK;
    tft.fillScreen(BACKGROUND_COLOUR);

    for (size_t i = 0; i < NUM_COMPONENTS; ++i)
      components[i]->render(i == focusIndex);

    redraw = false;
  }

  virtual void next()
  {
    focusIndex = (focusIndex + 1) % NUM_COMPONENTS;

    redraw = true;
  }

  virtual void prev()
  {
    focusIndex = (focusIndex - 1 + NUM_COMPONENTS) % NUM_COMPONENTS;

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

private:
  Button back;
  const static size_t NUM_COMPONENTS = 1;
  Component* components[NUM_COMPONENTS];
};

const size_t NUM_SCREENS = 2;
ScreenHome home;
ScreenScanner scanner;
Screen* screenStack[NUM_SCREENS] = {&home};
size_t screenIndex = 0;

void cb_go_scanner()
{
  screenStack[++screenIndex] = &scanner;

  // force a render
  screenStack[screenIndex]->render();
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
  if (digitalRead(PIN_NEXT))
    screen->next();
  else if (digitalRead(PIN_PREV))
    screen->prev();
  else if (digitalRead(PIN_SELECT))
    screen->select();
  // process actions
  // redraw whatever is needed

  if (screen->rerender())
    screen->render();
}


void setup()
{
  // gui init
  tft.init();
  tft.setRotation(3);

  pinMode(PIN_NEXT, INPUT);
  pinMode(PIN_PREV, INPUT);
  pinMode(PIN_SELECT, INPUT);
}

void loop()
{
  gui_update();
}
