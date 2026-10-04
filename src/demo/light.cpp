#include "features.h"
#include <matrixbit.h>

namespace demo {
namespace {
int value = 0;
uint32_t lastRead = 0;
void enter() { value = matrixbit::light(); lastRead = millis(); }
void update(uint32_t now) {
  if (due(now, lastRead, 100)) value = matrixbit::light();
}
void draw(Adafruit_SSD1306 &screen) {
  screen.printf("GPIO39 ADC: %d\n", value);
  screen.print("Range: 0..4095\nCover the light sensor\nRaw ADC, not lux");
}
void exit() { value = 0; }
}
const Feature lightFeature{"Light", enter, update, draw, nullptr, exit};
} // namespace demo
