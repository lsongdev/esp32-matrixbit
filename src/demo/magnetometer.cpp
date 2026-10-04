#include "features.h"
#include <matrixbit.h>

namespace demo {
namespace {
bool ready = false, valid = false;
uint32_t sampledAt = 0, lastRead = 0;
matrixbit::Vector3 reading;
void enter() {
  valid = false;
  reading = {};
  ready = matrixbit::magnetometer().begin();
  lastRead = millis() - 100;
}
void update(uint32_t now) {
  if (!ready || !due(now, lastRead, 100)) return;
  if (matrixbit::magnetometer().read(reading)) { valid = true; sampledAt = now; }
}
void draw(Adafruit_SSD1306 &screen) {
  if (!ready) { screen.print("Sensor unavailable"); return; }
  if (!valid || millis() - sampledAt >= 1500) {
    screen.print("Waiting for MAG data"); return;
  }
  screen.printf("%s (uT)\n", matrixbit::magnetometer().name());
  screen.printf("X %+.2f\nY %+.2f\nZ %+.2f\n", reading.x, reading.y, reading.z);
  screen.print("Rotate to check axes");
}
void exit() {
  matrixbit::magnetometer().end();
  ready = valid = false;
}
}
const Feature magnetometerFeature{"Magnetometer", enter, update, draw, nullptr, exit};
} // namespace demo
