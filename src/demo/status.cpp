#include "features.h"
#include <matrixbit.h>

namespace demo {
namespace {
void draw(Adafruit_SSD1306 &screen) {
  screen.printf("Uptime: %lu s\n", static_cast<unsigned long>(millis() / 1000));
  screen.printf("Heap: %u bytes\n", ESP.getFreeHeap());
  screen.printf("CPU: %u MHz\n", ESP.getCpuFreqMHz());
  screen.printf("Flash: %u MB\n", ESP.getFlashChipSize() / (1024 * 1024));
  screen.printf("I2C errors: %lu", static_cast<unsigned long>(matrixbit::i2cErrorCount()));
}
}
const Feature statusFeature{"Status", nullptr, nullptr, draw, nullptr, nullptr};
} // namespace demo
