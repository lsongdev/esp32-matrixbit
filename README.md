# esp32-matrixbit

Minimal [PlatformIO](https://platformio.org/) starter project for the ESP32-based Matrix:bit board.

The goal is intentionally small: treat Matrix:bit as an ESP32 board, keep the board-specific layer to pin/address definitions, and use normal Arduino libraries for peripherals.

## Hardware target

This repository currently targets the classic ESP32 Matrix:bit V2.x family. Matrix:bit revisions exist, so verify the board revision before relying on peripheral details that are not listed here.

Known wiring used by this project:

| Peripheral | Connection |
| --- | --- |
| Button A | GPIO 0 |
| Button B | GPIO 2 |
| Buzzer | GPIO 16 |
| RGB / NeoPixel data | GPIO 17 |
| I2C SCL | GPIO 22 |
| I2C SDA | GPIO 23 |
| Magnetometer | I2C `0x30` |
| OLED | I2C `0x3c` |
| IMU | I2C `0x6b` |

GPIO 0 and GPIO 2 are ESP32 boot-strapping pins. Avoid holding the buttons while resetting or powering on the board.

References:

- YFROBOT Matrix:bit wiki: https://yfrobot.com.cn/wiki/index.php?title=Matrix%3ABit%E4%B8%BB%E6%9D%BF
- Matrix:bit V2.0 hardware summary: https://shop.tavir.hu/termek/alappanel/espressif/matrixbit-esp32-v2/

## Getting started

Install PlatformIO, connect the board over USB, then:

```sh
pio run
pio run -t upload
pio device monitor
```

The default firmware initializes the shared I2C bus, scans all I2C addresses, and reports Button A/B changes over the serial monitor.

A typical V2.0 board may report:

```text
found 0x30
found 0x3C
found 0x6B
```

That makes the starter firmware useful as a first hardware/revision check before adding display, IMU, RGB, or other drivers.

## Project layout

```text
.
├── include/
│   └── matrixbit.h
├── src/
│   └── main.cpp
└── platformio.ini
```

`matrixbit.h` contains only Matrix:bit-specific hardware constants. It deliberately does not wrap Arduino APIs or third-party peripheral libraries.

For example:

```cpp
#include <Wire.h>
#include "matrixbit.h"

void setup()
{
  Wire.begin(matrixbit::pin::i2c_sda, matrixbit::pin::i2c_scl);
}
```

## PlatformIO board

For now the project uses PlatformIO's standard `esp32dev` board definition:

```ini
[env:matrixbit]
platform = espressif32
board = esp32dev
framework = arduino
```

This keeps the project usable without inventing unverified flash/upload metadata. A dedicated `matrixbit.json` board definition can be added later once the exact flash and upload characteristics of the supported revision are verified.
