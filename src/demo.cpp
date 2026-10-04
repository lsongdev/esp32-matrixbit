#include <Arduino.h>
#include <Wire.h>
#include <WiFi.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_NeoPixel.h>
#include <math.h>
#include <esp_wifi.h>
#include <OledMenu.h>
#include "menu_assets.h"

#include <matrixbit.h>

namespace {

// These analog/touch connections follow the compatible mPython board.
// Their response still needs confirmation on the attached Matrix:bit revision.
constexpr uint8_t touchPins[] = {
  matrixbit::pin::touch_p, matrixbit::pin::touch_y, matrixbit::pin::touch_t,
  matrixbit::pin::touch_h, matrixbit::pin::touch_o, matrixbit::pin::touch_n
};
constexpr char touchNames[] = "PYTHON";
enum Page : uint8_t { Status, Imu, Magnetic, Analog, Touch, Rgb, Buzzer, Wifi };
const oledmenu::Icon menuIcons[] = {
  {menu_assets::bitmap_icon_battery, 16, 16},
  {menu_assets::bitmap_icon_3dcube, 16, 16},
  {menu_assets::bitmap_icon_gps_speed, 16, 16},
  {menu_assets::bitmap_icon_dashboard, 16, 16},
  {menu_assets::bitmap_icon_knob_over_oled, 16, 16},
  {menu_assets::bitmap_icon_fireworks, 16, 16},
  {menu_assets::bitmap_icon_turbo, 16, 16},
  {menu_assets::bitmap_icon_parksensor, 16, 16}
};
const oledmenu::Item menuItems[] = {
  {"Status", nullptr, &menuIcons[0]},
  {"IMU", nullptr, &menuIcons[1]},
  {"Magnetometer", nullptr, &menuIcons[2]},
  {"Light / Sound", nullptr, &menuIcons[3]},
  {"Touch", nullptr, &menuIcons[4]},
  {"RGB LEDs", nullptr, &menuIcons[5]},
  {"Buzzer", nullptr, &menuIcons[6]},
  {"Wi-Fi", nullptr, &menuIcons[7]}
};
constexpr uint8_t pageCount = sizeof(menuItems) / sizeof(menuItems[0]);
oledmenu::Menu menu(menuItems, pageCount);
bool inMenu = true;

Adafruit_SSD1306 &display = matrixbit::display();
Adafruit_NeoPixel &pixels = matrixbit::rgb();

struct Button {
  Button(uint8_t inputPin, const char *label) : pin(inputPin), name(label) {}
  uint8_t pin;
  const char *name;
  int stable = HIGH;
  int raw = HIGH;
  uint32_t changedAt = 0;
  uint32_t presses = 0;
  uint32_t pressedAt = 0;
  bool held = false;
};

Button buttonA{matrixbit::pin::button_a, "A"};
Button buttonB{matrixbit::pin::button_b, "B"};
bool oledReady = false;
bool imuReady = false;
bool magReady = false;
bool imuSampleValid = false;
bool magSampleValid = false;
uint32_t imuSamples = 0;
uint32_t magSamples = 0;
uint32_t lastImuAt = 0;
uint32_t lastMagAt = 0;
float acceleration[3] = {};
float angularRate[3] = {};
float magneticField[3] = {};
float imuTemperature = NAN;
uint8_t page = 0;
uint8_t rgbStage = 0;
uint32_t rgbChangedAt = 0;
int lightValue = 0;
int micMinimum = 4095;
int micMaximum = 0;
int micAmplitude = 0;
uint16_t touchValues[6] = {};
int wifiNetworks = -1;
bool wifiScanning = false;
uint32_t wifiStartedAt = 0;
constexpr uint16_t wifiCapacity = 24;
struct WifiResult { char ssid[33] = {}; int32_t rssi = 0; bool secured = false; };
WifiResult wifiResults[wifiCapacity];
uint16_t wifiResultCount = 0, wifiSelected = 0;
volatile bool wifiFinished = false;
volatile uint32_t wifiStatus = 0;
const char *wifiError = nullptr;
uint32_t buzzerUntil = 0;

void sampleSensors()
{
  matrixbit::ImuReading motion;
  if (imuReady && matrixbit::imu().read(motion)) {
    imuTemperature = motion.temperature;
    acceleration[0] = motion.acceleration.x; acceleration[1] = motion.acceleration.y; acceleration[2] = motion.acceleration.z;
    angularRate[0] = motion.gyroscope.x; angularRate[1] = motion.gyroscope.y; angularRate[2] = motion.gyroscope.z;
    imuSampleValid = true; lastImuAt = millis(); ++imuSamples;
  }
  matrixbit::Vector3 field;
  if (magReady && matrixbit::magnetometer().read(field)) {
    magneticField[0] = field.x; magneticField[1] = field.y; magneticField[2] = field.z;
    magSampleValid = true; lastMagAt = millis(); ++magSamples;
  }
  lightValue = matrixbit::light();
  for (size_t i = 0; i < 6; ++i) touchValues[i] = matrixbit::touch(static_cast<matrixbit::TouchPad>(i));
}

const char *rgbStageName()
{
  static const char *names[] = {"all-red", "all-green", "all-blue", "LED1-white", "LED2-white", "LED3-white", "all-off"};
  return names[rgbStage];
}

void showRgbStage()
{
  pixels.clear();
  if (rgbStage < 3) {
    const uint32_t colors[] = {pixels.Color(255, 0, 0), pixels.Color(0, 255, 0), pixels.Color(0, 0, 255)};
    pixels.fill(colors[rgbStage]);
  } else if (rgbStage < 6) {
    pixels.setPixelColor(rgbStage - 3, pixels.Color(255, 255, 255));
  }
  pixels.show();
  Serial.printf("[RGB] %s; visual confirmation required\n", rgbStageName());
}

void startWifiScan()
{
  if (wifiScanning) return;
  wifiError = nullptr;
  wifiResultCount = 0; wifiSelected = 0; wifiNetworks = -1;
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(false); WiFi.disconnect(false, false); WiFi.setSleep(false);
  WiFi.scanDelete();
  wifi_scan_config_t config = {};
  config.show_hidden = true;
  config.scan_type = WIFI_SCAN_TYPE_ACTIVE;
  config.scan_time.active.min = 100; config.scan_time.active.max = 300;
  wifiFinished = false;
  const esp_err_t result = esp_wifi_scan_start(&config, false);
  wifiStartedAt = millis();
  wifiScanning = result == ESP_OK;
  if (!wifiScanning) {
    wifiNetworks = -2; wifiError = "Scan start failed";
    Serial.printf("[WIFI] start failed: %s\n", esp_err_to_name(result));
    WiFi.mode(WIFI_OFF);
  } else Serial.println("[WIFI] scanning...");
}

void updateWifiScan()
{
  if (!wifiScanning) return;
  if (!wifiFinished && millis() - wifiStartedAt < 20000) return;
  if (!wifiFinished) {
    esp_wifi_scan_stop(); wifiError = "Scan timed out";
  } else if (wifiStatus != 0) wifiError = "Scan failed";
  else {
    // Arduino's scan-done handler already moves the native AP list into its cache.
    const int total = WiFi.scanComplete();
    if (total < 0) wifiError = "Read results failed";
    else {
      wifiNetworks = total;
      wifiResultCount = min(static_cast<uint16_t>(total), wifiCapacity);
      for (uint16_t i = 0; i < wifiResultCount; ++i) {
        String ssid; uint8_t auth; int32_t rssi, channel; uint8_t *bssid;
        if (!WiFi.getNetworkInfo(i, ssid, auth, rssi, bssid, channel)) {
          wifiError = "Read results failed"; wifiResultCount = 0; break;
        }
        ssid.toCharArray(wifiResults[i].ssid, sizeof(wifiResults[i].ssid));
        wifiResults[i].rssi = rssi; wifiResults[i].secured = auth != WIFI_AUTH_OPEN;
      }
    }
  }
  wifiScanning = false;
  if (wifiError) { wifiNetworks = -2; esp_wifi_clear_ap_list(); }
  WiFi.scanDelete();
  Serial.printf("[WIFI] found=%d showing=%u error=%s\n", wifiNetworks, wifiResultCount, wifiError ? wifiError : "none");
  for (uint16_t i = 0; i < wifiResultCount; ++i)
    Serial.printf("  %s %d dBm %s\n", wifiResults[i].ssid,
                  wifiResults[i].rssi, wifiResults[i].secured ? "secured" : "open");
  WiFi.mode(WIFI_OFF);
}

void drawWifi()
{
  display.printf("WiFi: %s", wifiScanning ? "scanning..." : "");
  if (!wifiScanning && !wifiError) display.printf("%d found", wifiNetworks < 0 ? 0 : wifiNetworks);
  display.drawLine(0, 9, 127, 9, SSD1306_WHITE);
  if (wifiScanning) {
    display.setCursor(0, 18); display.print("Please wait...\nHold B to return");
  } else if (wifiError || !wifiResultCount) {
    display.setCursor(0, 18); display.println(wifiError ? wifiError : "No networks found");
  } else {
    const uint16_t first = wifiSelected / 3 * 3;
    for (uint16_t row = 0; row < 3 && first + row < wifiResultCount; ++row) {
      const uint16_t index = first + row;
      const int y = 12 + row * 12;
      if (index == wifiSelected) display.fillRect(0, y - 1, 128, 10, SSD1306_WHITE);
      display.setTextColor(index == wifiSelected ? SSD1306_BLACK : SSD1306_WHITE);
      display.setCursor(1, y);
      const char *ssid = wifiResults[index].ssid;
      display.printf("%.20s", *ssid ? ssid : "<hidden>");
    }
    display.setTextColor(SSD1306_WHITE); display.setCursor(0, 46);
    const auto &network = wifiResults[wifiSelected];
    display.printf("%u/%u %ddBm %s", wifiSelected + 1, wifiResultCount,
                   network.rssi, network.secured ? "LOCK" : "OPEN");
  }
  display.setCursor(0, 56); display.print("B:scan Hold B:back");
  display.display();
}

void beep(uint32_t frequency)
{
  matrixbit::startTone(frequency);
  buzzerUntil = millis() + 100;
}

void handleButton(const Button &button, bool held)
{
  const bool isA = button.pin == matrixbit::pin::button_a;
  if (inMenu) {
    if (isA) { if (held) menu.previous(); else menu.next(); }
    else {
      if (held) return;
      page = menu.selected(); inMenu = false;
      if (page == Wifi) startWifiScan();
      if (page == Buzzer) beep(880);
      Serial.printf("[MENU] open %s\n", menuItems[page].label);
    }
  } else if (!isA && held) {
    inMenu = true;
    Serial.println("[MENU] back");
  } else if (page == Wifi) {
    if (isA && wifiResultCount && !wifiScanning) {
      wifiSelected = (wifiSelected + wifiResultCount + (held ? -1 : 1)) % wifiResultCount;
    } else if (!isA) startWifiScan();
  } else if (isA) {
    page = (page + pageCount + (held ? -1 : 1)) % pageCount;
    if (page == Wifi) startWifiScan();
  } else {
    beep(page == Buzzer ? 880 : 1320);
    if (page == Rgb || page == Status) {
      rgbStage = 0; rgbChangedAt = millis(); showRgbStage();
    }
    if (page == Status && !wifiScanning) startWifiScan();
  }
}

void updateButton(Button &button, uint32_t now)
{
  const int raw = digitalRead(button.pin);
  if (raw != button.raw) { button.raw = raw; button.changedAt = now; }
  if (raw != button.stable && now - button.changedAt >= 25) {
    button.stable = raw;
    if (raw == LOW) {
      ++button.presses; button.pressedAt = now; button.held = false;
    } else if (!button.held) handleButton(button, false);
  }
  if (button.stable == LOW && button.raw == LOW && !button.held && now - button.pressedAt >= 650) {
    button.held = true;
    handleButton(button, true);
  }
}

void drawDisplay()
{
  if (!oledReady) return;
  if (inMenu) {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    menu.draw(display);
    display.display();
    return;
  }
  display.clearDisplay();
  display.setTextWrap(false);
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  if (page == Wifi) { drawWifi(); return; }
  display.printf("%s", menuItems[page].label);
  display.drawLine(0, 9, 127, 9, SSD1306_WHITE);
  display.setCursor(0, 12);
  const bool imuFresh = imuSampleValid && millis() - lastImuAt < 1500;
  const bool magFresh = magSampleValid && millis() - lastMagAt < 1500;
  if (page == Status) {
    display.printf("A:%s %lu B:%s %lu\n", buttonA.stable == LOW ? "DN" : "UP",
                   static_cast<unsigned long>(buttonA.presses), buttonB.stable == LOW ? "DN" : "UP",
                   static_cast<unsigned long>(buttonB.presses));
    display.printf("RGB: %s\n", rgbStageName());
    display.printf("IMU:%s MAG:%s\n", imuFresh ? "DATA" : "WAIT", magFresh ? "DATA" : "WAIT");
    display.printf("WiFi: %d  I2Cerr:%lu\n", wifiNetworks, static_cast<unsigned long>(matrixbit::i2cErrorCount()));
    display.print("Hold B: menu");
  } else if (page == Imu) {
    if (!imuFresh) {
      display.print("IMU NO DATA\nWaiting for samples");
    } else {
      display.printf("IMU DATA T:%.1fC\n", imuTemperature);
      display.printf("g X%+.2f Y%+.2f\n", acceleration[0], acceleration[1]);
      display.printf("g Z%+.2f\n", acceleration[2]);
      display.printf("gx%+.1f gy%+.1f\n", angularRate[0], angularRate[1]);
      display.printf("gz%+.1f d/s", angularRate[2]);
    }
  } else if (page == Magnetic) {
    display.printf("MAG %s (uT)\n", magFresh ? "DATA" : "NO DATA");
    if (!magFresh) {
      display.print("Waiting for samples");
    } else {
      display.printf("X %+.2f\nY %+.2f\nZ %+.2f\n", magneticField[0], magneticField[1], magneticField[2]);
      display.print("Rotate to check axes");
    }
  } else if (page == Analog) {
    display.printf("ADC39 light: %d\n", lightValue);
    display.printf("ADC36 mic p-p: %d\n", micAmplitude);
    display.print("Cover light / clap\n");
    display.print("Pins need confirming");
  } else if (page == Touch) {
    display.print("TOUCH raw values\n");
    for (size_t i = 0; i < 6; ++i) {
      display.printf("%c:%u%s", touchNames[i], touchValues[i], i % 2 == 1 ? "\n" : " ");
    }
    display.print("Touch pads to compare");
  } else if (page == Rgb) {
    display.printf("%s\n3 LEDs GPIO17\nR/G/B + single white\nB: restart cycle\nHold B: menu", rgbStageName());
  } else if (page == Buzzer) {
    display.print("GPIO16 / LEDC0\n880 Hz, 100 ms\nB: play tone\nHold B: menu");
  }
  // A moving marker plus border makes pixel addressing and refresh visible.
  display.drawRect(0, 10, 128, 54, SSD1306_WHITE);
  display.fillRect((millis() / 150) % 120 + 2, 61, 4, 2, SSD1306_WHITE);
  display.display();
}

void report()
{
  Serial.printf("[STATUS] uptime=%lus A=%lu B=%lu I2C_errors=%lu heap=%u\n",
                static_cast<unsigned long>(millis() / 1000), static_cast<unsigned long>(buttonA.presses),
                static_cast<unsigned long>(buttonB.presses), static_cast<unsigned long>(matrixbit::i2cErrorCount()), ESP.getFreeHeap());
  Serial.printf("[IMU] samples=%lu age_ms=%lu accel_g=(%.3f,%.3f,%.3f) gyro_dps=(%.2f,%.2f,%.2f) temp_C=%.2f\n",
                static_cast<unsigned long>(imuSamples), static_cast<unsigned long>(millis() - lastImuAt),
                acceleration[0], acceleration[1], acceleration[2], angularRate[0], angularRate[1], angularRate[2], imuTemperature);
  Serial.printf("[MAG] samples=%lu age_ms=%lu field_uT=(%.2f,%.2f,%.2f)\n", static_cast<unsigned long>(magSamples),
                static_cast<unsigned long>(millis() - lastMagAt), magneticField[0], magneticField[1], magneticField[2]);
  Serial.printf("[ADC candidate pins] light_GPIO39=%d mic_GPIO36_peak_to_peak=%d\n", lightValue, micAmplitude);
  Serial.print("[TOUCH candidate pins]");
  for (size_t i = 0; i < 6; ++i) Serial.printf(" %c(GPIO%u)=%u", touchNames[i], touchPins[i], touchValues[i]);
  Serial.println();
}

} // namespace

