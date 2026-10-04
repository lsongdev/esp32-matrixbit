#include "features.h"
#include <stddef.h>
#include <matrixbit.h>
#include <driver/gpio.h>

namespace demo {
namespace {
// Non-strapping outputs not used by the board API. These are GPIO numbers,
// not edge-connector P labels; exposed contacts depend on the board revision.
constexpr uint8_t pins[] = {18, 19, 21, 25, 26, 32, 33};
constexpr size_t pinCount = sizeof(pins) / sizeof(pins[0]);
size_t selected = 4; // GPIO26 (the compatible mPython P8 mapping).
bool high = false;

gpio_num_t pin() { return static_cast<gpio_num_t>(pins[selected]); }
void release() {
  gpio_set_level(pin(), 0);
  gpio_set_direction(pin(), GPIO_MODE_INPUT);
  high = false;
}
void claim() {
  // Set the output latch before enabling the driver to avoid a HIGH glitch.
  gpio_set_level(pin(), 0);
  gpio_set_direction(pin(), GPIO_MODE_OUTPUT);
  high = false;
}
void enter() { selected = 4; claim(); }
void input(Input event) {
  if (event == Input::Activate) {
    high = !high;
    gpio_set_level(pin(), high ? 1 : 0);
  } else {
    release();
    selected = event == Input::Next ? (selected + 1) % pinCount : (selected + pinCount - 1) % pinCount;
    claim();
  }
  Serial.printf("[IO] GPIO%u %s\n", pins[selected], high ? "HIGH" : "LOW");
}
void draw(Adafruit_SSD1306 &screen) {
  screen.printf("GPIO%u: %s\n", pins[selected], high ? "HIGH" : "LOW");
  screen.print("A: next pin\nHold A: previous pin\nB: toggle HIGH / LOW\nExit: LOW then input");
}
void exit() { release(); }
}
const Feature ioFeature{"IO control", enter, nullptr, draw, input, exit};
} // namespace demo
