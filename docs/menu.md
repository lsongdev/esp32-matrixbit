# OLED 资源菜单

先烧录 `demo` 环境；默认 `matrixbit` 环境仍是简单资源接口示例。

```sh
pio run -e demo -t upload --upload-port /dev/ttyACM0
pio device monitor --port /dev/ttyACM0
```

## 操作

短按在释放时触发，按键消抖25 ms；长按650 ms触发一次，释放后不会再触发短按。

| 位置 | A短按 / 长按 | B短按 | B长按 |
| --- | --- | --- | --- |
| 菜单 | 下一项 / 上一项，循环选择 | 进入选中项 | 保持菜单 |
| 资源页面 | 下一页 / 上一页 | 执行该页操作 | 返回菜单 |
| Wi-Fi列表 | 下一网络 / 上一网络 | 重新扫描 | 返回菜单 |

返回菜单保留原来选中的菜单项。列表显示上一项、当前项、下一项；中间框是当前选中项，右侧是滚动位置。

## 页面

| 菜单项 | 内容和操作 |
| --- | --- |
| Status | 按键计数、RGB状态、传感器数据状态和I2C错误；B短鸣、重启RGB并扫描Wi-Fi |
| IMU | 加速度g、角速度°/s、芯片温度°C；无新数据时显示等待 |
| Magnetometer | 三轴磁场µT；超时的旧数据不作为有效测量显示 |
| Light / Sound | 光线ADC与麦克风峰峰值 |
| Touch | 六个触摸引脚的原始值 |
| RGB LEDs | 三色与单颗白灯循环；B重启循环 |
| Buzzer | B播放880 Hz、100 ms短鸣 |
| Wi-Fi | 异步扫描SSID、RSSI和加密标记；B重新扫描 |

Wi-Fi 页面进入时自动扫描，扫描期间不阻塞菜单操作。结果最多保存24条，按扫描器默认信号强度顺序显示；标题显示发现的总数，底部显示当前条目在保存结果中的位置、RSSI（dBm）与OPEN/LOCK。SSID在屏幕上最多显示20个字符，隐藏网络显示 `<hidden>`。默认字体不支持中文SSID，完整SSID输出至串口。

扫描失败、超时（20秒）和没有网络时分别提示，可按B重试。扫描期间返回菜单，扫描继续在后台完成；扫描结束后释放扫描缓存并关闭Wi-Fi。没有连接网络或保存密码。

串口可操作菜单：`n`下一项、`p`上一项、`o`进入/执行、`b`返回；`w`打开Wi-Fi页并扫描，`r`短暂反色。

## 实现和来源

- `src/demo.cpp`：页面、按键事件、数据刷新和Wi-Fi扫描。
- `include/oled_menu.h`：独立的 `oled_menu::Menu` 渲染器，接受 `Item` 数组（标签、16×16 XBM图标），提供 `next()`、`previous()`、`selected()`、`draw(screen)`。`draw()` 只绘制缓冲区，调用方负责 `screen.display()`。
- `include/menu_assets.h`：菜单示例的图标，使用 `drawXBitmap()` 保持原始位序。

菜单布局和图标适配自 [lsongdev/arduino-oled-menu](https://github.com/lsongdev/arduino-oled-menu)，原作者upir，固定来源提交及MIT许可见 `third_party/arduino-oled-menu`。该仓库原本为u8g/u8g2 Arduino示例，本项目使用现有Adafruit SSD1306，不额外引入显示库。
