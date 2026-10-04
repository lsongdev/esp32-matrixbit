#include "features.h"
#include <matrixbit.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <atomic>

namespace demo {
namespace {
constexpr uint16_t capacity = 24;
struct Network { char ssid[33] = {}; int32_t rssi = 0; bool secured = false; };
Network networks[capacity];
uint16_t count = 0, selected = 0;
int total = 0;
bool scanning = false, radioReady = false;
uint32_t startedAt = 0;
const char *error = nullptr;
wifi_event_id_t eventHandler = 0;
std::atomic<uint32_t> generation{0}, scanStatus{0};
std::atomic<bool> finished{false};

void scan() {
  if (scanning || !radioReady) return;
  count = selected = 0; total = 0; error = nullptr;
  WiFi.scanDelete();
  finished.store(false);
  wifi_scan_config_t config = {};
  config.show_hidden = true;
  config.scan_type = WIFI_SCAN_TYPE_ACTIVE;
  config.scan_time.active.min = 100;
  config.scan_time.active.max = 300;
  const esp_err_t result = esp_wifi_scan_start(&config, false);
  scanning = result == ESP_OK;
  startedAt = millis();
  if (!scanning) error = "Scan start failed";
}
void enter() {
  count = selected = 0; total = 0; scanning = false; error = nullptr;
  const uint32_t token = generation.fetch_add(1) + 1;
  eventHandler = WiFi.onEvent([token](WiFiEvent_t, WiFiEventInfo_t info) {
    if (generation.load() != token) return;
    scanStatus.store(info.wifi_scan_done.status);
    finished.store(true);
  }, ARDUINO_EVENT_WIFI_SCAN_DONE);
  WiFi.persistent(false);
  WiFi.setAutoReconnect(false);
  radioReady = WiFi.mode(WIFI_STA);
  if (!radioReady) { error = "Wi-Fi unavailable"; return; }
  WiFi.disconnect(false, false);
  WiFi.setSleep(false);
  scan();
}
void update(uint32_t now) {
  if (!scanning) return;
  if (!finished.load() && now - startedAt < 20000) return;
  if (!finished.load()) {
    esp_wifi_scan_stop(); error = "Scan timed out";
  } else if (scanStatus.load() != 0) error = "Scan failed";
  else {
    // Arduino's scan-done handler has already populated its result cache.
    total = WiFi.scanComplete();
    if (total < 0) error = "Read results failed";
    else {
      count = min(static_cast<uint16_t>(total), capacity);
      for (uint16_t i = 0; i < count; ++i) {
        String ssid; uint8_t auth; int32_t rssi, channel; uint8_t *bssid;
        if (!WiFi.getNetworkInfo(i, ssid, auth, rssi, bssid, channel)) {
          error = "Read results failed"; count = 0; break;
        }
        ssid.toCharArray(networks[i].ssid, sizeof(networks[i].ssid));
        networks[i].rssi = rssi; networks[i].secured = auth != WIFI_AUTH_OPEN;
      }
    }
  }
  scanning = false;
  if (error) { count = 0; esp_wifi_clear_ap_list(); }
  WiFi.scanDelete();
  Serial.printf("[WIFI] found=%d showing=%u error=%s\n", total, count, error ? error : "none");
  for (uint16_t i = 0; i < count; ++i)
    Serial.printf("  %s %ld dBm %s\n", networks[i].ssid, static_cast<long>(networks[i].rssi),
                  networks[i].secured ? "secured" : "open");
}
void input(Input event) {
  if (event == Input::Activate) { scan(); return; }
  if (scanning || !count) return;
  selected = event == Input::Next ? (selected + 1) % count : (selected + count - 1) % count;
}
void draw(Adafruit_SSD1306 &screen) {
  if (scanning) { screen.print("Scanning...\nB: scan again when done"); return; }
  if (error || !count) {
    screen.println(error ? error : "No networks found");
    screen.print("B: scan again"); return;
  }
  const uint16_t first = selected / 3 * 3;
  for (uint16_t row = 0; row < 3 && first + row < count; ++row) {
    const uint16_t i = first + row;
    const int y = 12 + row * 10;
    if (i == selected) screen.fillRect(0, y - 1, 128, 9, SSD1306_WHITE);
    screen.setTextColor(i == selected ? SSD1306_BLACK : SSD1306_WHITE);
    screen.setCursor(1, y);
    screen.printf("%.20s", networks[i].ssid[0] ? networks[i].ssid : "<hidden>");
  }
  screen.setTextColor(SSD1306_WHITE); screen.setCursor(0, 43);
  screen.printf("%u/%u %lddBm %s", selected + 1, count, static_cast<long>(networks[selected].rssi),
                networks[selected].secured ? "LOCK" : "OPEN");
}
void exit() {
  // Invalidate any callback already queued before unregistering this session.
  generation.fetch_add(1);
  WiFi.removeEvent(eventHandler);
  if (scanning) esp_wifi_scan_stop();
  WiFi.mode(WIFI_OFF);
  WiFi.scanDelete();
  scanning = radioReady = false;
  finished.store(false);
  count = selected = 0; total = 0;
  for (auto &network : networks) network = {};
}
}
const Feature wifiFeature{"Wi-Fi", enter, update, draw, input, exit};
} // namespace demo
