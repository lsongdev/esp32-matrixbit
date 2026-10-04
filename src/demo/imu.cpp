#include "features.h"
#include <matrixbit.h>

namespace demo {
namespace {
bool ready = false, valid = false;
uint32_t sampledAt = 0, lastRead = 0;
matrixbit::ImuReading reading;
void enter() {
  valid = false;
  reading = {};
  ready = matrixbit::imu().begin();
  lastRead = millis() - 100;
}
void update(uint32_t now) {
  if (!ready || !due(now, lastRead, 100)) return;
  if (matrixbit::imu().read(reading)) { valid = true; sampledAt = now; }
}
void draw(Adafruit_SSD1306 &screen) {
  if (!ready) { screen.print("IMU unavailable"); return; }
  if (!valid || millis() - sampledAt >= 1500) {
    screen.print("Waiting for IMU data"); return;
  }
  screen.printf("Temp: %.1f C\n", reading.temperature);
  const auto &a = reading.acceleration;
  const auto &g = reading.gyroscope;
  screen.printf("g X%+.2f Y%+.2f\n", a.x, a.y);
  screen.printf("g Z%+.2f\n", a.z);
  screen.printf("gx%+.1f gy%+.1f\n", g.x, g.y);
  screen.printf("gz%+.1f d/s", g.z);
}
void exit() {
  if (!matrixbit::imu().end()) Serial.println("[IMU] failed to disable sensor");
  ready = valid = false;
}
}
const Feature imuFeature{"IMU", enter, update, draw, nullptr, exit};
} // namespace demo
