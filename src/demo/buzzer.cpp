#include "features.h"
#include <matrixbit.h>

namespace demo {
namespace {
bool playing = false;
uint32_t startedAt = 0;
void play() {
  matrixbit::startTone(880);
  startedAt = millis(); playing = true;
}
void enter() { matrixbit::beginBuzzer(); play(); }
void update(uint32_t now) {
  if (playing && now - startedAt >= 100) { matrixbit::stopTone(); playing = false; }
}
void input(Input event) { if (event == Input::Activate) play(); }
void draw(Adafruit_SSD1306 &screen) {
  screen.printf("GPIO16 / LEDC0\n880 Hz, 100 ms\n%s\nB: play tone", playing ? "Playing" : "Silent");
}
void exit() { matrixbit::endBuzzer(); playing = false; }
}
const Feature buzzerFeature{"Buzzer", enter, update, draw, input, exit};
} // namespace demo
