#include "demo/features.h"
#include <matrixbit.h>
#include <driver/gpio.h>
#include <cassert>
#include <string>

TestSerial Serial;
struct PinState { uint32_t level = 0; gpio_mode_t mode = GPIO_MODE_INPUT; };
PinState pins[40];
int gpio_set_level(gpio_num_t pin, uint32_t level) {
  assert(pin >= 0 && pin < 40);
  pins[pin].level = level;
  return 0;
}
int gpio_set_direction(gpio_num_t pin, gpio_mode_t mode) {
  // Enabling an output must always start LOW, including after changing pins.
  if (mode == GPIO_MODE_OUTPUT) assert(pins[pin].level == 0);
  pins[pin].mode = mode;
  return 0;
}
std::string calls;
void enterA() { calls += "enter A;"; }
void updateA(uint32_t) { calls += "update A;"; }
void inputA(demo::Input) { calls += "input A;"; }
void exitA() { calls += "exit A;"; }
void enterB() { calls += "enter B;"; }
void exitB() { calls += "exit B;"; }
const demo::Feature a{"A", enterA, updateA, nullptr, inputA, exitA};
const demo::Feature b{"B", enterB, nullptr, nullptr, nullptr, exitB};

int main() {
  demo::FeatureSession session;
  session.update(1); session.input(demo::Input::Activate); session.close();
  assert(calls.empty());
  session.open(a); session.update(1); session.input(demo::Input::Activate);
  session.open(b); session.close(); session.close(); session.update(2);
  assert(calls == "enter A;update A;input A;exit A;enter B;exit B;");
  assert(!session.active());
  // Re-entry starts a new session rather than resuming a previous run.
  calls.clear();
  session.open(a); session.open(a); session.close();
  assert(calls == "enter A;exit A;enter A;exit A;");

  // Exercise the real IO feature through the same owner used by the menu.
  session.open(demo::ioFeature);
  assert(pins[26].mode == GPIO_MODE_OUTPUT && pins[26].level == 0);
  session.input(demo::Input::Activate);
  assert(pins[26].level == 1);
  session.input(demo::Input::Next);
  assert(pins[26].mode == GPIO_MODE_INPUT && pins[26].level == 0);
  assert(pins[32].mode == GPIO_MODE_OUTPUT && pins[32].level == 0);
  session.input(demo::Input::Activate);
  assert(pins[32].level == 1);
  session.input(demo::Input::Previous);
  assert(pins[32].mode == GPIO_MODE_INPUT && pins[32].level == 0);
  assert(pins[26].mode == GPIO_MODE_OUTPUT && pins[26].level == 0);
  session.input(demo::Input::Activate);
  session.open(a); // A serial/menu replacement must release HIGH outputs too.
  assert(pins[26].mode == GPIO_MODE_INPUT && pins[26].level == 0);
  session.close();
  session.open(demo::ioFeature);
  session.input(demo::Input::Activate);
  session.close();
  assert(pins[26].mode == GPIO_MODE_INPUT && pins[26].level == 0);
  session.input(demo::Input::Activate);
  assert(pins[26].level == 0);
  session.open(demo::ioFeature);
  // Traverse every allowed pin; never expose flash or board resource pins.
  for (int i = 0; i < 7; ++i) {
    for (int pin : {0, 2, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 22, 23, 27})
      assert(pins[pin].mode == GPIO_MODE_INPUT);
    session.input(demo::Input::Next);
  }
  session.close();
  uint32_t last = UINT32_MAX - 10;
  assert(!demo::due(5, last, 20));
  assert(demo::due(15, last, 20));
}
