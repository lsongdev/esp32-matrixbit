#pragma once
#include <stdint.h>
class Adafruit_SSD1306 {
public:
  template <typename... Args> void printf(const char *, Args...) {}
  void print(const char *) {}
};
class TestSerial {
public:
  template <typename... Args> void printf(const char *, Args...) {}
};
extern TestSerial Serial;
