# esp32-matrixbit

A small PlatformIO project for the classic ESP32 Matrix:bit, with a header-only peripheral API.

Include **[matrixbit.h](include/matrixbit.h)** and call `matrixbit::begin()` to initialize the OLED, three RGB LEDs, buttons, buzzer, IMU, magnetometer, analog inputs and touch inputs. OLED and RGB helpers expose the original Adafruit library objects for drawing and individual LED control.

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
  const auto result = matrixbit::begin();
  if (result.display) {
    matrixbit::display().println("Hello Matrix:bit");
    matrixbit::display().display();
  }
  matrixbit::setRGB(0, 32, 0);
}

void loop() {
  matrixbit::ImuReading motion;
  if (matrixbit::imu().read(motion)) {
    // acceleration in g, gyroscope in degrees/second, temperature in Celsius
  }
  delay(20);
}
```

## Hardware diagnostics

```sh
pio run -e diagnostics -t upload --upload-port /dev/ttyACM0
pio device monitor --port /dev/ttyACM0
```

Press A to cycle status, IMU, magnetometer, analog and touch pages. B beeps and restarts the RGB cycle. Serial commands: `r` inverts the OLED briefly; `w` repeats the Wi-Fi scan.

The connected board has a QMI8658 IMU and MMC5983MA magnetometer. The header also detects MMC5603NJ, but that variant has not been tested on hardware. Touch pad responses still need physical confirmation. See the documentation for complete verification results.

## Layout and target

- `include/matrixbit.h`: wiring, shared peripheral objects and convenience functions.
- `src/main.cpp`: example using a single include.
- `src/diagnostics.cpp`: interactive resource diagnostics using the same API.
- `docs/matrixbit.md`: resource documentation.

PlatformIO uses `esp32dev` with Arduino ESP32 2.x. The attached ESP32 has 8 MB physical flash; the current standard board configuration uses a 4 MB layout. Dependencies are declared in `platformio.ini`.

Buttons use boot-strapping GPIO0 and GPIO2; avoid holding them during reset or power-on. Board revisions can differ; verify wiring before using this header on another revision.