void setup()
{
  Serial.begin(115200);
  delay(300);
  Serial.println("\nMatrix:bit demo");
  Serial.printf("[ESP32] flash=%u bytes CPU=%u MHz\n", ESP.getFlashChipSize(), ESP.getCpuFreqMHz());
  matrixbit::begin();
  matrixbit::beginRGB();
  matrixbit::beginBuzzer();
  oledReady = matrixbit::beginDisplay();
  imuReady = matrixbit::imu().begin();
  magReady = matrixbit::magnetometer().begin();
  WiFi.onEvent([](WiFiEvent_t, WiFiEventInfo_t info) {
    wifiStatus = info.wifi_scan_done.status;
    wifiFinished = true;
  }, ARDUINO_EVENT_WIFI_SCAN_DONE);
  for (uint8_t address = 1; address < 0x7f; ++address) {
    Wire.beginTransmission(address);
    if (Wire.endTransmission() == 0) Serial.printf("[I2C] ACK 0x%02X\n", address);
  }
  Serial.printf("[INIT] OLED=%d IMU=%d (ID=0x%02X) MAG=%d (%s ID=0x%02X)\n",
                oledReady, imuReady, matrixbit::imu().chipId(), magReady,
                matrixbit::magnetometer().name(), matrixbit::magnetometer().chipId());
  showRgbStage();
  rgbChangedAt = millis();
  // OLED all-on then checkerboard verifies the full panel, not just text glyphs.
  if (oledReady) {
    display.clearDisplay();
    display.fillRect(0, 0, 128, 64, SSD1306_WHITE);
    display.display();
    delay(700);
    display.clearDisplay();
    for (int y = 0; y < 64; y += 8) {
      for (int x = 0; x < 128; x += 8) {
        if ((x / 8 + y / 8) % 2 == 0) display.fillRect(x, y, 8, 8, SSD1306_WHITE);
      }
    }
    display.display();
    delay(700);
  }
  beep(880);
  Serial.println("[BUZZER] 880 Hz/100 ms sent; listening confirmation required");
  Serial.println("[HELP] A=next, hold A=previous, B=open/action, hold B=menu; serial n/p/o/b=menu controls, w=WiFi scan, r=invert OLED");
  drawDisplay();
}

