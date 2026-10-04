#include "features.h"
#include <matrixbit.h>

namespace demo {
namespace {
int low = 4095, high = 0, amplitude = 0;
uint32_t lastRead = 0, windowAt = 0;
void enter() {
  low = 4095; high = amplitude = 0;
  lastRead = micros(); windowAt = millis();
}
void update(uint32_t now) {
  const uint32_t us = micros();
  if (due(us, lastRead, 500)) {
    const int value = matrixbit::sound();
    low = min(low, value); high = max(high, value);
  }
  if (due(now, windowAt, 100)) {
    amplitude = high >= low ? high - low : 0;
    low = 4095; high = 0;
  }
}
void draw(Adafruit_SSD1306 &screen) {
  screen.printf("GPIO36 mic p-p: %d\n", amplitude);
  screen.print("Window: 100 ms\nClap to compare\nRaw ADC, not dB");
}
void exit() { low = 4095; high = amplitude = 0; }
}
const Feature soundFeature{"Microphone", enter, update, draw, nullptr, exit};
} // namespace demo
