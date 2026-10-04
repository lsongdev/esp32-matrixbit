#include "features.h"
#include <matrixbit.h>

namespace demo {
namespace {
uint16_t values[6] = {};
uint32_t lastRead = 0;
void enter() {
  for (auto &value : values) value = 0;
  lastRead = millis() - 100;
}
void update(uint32_t now) {
  if (!due(now, lastRead, 100)) return;
  for (size_t i = 0; i < 6; ++i)
    values[i] = matrixbit::touch(static_cast<matrixbit::TouchPad>(i));
}
void draw(Adafruit_SSD1306 &screen) {
  screen.print("Touch raw values\n");
  constexpr char names[] = "PYTHON";
  for (size_t i = 0; i < 6; ++i)
    screen.printf("%c:%u%s", names[i], values[i], i % 2 == 1 ? "\n" : " ");
  screen.print("Touch pads to compare");
}
void exit() { for (auto &value : values) value = 0; }
}
const Feature touchFeature{"Touch", enter, update, draw, nullptr, exit};
} // namespace demo
