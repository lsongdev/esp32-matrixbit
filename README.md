# esp32-matrixbit

A small PlatformIO project for the classic ESP32 Matrix:bit, with a lightweight header-only board API.

`matrixbit::begin()` initializes only shared GPIO, ADC and I2C resources. OLED, RGB, buzzer, IMU and magnetometer initialization stays explicit, so applications only claim the peripherals they actually use.

**[资源和 API 使用文档（中文）](docs/matrixbit.md)** covers wiring, units, examples, initialization results and hardware verification status.

## Use in another project

Install the board and library once on each development machine:

```sh
python3 /path/to/esp32-matrixbit/scripts/install.py
```

The installer links `boards/matrixbit.json` into `~/.platformio/boards/` and
`lib/Matrixbit` into `~/.platformio/lib/`. Keep this checkout in place;
updates to its board definition and header are shared by consuming projects.
Use `--core-dir PATH` if PlatformIO uses a different Core directory.
The installer refuses to overwrite an unrelated existing installation.

A new project's `platformio.ini` only needs the usual platform/framework settings:

```ini
[env:matrixbit]
platform = espressif32 @ 7.1.3
board = matrixbit
framework = arduino
monitor_speed = 115200
```

Then `#include <matrixbit.h>` and call the same resource API shown below.
The installer also installs the declared Adafruit dependencies globally.
PlatformIO discovers the shared library from the include when building.
No copied header or `lib_deps` is needed.
Remove any old project-local `include/matrixbit.h` so it cannot shadow the shared library.
This global installation is local to the machine: teammates and CI must run the
installer too. Selecting the board alone does not install the library.
`matrixbit::begin()` sets up the board's I2C pins; bare `Wire.begin()` still uses
the generic ESP32 variant defaults. This board definition is for the verified
8 MB classic ESP32 revision, not every board sold under the Matrix:bit name.

## Quick start

```sh
pio run
pio run -t upload --upload-port /dev/ttyACM0
pio device monitor --port /dev/ttyACM0
```

The default example displays sensor values, sets the RGB LEDs red while A is pressed, and blue with a short beep when B is pressed. The serial monitor runs at 115200 baud.

```cpp
#include <matrixbit.h>

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

- `lib/Matrixbit/src/matrixbit.h`: wiring, shared peripheral objects and convenience functions.
- `src/main.cpp`: example using a single include.
- `src/demo.cpp`: menu and resource pages using the same API.
- `include/oled_menu.h`: reusable three-row SSD1306 menu renderer.
- `include/menu_assets.h`: attributed menu icons.
- `docs/matrixbit.md`: resource documentation.

PlatformIO is pinned to `espressif32 7.1.3` with Arduino-ESP32 2.0.17. `boards/matrixbit.json` describes the tested classic ESP32 board with 8 MB flash and the framework's `default_8MB.csv` partition table (two 3.1875 MiB application slots for OTA). Library dependencies are declared in `lib/Matrixbit/library.json`. Changing partitions on an existing device requires a serial upload; existing filesystem contents may need rebuilding.

GPIO0/GPIO2 (buttons) and GPIO12/GPIO15 (touch T/O) are ESP32 strapping pins. Avoid externally forcing them to incompatible levels during reset or power-on. Board revisions can differ; verify wiring before using this header on another revision.
