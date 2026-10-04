# Matrix:bit 资源使用说明

适用于本项目的经典 ESP32 Matrix:bit。应用只需 `#include <matrixbit.h>`；板级 API 保持轻量，不重新包装 Arduino 生态。

本仓库的库位于 `lib/Matrixbit`，依赖在 `library.json` 声明：Adafruit SSD1306、Adafruit NeoPixel、GFX 和 BusIO。

## 在其他项目使用

每台开发机先安装一次共享 board、库及依赖：

```sh
python3 /home/lsong/Projects/matrixbit-demo/scripts/install.py
```

新项目保留常规的 `platform = espressif32 @ 7.1.3`、`framework = arduino`，设置 `board = matrixbit`，代码中 `#include <matrixbit.h>` 即可调用下面的 API，不必复制头文件或填写这些外设库的 `lib_deps`。

安装器在 `~/.platformio/boards/` 和 `~/.platformio/lib/` 创建指向本仓库的链接，因此应保留本仓库位置；修改共享库后，其他项目重新编译即可使用。换机器或 CI 也要先执行安装器。自定义 PlatformIO Core 路径可通过 `--core-dir PATH` 指定。

board 适用于已验证的经典 ESP32 / 8 MB Flash 版本，使用框架的 `default_8MB.csv` OTA 分区。已有设备切换分区需通过串口上传，文件系统数据可能需要重新构建。board 保留通用 ESP32 Arduino variant，I2C 的板载接线仍由 `matrixbit::begin()` 配置。

## 初始化

`matrixbit::begin()` 只初始化共享资源：A/B 按键、光线/麦克风 ADC，以及 I2C（SDA=23 / SCL=22、100 kHz、25 ms timeout）。它不会自动占用 OLED、RGB、蜂鸣器、IMU 或磁力计。

需要什么就显式初始化什么：

```cpp
#include <matrixbit.h>

void setup() {
  Serial.begin(115200);
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
}
```

建议在 Arduino `setup()` 内调用一次 `matrixbit::begin()`。它不启动串口、Wi-Fi 或蓝牙。

| 资源 | 连接 / 配置 | 接口 |
| --- | --- | --- |
| OLED | 0x3C，SSD1306，128×64 | `beginDisplay()`、`display()` |
| RGB | GPIO17，3 颗，GRB / 800 kHz | `beginRGB()`、`rgb()`、`setRGB()` |
| 按键 A / B | GPIO0 / GPIO2，低电平按下 | `buttonA()`、`buttonB()` |
| 蜂鸣器 | GPIO16，LEDC 通道0 | `beginBuzzer()`、`beep()`、`startTone()`、`stopTone()` |
| IMU | 0x6B，QMI8658，ID=0x05 | `imu()` |
| 磁力计 | 0x30，自动识别芯片 | `magnetometer()` |
| 光线 | GPIO39，ADC 原始值 | `light()` |
| 麦克风 | GPIO36，ADC 原始值 | `sound()`、`soundLevel()` |
| 触摸 P/Y/T/H/O/N | GPIO27/14/12/13/15/4 | `touch()`、`touchPin()` |

GPIO0、GPIO2、GPIO12、GPIO15 都是 ESP32 strapping pins。板载电路按设计使用即可；如果外接电路，复位或上电时不要强制到不兼容的电平。

## 显示屏和 RGB

`beginDisplay()` 探测并初始化 OLED，成功返回 `true`。`display()` 返回 `Adafruit_SSD1306&`，可使用原库的文字、线条、图形 API。`clearDisplay()` 和绘图函数只修改内存，调用 `display().display()` 才刷新屏幕；清屏后按需要 `setCursor(0, 0)`。内置默认字体不支持中文。

`beginRGB(brightness)` 初始化 RGB，亮度范围 0–255，默认32。`setRGB(red, green, blue)` 设置全部三颗灯并立即显示，颜色参数 0–255。分别控制灯珠时使用原库：

```cpp
matrixbit::rgb().clear();
matrixbit::rgb().setPixelColor(0, matrixbit::rgb().Color(255, 0, 0));
matrixbit::rgb().show(); // 灯珠索引 0、1、2
```

## 按键、蜂鸣器、模拟量、触摸

- `buttonA()` / `buttonB()`：返回是否按下的实时状态，没有自动消抖、长按或边沿检测。应用自行处理；Demo 中提供了 25 ms 消抖示例。
- `beginBuzzer()`：初始化 GPIO16，并占用 LEDC channel 0。
- `beep(uint16_t frequency = 880, uint16_t durationMs = 80)`：频率 Hz，时长 ms，调用期间阻塞；使用前先调用 `beginBuzzer()`。
- `startTone(uint16_t frequency)` / `stopTone()`：立即开始/停止发声。配合 `millis()` 自行实现不阻塞的定时鸣叫。
- `light()` / `sound()`：返回0–4095的ADC值，不是照度 lux 或声压 dB，光照方向及阈值应按实际板子标定。
- `soundLevel(uint16_t windowMs = 20)`：在采样窗口内返回最大值减最小值，单位仍为ADC计数；约每250 µs读取一次，调用期间阻塞。窗口为0返回0。
- `touch(TouchPad pad)`：返回ESP32原始触摸计数；枚举为 `P,Y,T,H,O,N`。不自动判定按下，需测量基线并设置阈值。非法枚举值返回0。

