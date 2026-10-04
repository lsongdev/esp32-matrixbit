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

The demo opens eleven independent features: Status, OLED, IMU, Magnetometer, Light, Microphone, Touch, RGB LEDs, Buzzer, IO control and Wi-Fi. Each feature starts when opened and stops when closed. Its animated scrolling menu uses the pinned [arduino-oled-menu](https://github.com/lsongdev/arduino-oled-menu) library; license and icon attribution are in `third_party/arduino-oled-menu`.

- Short A: next menu item; hold A (650 ms): previous item. Inside a feature, A controls that feature's options.
- Short B: open the selected feature or run its action; hold B: stop the feature and return.
- IO control: A selects GPIO18/19/21/25/26/32/33; B toggles HIGH/LOW. Entry defaults to GPIO26 LOW. Changing pins or closing the feature drives the old pin LOW, then releases it as an input.
- Wi-Fi: scans only while open; returning cancels an active scan and turns the radio off.
- RGB / Buzzer: lights and sound run only in their own features and stop on exit.
- Serial: `n`/`p` control A, `o` opens/runs the action, `b` exits, `w` switches to Wi-Fi through the same cleanup path.

`src/demo.cpp` handles navigation and display refresh; each feature lives in `src/demo/`. The default `matrixbit` environment keeps the small board API example. [菜单操作说明（中文）](docs/menu.md) describes controls, cleanup and IO pin selection.

The connected board has a QMI8658 IMU and MMC5983MA magnetometer. The header also detects MMC5603NJ, but that variant has not been tested on hardware. Touch pad responses still need physical confirmation. See the documentation for complete verification results.

## Layout and target

- `lib/Matrixbit/src/matrixbit.h`: wiring, shared peripheral objects and convenience functions.
- `src/main.cpp`: example using a single include.
- `src/demo.cpp`: menu navigation and feature activation.
- `src/demo/`: independent resource features and their lifecycle interface.
- `arduino-oled-menu`: pinned reusable menu library, installed for the `demo` environment.
- `include/menu_assets.h`: attributed menu icons.
- `docs/matrixbit.md`: resource documentation.

PlatformIO is pinned to `espressif32 7.1.3` with Arduino-ESP32 2.0.17. `boards/matrixbit.json` describes the tested classic ESP32 board with 8 MB flash and the framework's `default_8MB.csv` partition table (two 3.1875 MiB application slots for OTA). Library dependencies are declared in `lib/Matrixbit/library.json`. Changing partitions on an existing device requires a serial upload; existing filesystem contents may need rebuilding.

GPIO0/GPIO2 (buttons) and GPIO12/GPIO15 (touch T/O) are ESP32 strapping pins. Avoid externally forcing them to incompatible levels during reset or power-on. Board revisions can differ; verify wiring before using this header on another revision.
