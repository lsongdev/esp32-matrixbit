#include "features.h"
#include <matrixbit.h>

namespace demo {
namespace {
uint8_t stage = 0;
uint32_t changedAt = 0;
const char *names[] = {"all-red", "all-green", "all-blue", "LED1-white", "LED2-white", "LED3-white", "all-off"};
void show() {
  auto &pixels = matrixbit::rgb();
  pixels.clear();
  if (stage < 3) {
    const uint32_t colors[] = {pixels.Color(255, 0, 0), pixels.Color(0, 255, 0), pixels.Color(0, 0, 255)};
    pixels.fill(colors[stage]);
  } else if (stage < 6) pixels.setPixelColor(stage - 3, pixels.Color(255, 255, 255));
  pixels.show();
}
void enter() {
  matrixbit::beginRGB();
  stage = 0; changedAt = millis(); show();
}
void update(uint32_t now) {
  if (due(now, changedAt, 900)) { stage = (stage + 1) % 7; show(); }
}
void input(Input event) {
  stage = event == Input::Activate ? 0 : (stage + (event == Input::Next ? 1 : 6)) % 7;
  changedAt = millis(); show();
}
void draw(Adafruit_SSD1306 &screen) {
  screen.printf("%s\n3 LEDs / GPIO17\nA: change color\nB: restart cycle", names[stage]);
}
void exit() { matrixbit::endRGB(); }
}
const Feature rgbFeature{"RGB LEDs", enter, update, draw, input, exit};
} // namespace demo
