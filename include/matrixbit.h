#pragma once

#include <stdint.h>

namespace matrixbit {

namespace pin {
constexpr uint8_t button_a = 0;
constexpr uint8_t button_b = 2;
constexpr uint8_t buzzer = 16;
constexpr uint8_t rgb = 17;
constexpr uint8_t i2c_scl = 22;
constexpr uint8_t i2c_sda = 23;
} // namespace pin

namespace i2c {
constexpr uint8_t magnetometer = 0x30;
constexpr uint8_t display = 0x3c;
constexpr uint8_t imu = 0x6b;
} // namespace i2c

} // namespace matrixbit
