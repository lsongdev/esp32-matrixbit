#include "features.h"
#include <matrixbit.h>

namespace demo {
namespace {
uint8_t pattern = 0;
bool inverted = false;
void enter() {
  pattern = 0;
  inverted = false;
  matrixbit::display().invertDisplay(false);
}
void input(Input event) {
  if (event == Input::Activate) {
    inverted = !inverted;
    matrixbit::display().invertDisplay(inverted);
  } else {
    pattern = (pattern + (event == Input::Next ? 1 : 2)) % 3;
  }
}
void draw(Adafruit_SSD1306 &screen) {
  if (pattern == 0) {
    screen.print("128 x 64 OLED\nA: change pattern\nB: invert display");
    screen.drawRect(0, 39, 128, 14, SSD1306_WHITE);
    screen.fillRect(2 + (millis() / 100) % 120, 44, 4, 4, SSD1306_WHITE);
  } else if (pattern == 1) {
    screen.fillRect(0, 12, 128, 42, SSD1306_WHITE);
  } else {
    for (int y = 12; y < 54; y += 6)
      for (int x = 0; x < 128; x += 8)
        if ((x / 8 + (y - 12) / 6) % 2 == 0)
          screen.fillRect(x, y, 8, 6, SSD1306_WHITE);
  }
}
void exit() { matrixbit::display().invertDisplay(false); }
}
const Feature displayFeature{"OLED", enter, nullptr, draw, input, exit};
} // namespace demo
