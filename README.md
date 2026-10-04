# esp32-matrixbit

A small PlatformIO project for the classic ESP32 Matrix:bit, with a lightweight header-only board API.

`matrixbit::begin()` initializes only shared GPIO, ADC and I2C resources. OLED, RGB, buzzer, IMU and magnetometer initialization stays explicit, so applications only claim the peripherals they actually use.

**[资源和 API 使用文档（中文）](docs/matrixbit.md)** covers wiring, units, examples, initialization results and hardware verification status.

## Quick start

```sh
pio run
pio run -t upload --upload-port /dev/ttyACM0
pio device monitor --port /dev/ttyACM0
```

The default example displays sensor values, sets the RGB LEDs red while A is pressed, and blue with a short beep when B is pressed. The serial monitor runs at 115200 baud.

```cpp
#include "matrixbit.h"

void setup() {
  matrixbit::begin();
  matrixbit::beginRGB();
  matrixbit::beginBuzzer();

  const bool oled = matrixbit::beginDisplay();
  const bool imu = matrixbit::imu().begin();
  const bool mag = matrixbit::magnetometer().begin();

  if (oled) {
    matrixbit::display().println("Hello Matrix:bit");
    matrixbit::display().display();
  }

  matrixbit::setRGB(0, 32, 0);
}
```

## Demo

```sh
pio run -e demo -t upload --upload-port /dev/ttyACM0
pio device monitor --port /dev/ttyACM0
```

The separate demo exercises the whole board without putting UI state, Wi-Fi scanning or button debounce into the board API. Press A to cycle pages; B beeps and restarts the RGB cycle. Serial commands: `r` briefly inverts the OLED and `w` repeats the Wi-Fi scan.

The connected board has a QMI8658 IMU and MMC5983MA magnetometer. The header also detects MMC5603NJ, but that variant has not been tested on hardware. Touch pad responses still need physical confirmation. See the documentation for complete verification results.

## Layout and target

- `include/matrixbit.h`: wiring, shared peripheral objects and convenience functions.
- `src/main.cpp`: example using a single include.
- `src/demo.cpp`: interactive resource diagnostics using the same API.
- `docs/matrixbit.md`: resource documentation.

PlatformIO is pinned to `espressif32 7.1.3` and uses the standard `esp32dev` target with Arduino-ESP32 2.0.17. The attached ESP32 has 8 MB physical flash; the current standard board configuration uses a 4 MB layout. Dependencies are declared in `platformio.ini`.

GPIO0/GPIO2 (buttons) and GPIO12/GPIO15 (touch T/O) are ESP32 strapping pins. Avoid externally forcing them to incompatible levels during reset or power-on. Board revisions can differ; verify wiring before using this header on another revision.