模拟和触摸引脚来自兼容 mPython 接线。已读到模拟量变化和触摸原始值，但尚未逐个确认触摸焊盘的实际响应；不同版本应核对接线。

## IMU

`imu().begin()` 会验证器件 ID、执行 QMI8658 自身 soft reset，再写入确定的量程和 ODR 配置。ESP32 复位并不一定让 IMU 掉电，因此这里不依赖上一次程序留下的寄存器状态。`ready()` 表示初始化成功，`chipId()` 返回读取到的器件ID。

```cpp
matrixbit::ImuReading motion;
if (matrixbit::imu().read(motion)) {
  // motion.acceleration.x/y/z : g（含重力），量程 ±2 g
  // motion.gyroscope.x/y/z    : °/s，量程 ±512 °/s
  // motion.temperature       : 芯片内部温度 °C，不是环境温度
}
```

输出数据率约117.5 Hz。`read()` 仅在加速度和陀螺仪都准备好时返回 `true`；`false` 表示尚无新数据、未初始化成功或I2C传输失败，输出参数保持不变。不要把未成功读取的默认零值显示为有效测量。静止时加速度向量的长度通常接近1 g。

## 磁力计

```cpp
matrixbit::Vector3 field;
if (matrixbit::magnetometer().read(field)) {
  Serial.printf("%.2f %.2f %.2f uT\n", field.x, field.y, field.z);
}
```

`begin()` 根据ID自动识别 MMC5983MA（寄存器0x2F=0x30）或 MMC5603NJ（0x39=0x10），未知ID拒绝初始化。当前连接的板子是 **MMC5983MA**；MMC5603NJ 分支尚未在本机实物验证。`name()`、`model()`、`chipId()`、`ready()` 可查询识别和初始化结果。

`read(Vector3&)` 输出三轴磁场 µT，使用单次测量及自动 SET/RESET，等待转换最多25 ms，另有I2C传输耗时；失败返回 `false` 并保持输出不变。这是原始磁场，没有软硬铁校准、倾斜补偿或航向角计算，远离磁铁和大电流导线测量。

`i2cErrorCount()` 返回封装读写操作的累计传输错误数。扫描地址的预期NACK、未知芯片ID和无新数据不计为传输错误；错误计数为0不代表每次读取都有新数据。

## Demo

```sh
pio run -e demo -t upload --upload-port /dev/ttyACM0
pio device monitor --port /dev/ttyACM0
```

Demo 启动后显示8项资源菜单。A短按下一项、长按上一项；B短按进入/执行、长按返回。Wi-Fi 页面支持扫描、浏览SSID、查看RSSI与加密标记。详细说明见 [菜单操作说明](menu.md)。启动时保留屏幕全亮、棋盘格和RGB循环。

本次实板验证：

| 资源 | 结果 |
| --- | --- |
| OLED、A/B、三颗RGB、蜂鸣器 | 用户已确认显示、操作、颜色和发声正常 |
| QMI8658 IMU | 加速度、陀螺仪、温度持续更新；用户已看到数据 |
| MMC5983MA 磁力计 | 三轴数据持续更新，转动板子时变化 |
| 光线、麦克风 | ADC可读取且有变化；未标定物理单位 |
| 触摸 | 可读取原始值；未逐个确认触摸响应 |
| Wi-Fi | 扫描收到37–39个网络；未验证联网 |
| 蓝牙 | 未验证 |

Demo 使用 ESP32 `esp_wifi_scan_start(..., false)` 和扫描完成事件异步扫描Wi-Fi，并读取SSID/RSSI/加密类型；扫描完成后关闭无线。网络功能没有加入 `matrixbit.h`。

## 参考资料

- [YFROBOT Matrix:bit 主板](https://yfrobot.com.cn/wiki/index.php?title=Matrix:Bit%E4%B8%BB%E6%9D%BF)
- [mPython 官方接线与磁力计实现](https://github.com/labplus-cn/mpython/blob/master/port/boards/mpython/modules/mpython.py)
- [QMI8658C 数据手册（Waveshare 镜像）](https://files.waveshare.com/wiki/common/QMI8658C_datasheet_rev_0.9.pdf)
- [SparkFun MMC5983MA 驱动](https://github.com/sparkfun/SparkFun_MMC5983MA_Magnetometer_Arduino_Library)
- [Adafruit MMC5603 驱动](https://github.com/adafruit/Adafruit_MMC56x3)