void loop()
{
  const uint32_t now = millis();
  static uint32_t sensorsAt = 0;
  static uint32_t displayAt = 0;
  static uint32_t reportAt = 0;
  static uint32_t micAt = 0;
  updateButton(buttonA, now);
  updateButton(buttonB, now);
  updateWifiScan();
  if (buzzerUntil && static_cast<int32_t>(now - buzzerUntil) >= 0) {
    matrixbit::stopTone();
    buzzerUntil = 0;
  }
  if (now - rgbChangedAt >= 900) {
    rgbStage = (rgbStage + 1) % 7;
    rgbChangedAt = now;
    showRgbStage();
  }
  if (micros() - micAt >= 500) {
    micAt = micros();
    const int value = matrixbit::sound();
    micMinimum = min(micMinimum, value);
    micMaximum = max(micMaximum, value);
  }
  if (now - sensorsAt >= 100) { sensorsAt = now; sampleSensors(); }
  if (now - reportAt >= 1000) {
    reportAt = now;
    micAmplitude = micMaximum - micMinimum;
    micMinimum = 4095;
    micMaximum = 0;
    report();
  }
  if (now - displayAt >= 200) { displayAt = now; drawDisplay(); }
  while (Serial.available()) {
    const char command = Serial.read();
    if (command == 'n') handleButton(buttonA, false);
    if (command == 'p') handleButton(buttonA, true);
    if (command == 'o') handleButton(buttonB, false);
    if (command == 'b') handleButton(buttonB, true);
    if (command == 'w') { page = Wifi; inMenu = false; startWifiScan(); }
    if (command == 'r' && oledReady) {
      // Briefly invert all panel pixels so the output can be checked again.
      display.invertDisplay(true);
      delay(250);
      display.invertDisplay(false);
      Serial.println("[OLED] inversion test sent; visual confirmation required");
    }
  }
  delay(1);
}
