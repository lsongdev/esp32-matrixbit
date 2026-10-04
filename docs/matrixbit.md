# Matrix:bit 资源使用说明

适用于本项目的经典 ESP32 Matrix:bit。应用只需 `#include "matrixbit.h"`，在 `setup()` 中调用 `matrixbit::begin()`。依赖由 PlatformIO 自动安装：Adafruit SSD1306、Adafruit NeoPixel，以及 SSD1306 的 GFX/BusIO 依赖。

## 最小示例

```cpp
#include "matrixbit.h"

void setup() {
  Serial.begin(115200);
  const auto result = matrixbit::begin();
  if (result.display) {
    auto &screen = matrixbit::display();
    screen.println("Hello Matrix:bit");
    screen.display();
  }
  matrixbit::setRGB(0, 32, 0);
}

void loop() {
  matrixbit::ImuReading motion;
  if (matrixbit::imu().read(motion)) {
    Serial.printf("Z: %.3f g\n", motion.acceleration.z);
  }
  delay(20);
}
```

完整例子见 [src/main.cpp](../src/main.cpp)。以下接口均位于 `matrixbit` 命名空间。

## 初始化和硬件配置

`InitResult begin(uint8_t brightness = 32)` 初始化按键、ADC、蜂鸣器、I2C、RGB、OLED、IMU 和磁力计；返回 `.display`、`.imu`、`.magnetometer` 三个独立的成功标志。某个 I2C 设备失败不会阻止其他设备初始化。OLED 成功表示地址应答和库初始化成功，像素显示效果仍需观察屏幕确认。

调用一次即可；应在 Arduino `setup()` 内调用，不要在全局对象构造阶段调用。它不启动串口、Wi-Fi 或蓝牙。`display()`、`rgb()`、`imu()`、`magnetometer()` 返回共享对象引用，头文件可被多个源文件引入。

初始化配置：I2C SDA=23 / SCL=22、100 kHz、传输超时 25 ms；ADC 12 位、11 dB 衰减；蜂鸣器占用 LEDC 通道 0。使用其他外设时注意这些共享配置。当前代码使用 Arduino ESP32 2.x 的 LEDC API；迁移 3.x 时需要调整蜂鸣器接口。

| 资源 | 连接 / 配置 | 接口 |
| --- | --- | --- |
| OLED | 0x3C，SSD1306，128×64 | `display()` |
| RGB | GPIO17，3 颗，GRB / 800 kHz | `rgb()`、`setRGB(r,g,b)` |
| 按键 A / B | GPIO0 / GPIO2，低电平按下 | `buttonA()`、`buttonB()` |
| 蜂鸣器 | GPIO16，LEDC 通道0 | `beep()`、`startTone()`、`stopTone()` |
| IMU | 0x6B，QMI8658，ID=0x05 | `imu()` |
| 磁力计 | 0x30，自动识别芯片 | `magnetometer()` |
| 光线 | GPIO39，ADC 原始值 | `light()` |
| 麦克风 | GPIO36，ADC 原始值 | `sound()`、`soundLevel()` |
| 触摸 P/Y/T/H/O/N | GPIO27/14/12/13/15/4 | `touch(TouchPad::P)` 等 |

GPIO0、GPIO2 是启动配置引脚，复位或上电时避免按住按键。引脚和地址也可以直接通过 `pin::button_a`、`pin::light`、`i2c::imu` 等常量访问；屏幕尺寸与灯珠数量为 `screen_width`、`screen_height`、`rgb_count`。

## 显示屏和 RGB

`display()` 返回 `Adafruit_SSD1306&`，可使用原库的文字、线条、图形 API。`clearDisplay()` 和绘图函数只修改内存，调用 `display().display()` 才刷新屏幕；清屏后按需要 `setCursor(0, 0)`。内置默认字体不支持中文。

`setRGB(red, green, blue)` 设置全部三颗灯并立即显示，颜色参数 0–255。`begin(brightness)` 的亮度范围也是 0–255，默认32。分别控制灯珠时使用原库：

```cpp
matrixbit::rgb().clear();
matrixbit::rgb().setPixelColor(0, matrixbit::rgb().Color(255, 0, 0));
matrixbit::rgb().show(); // 灯珠索引 0、1、2
```

## 按键、蜂鸣器、模拟量、触摸

- `buttonA()` / `buttonB()`：返回是否按下的实时状态，没有自动消抖、长按或边沿检测。应用自行处理；诊断程序提供25 ms消抖示例。
- `beep(uint16_t frequency = 880, uint16_t durationMs = 80)`：频率 Hz，时长 ms，调用期间阻塞。
- `startTone(uint16_t frequency)` / `stopTone()`：立即开始/停止发声。配合 `millis()` 自行实现不阻塞的定时鸣叫。
- `light()` / `sound()`：返回0–4095的ADC值，不是照度 lux 或声压 dB，光照方向及阈值应按实际板子标定。
- `soundLevel(uint16_t windowMs = 20)`：在采样窗口内返回最大值减最小值，单位仍为ADC计数；约每250 µs读取一次，调用期间阻塞。窗口为0返回0。
- `touch(TouchPad pad)`：返回ESP32原始触摸计数；枚举为 `P,Y,T,H,O,N`。不自动判定按下，需测量基线并设置阈值。非法枚举值返回0。

模拟和触摸引脚来自兼容 mPython 接线。已读到模拟量变化和触摸原始值，但尚未逐个确认触摸焊盘的实际响应；不同版本应核对接线。

## IMU

`imu().begin()` 可单独重新初始化；`ready()` 表示初始化成功，`chipId()` 返回读取到的器件ID。`matrixbit::begin()` 已调用初始化。

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

## 诊断固件

```sh
pio run -e diagnostics -t upload --upload-port /dev/ttyACM0
pio device monitor --port /dev/ttyACM0
```

A切换5页（概览、IMU、磁力计、模拟量、触摸）；B短鸣并重启RGB循环。启动时屏幕全亮及棋盘格测试，RGB轮流全红/绿/蓝和三颗独立白灯。串口 `r` 短暂反色屏幕，`w` 重新扫描Wi-Fi。

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

Wi-Fi 使用ESP32原生扫描接口，应用仍可直接使用 Arduino `WiFi.h`；网络功能没有另加封装。

## 参考资料

- [YFROBOT Matrix:bit 主板](https://yfrobot.com.cn/wiki/index.php?title=Matrix:Bit%E4%B8%BB%E6%9D%BF)
- [mPython 官方接线与磁力计实现](https://github.com/labplus-cn/mpython/blob/master/port/boards/mpython/modules/mpython.py)
- [QMI8658C 数据手册（Waveshare 镜像）](https://files.waveshare.com/wiki/common/QMI8658C_datasheet_rev_0.9.pdf)
- [SparkFun MMC5983MA 驱动](https://github.com/sparkfun/SparkFun_MMC5983MA_Magnetometer_Arduino_Library)
- [Adafruit MMC5603 驱动](https://github.com/adafruit/Adafruit_MMC56x3)
