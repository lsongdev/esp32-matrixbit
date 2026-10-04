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

The demo opens an eight-item OLED menu: Status, IMU, Magnetometer, Light / Sound, Touch, RGB LEDs, Buzzer and Wi-Fi. Its three-row layout and icons are adapted from [arduino-oled-menu](https://github.com/lsongdev/arduino-oled-menu); attribution and the MIT license are in `third_party/arduino-oled-menu`.

- Short A: next item; hold A (650 ms): previous item.
- Short B: open the selected page or run its action; hold B: return to the menu.
- Wi-Fi: entering the page scans nearby networks. A browses SSIDs; B scans again. The page shows RSSI and OPEN/LOCK, and keeps up to 24 results. Scanning is asynchronous; returning to the menu stays available.
- RGB / Buzzer: B restarts the LED cycle / plays a tone.
- Serial commands: `n`/`p` move, `o` opens/runs the action, `b` returns, `w` opens the Wi-Fi scanner, `r` briefly inverts the OLED.

UI state and scanning remain in `src/demo.cpp`; the default `matrixbit` environment keeps the small board API example. [菜单操作说明（中文）](docs/menu.md) describes the page controls.

The connected board has a QMI8658 IMU and MMC5983MA magnetometer. The header also detects MMC5603NJ, but that variant has not been tested on hardware. Touch pad responses still need physical confirmation. See the documentation for complete verification results.

## Layout and target

- `include/matrixbit.h`: wiring, shared peripheral objects and convenience functions.
- `src/main.cpp`: example using a single include.
- `src/demo.cpp`: menu and resource pages using the same API.
- `include/oled_menu.h`: reusable three-row SSD1306 menu renderer.
- `include/menu_assets.h`: attributed menu icons.
- `docs/matrixbit.md`: resource documentation.

PlatformIO is pinned to `espressif32 7.1.3` and uses the standard `esp32dev` target with Arduino-ESP32 2.0.17. The attached ESP32 has 8 MB physical flash; the current standard board configuration uses a 4 MB layout. Dependencies are declared in `platformio.ini`.

GPIO0/GPIO2 (buttons) and GPIO12/GPIO15 (touch T/O) are ESP32 strapping pins. Avoid externally forcing them to incompatible levels during reset or power-on. Board revisions can differ; verify wiring before using this header on another revision.
