#pragma once

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_NeoPixel.h>

// Header-only helpers for the classic ESP32 Matrix:bit. Call begin() in setup().
namespace matrixbit {
namespace pin {
constexpr uint8_t button_a = 0, button_b = 2, buzzer = 16, rgb = 17;
constexpr uint8_t i2c_scl = 22, i2c_sda = 23, light = 39, microphone = 36;
} // namespace pin
namespace i2c {
constexpr uint8_t magnetometer = 0x30, display = 0x3c, imu = 0x6b;
} // namespace i2c
constexpr uint8_t rgb_count = 3;
constexpr uint16_t screen_width = 128, screen_height = 64;
enum class TouchPad : uint8_t { P, Y, T, H, O, N };
struct Vector3 { float x = 0, y = 0, z = 0; };
struct ImuReading {
  Vector3 acceleration; // g, including gravity
  Vector3 gyroscope;    // degrees/second
  float temperature = 0; // sensor die temperature, degrees Celsius
};
namespace detail {
inline uint32_t &errors() { static uint32_t value = 0; return value; }
inline bool probe(uint8_t address) {
  Wire.beginTransmission(address);
  return Wire.endTransmission() == 0;
}
inline bool read(uint8_t address, uint8_t reg, uint8_t *data, size_t count) {
  Wire.beginTransmission(address);
  Wire.write(reg);
  if (Wire.endTransmission(false) != 0 || Wire.requestFrom(address, count, true) != count) {
    ++errors();
    while (Wire.available()) Wire.read();
    return false;
  }
  for (size_t i = 0; i < count; ++i) data[i] = Wire.read();
  return true;
}
inline bool write(uint8_t address, uint8_t reg, uint8_t value) {
  Wire.beginTransmission(address);
  Wire.write(reg);
  Wire.write(value);
  if (Wire.endTransmission() == 0) return true;
  ++errors();
  return false;
}
inline int16_t signedLE(const uint8_t *data) {
  return static_cast<int16_t>(static_cast<uint16_t>(data[0]) |
                              (static_cast<uint16_t>(data[1]) << 8));
}
} // namespace detail

class Imu {
public:
  bool begin() {
    ready_ = false;
    id_ = 0;
    if (!detail::probe(i2c::imu) || !detail::read(i2c::imu, 0x00, &id_, 1) || id_ != 0x05) return false;
    // QMI8658: auto increment, +/-2 g, +/-512 dps, 117.5 Hz.
    if (!detail::write(i2c::imu, 0x08, 0x00) || !detail::write(i2c::imu, 0x02, 0x60) ||
        !detail::write(i2c::imu, 0x03, 0x06) || !detail::write(i2c::imu, 0x04, 0x56) ||
        !detail::write(i2c::imu, 0x08, 0x03)) return false;
    uint8_t enabled = 0;
    ready_ = detail::read(i2c::imu, 0x08, &enabled, 1) && (enabled & 3) == 3;
    return ready_;
  }
  // False means no new sample or a transfer failure; output is unchanged.
  bool read(ImuReading &output) {
    uint8_t status = 0, bytes[14];
    if (!ready_ || !detail::read(i2c::imu, 0x2e, &status, 1) || (status & 3) != 3 ||
        !detail::read(i2c::imu, 0x33, bytes, sizeof(bytes))) return false;
    output.temperature = detail::signedLE(bytes) / 256.0f;
    output.acceleration.x = detail::signedLE(bytes + 2) / 16384.0f;
    output.acceleration.y = detail::signedLE(bytes + 4) / 16384.0f;
    output.acceleration.z = detail::signedLE(bytes + 6) / 16384.0f;
    output.gyroscope.x = detail::signedLE(bytes + 8) / 64.0f;
    output.gyroscope.y = detail::signedLE(bytes + 10) / 64.0f;
    output.gyroscope.z = detail::signedLE(bytes + 12) / 64.0f;
    return true;
  }
  bool ready() const { return ready_; }
  uint8_t chipId() const { return id_; }
private:
  bool ready_ = false;
  uint8_t id_ = 0;
};

class Magnetometer {
public:
  enum class Model { Unknown, MMC5603NJ, MMC5983MA };
  bool begin() {
    ready_ = false;
    model_ = Model::Unknown;
    id_ = 0;
    if (!detail::probe(i2c::magnetometer) || !detail::read(i2c::magnetometer, 0x39, &id_, 1)) return false;
    if (id_ == 0x10) model_ = Model::MMC5603NJ;
    else {
      if (!detail::read(i2c::magnetometer, 0x2f, &id_, 1) || id_ != 0x30) return false;
      model_ = Model::MMC5983MA;
    }
    if (!detail::write(i2c::magnetometer, is5983() ? 0x0a : 0x1c, 0x80)) return false;
    delay(25);
    ready_ = detail::write(i2c::magnetometer, is5983() ? 0x0b : 0x1d, 0);
    return ready_;
  }
  // Single measurement with automatic SET/RESET. Output is in microtesla.
  // Waits up to 25 ms for conversion (plus bounded I2C transfer time).
  bool read(Vector3 &output) {
    if (!ready_) return false;
    const bool m = is5983();
    if ((m && !detail::write(i2c::magnetometer, 0x08, 1)) ||
        !detail::write(i2c::magnetometer, m ? 0x09 : 0x1b, 0x21)) return false;
    const uint32_t started = millis();
    uint8_t status = 0;
    while (true) {
      if (!detail::read(i2c::magnetometer, m ? 0x08 : 0x18, &status, 1)) return false;
      if (status & (m ? 1 : 0x40)) break;
      if (millis() - started >= 25) return false;
      delay(1);
    }
    uint8_t bytes[9];
    if (!detail::read(i2c::magnetometer, 0, bytes, m ? 7 : 9)) return false;
    float values[3];
    for (size_t axis = 0; axis < 3; ++axis) {
      if (m) {
        const int32_t raw = (static_cast<uint32_t>(bytes[axis * 2]) << 10) |
          (static_cast<uint32_t>(bytes[axis * 2 + 1]) << 2) | ((bytes[6] >> (6 - axis * 2)) & 3);
        values[axis] = (raw - 131072) * (100.0f / 16384.0f);
      } else {
        const int32_t raw = (static_cast<uint32_t>(bytes[axis * 2]) << 12) |
          (static_cast<uint32_t>(bytes[axis * 2 + 1]) << 4) | (bytes[6 + axis] >> 4);
        values[axis] = (raw - 524288) * 0.00625f;
      }
    }
    output.x = values[0]; output.y = values[1]; output.z = values[2];
    return true;
  }
  bool ready() const { return ready_; }
  Model model() const { return model_; }
  uint8_t chipId() const { return id_; }
  const char *name() const {
    return model_ == Model::MMC5983MA ? "MMC5983MA" :
           model_ == Model::MMC5603NJ ? "MMC5603NJ" : "unknown";
  }
private:
  bool is5983() const { return model_ == Model::MMC5983MA; }
  bool ready_ = false;
  Model model_ = Model::Unknown;
  uint8_t id_ = 0;
};

// Function-local statics keep one shared object across translation units (C++11).
inline Adafruit_SSD1306 &display() {
  static Adafruit_SSD1306 value(screen_width, screen_height, &Wire, -1, 100000, 100000);
  return value;
}
inline Adafruit_NeoPixel &rgb() {
  static Adafruit_NeoPixel value(rgb_count, pin::rgb, NEO_GRB + NEO_KHZ800);
  return value;
}
inline Imu &imu() { static Imu value; return value; }
inline Magnetometer &magnetometer() { static Magnetometer value; return value; }
inline uint32_t i2cErrorCount() { return detail::errors(); }
inline bool buttonA() { return digitalRead(pin::button_a) == LOW; }
inline bool buttonB() { return digitalRead(pin::button_b) == LOW; }
inline int light() { return analogRead(pin::light); }
inline int sound() { return analogRead(pin::microphone); }
inline int soundLevel(uint16_t windowMs = 20) {
  if (windowMs == 0) return 0;
  int low = 4095, high = 0;
  const uint32_t started = millis();
  do {
    const int value = sound();
    low = min(low, value); high = max(high, value);
    delayMicroseconds(250);
  } while (millis() - started < windowMs);
  return high - low;
}
inline uint16_t touch(TouchPad pad) {
  static const uint8_t pins[] = {27, 14, 12, 13, 15, 4};
  const uint8_t index = static_cast<uint8_t>(pad);
  return index < 6 ? touchRead(pins[index]) : 0;
}
inline void setRGB(uint8_t red, uint8_t green, uint8_t blue) {
  rgb().fill(rgb().Color(red, green, blue)); rgb().show();
}
inline void startTone(uint16_t frequency) { ledcWriteTone(0, frequency); }
inline void stopTone() { ledcWrite(0, 0); }
inline void beep(uint16_t frequency = 880, uint16_t durationMs = 80) {
  startTone(frequency); delay(durationMs); stopTone();
}
struct InitResult { bool display = false, imu = false, magnetometer = false; };
inline InitResult begin(uint8_t brightness = 32) {
  pinMode(pin::button_a, INPUT_PULLUP); pinMode(pin::button_b, INPUT_PULLUP);
  pinMode(pin::light, INPUT); pinMode(pin::microphone, INPUT);
  analogReadResolution(12);
  analogSetPinAttenuation(pin::light, ADC_11db);
  analogSetPinAttenuation(pin::microphone, ADC_11db);
  ledcSetup(0, 880, 10); ledcAttachPin(pin::buzzer, 0); stopTone();
  Wire.begin(pin::i2c_sda, pin::i2c_scl, 100000); Wire.setTimeOut(25);
  rgb().begin(); rgb().setBrightness(brightness); rgb().clear(); rgb().show();
  InitResult result;
  result.display = detail::probe(i2c::display) &&
    display().begin(SSD1306_SWITCHCAPVCC, i2c::display, false, false);
  if (result.display) {
    display().clearDisplay(); display().setTextSize(1);
    display().setTextColor(SSD1306_WHITE); display().setCursor(0, 0); display().display();
  }
  result.imu = imu().begin(); result.magnetometer = magnetometer().begin();
  return result;
}
} // namespace matrixbit
