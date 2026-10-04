#include <Arduino.h>
#include <matrixbit.h>
#include <OledMenu.h>
#include "menu_assets.h"
#include "demo/features.h"

namespace {
using demo::Input;
demo::FeatureSession session;
const demo::Feature *const features[] = {
  &demo::statusFeature, &demo::displayFeature, &demo::imuFeature,
  &demo::magnetometerFeature, &demo::lightFeature, &demo::soundFeature,
  &demo::touchFeature, &demo::rgbFeature, &demo::buzzerFeature, &demo::ioFeature,
  &demo::wifiFeature
};
constexpr size_t featureCount = sizeof(features) / sizeof(features[0]);

void openFeature(const demo::Feature &feature) {
  if (session.active()) Serial.printf("[FEATURE] exit %s\n", session.active()->title);
  session.open(feature);
  Serial.printf("[FEATURE] enter %s\n", feature.title);
}
void openSelected(oledmenu::Menu &menu) { openFeature(*features[menu.selected()]); }
const oledmenu::Icon icons[] = {
  {menu_assets::bitmap_icon_battery, 16, 16},
  {menu_assets::bitmap_icon_knob_over_oled, 16, 16},
  {menu_assets::bitmap_icon_3dcube, 16, 16},
  {menu_assets::bitmap_icon_gps_speed, 16, 16},
  {menu_assets::bitmap_icon_dashboard, 16, 16},
  {menu_assets::bitmap_icon_dashboard, 16, 16},
  {menu_assets::bitmap_icon_knob_over_oled, 16, 16},
  {menu_assets::bitmap_icon_fireworks, 16, 16},
  {menu_assets::bitmap_icon_turbo, 16, 16},
  {menu_assets::bitmap_icon_knob_over_oled, 16, 16},
  {menu_assets::bitmap_icon_parksensor, 16, 16}
};
const oledmenu::Item items[] = {
  {features[0]->title, openSelected, &icons[0]},
  {features[1]->title, openSelected, &icons[1]},
  {features[2]->title, openSelected, &icons[2]},
  {features[3]->title, openSelected, &icons[3]},
  {features[4]->title, openSelected, &icons[4]},
  {features[5]->title, openSelected, &icons[5]},
  {features[6]->title, openSelected, &icons[6]},
  {features[7]->title, openSelected, &icons[7]},
  {features[8]->title, openSelected, &icons[8]},
  {features[9]->title, openSelected, &icons[9]},
  {features[10]->title, openSelected, &icons[10]}
};
static_assert(sizeof(items) / sizeof(items[0]) == featureCount, "Menu and features must match");
oledmenu::Menu menu(items, featureCount);
bool oledReady = false;

struct Button {
  explicit Button(uint8_t pin) : pin(pin) {}
  uint8_t pin;
  bool raw = false, stable = false, held = false;
  uint32_t changedAt = 0, pressedAt = 0;
};
Button buttonA(matrixbit::pin::button_a), buttonB(matrixbit::pin::button_b);

void back() {
  if (session.active()) Serial.printf("[FEATURE] exit %s\n", session.active()->title);
  session.close();
}
void input(bool isA, bool held) {
  if (session.active()) {
    if (!isA && held) back();
    else session.input(isA ? (held ? Input::Previous : Input::Next) : Input::Activate);
  } else if (isA) {
    if (held) menu.previous(); else menu.next();
  } else if (!held) menu.activate();
}
void updateButton(Button &button, uint32_t now) {
  const bool pressed = digitalRead(button.pin) == LOW;
  if (pressed != button.raw) { button.raw = pressed; button.changedAt = now; }
  if (button.raw != button.stable && now - button.changedAt >= 25) {
    button.stable = button.raw;
    if (button.stable) { button.pressedAt = now; button.held = false; }
    else if (!button.held) input(button.pin == buttonA.pin, false);
  }
  if (button.stable && button.raw && !button.held && now - button.pressedAt >= 650) {
    button.held = true;
    input(button.pin == buttonA.pin, true);
  }
}
void draw() {
  if (!oledReady) return;
  auto &screen = matrixbit::display();
  screen.clearDisplay();
  screen.setTextSize(1); screen.setTextWrap(false);
  screen.setTextColor(SSD1306_WHITE); screen.setCursor(0, 0);
  if (const auto *feature = session.active()) {
    screen.print(feature->title);
    screen.drawLine(0, 9, 127, 9, SSD1306_WHITE);
    screen.setCursor(0, 12);
    feature->draw(screen);
    screen.setTextColor(SSD1306_WHITE); screen.setCursor(0, 56);
    screen.print("Hold B: back");
  } else menu.draw(screen);
  screen.display();
}
} // namespace

void setup() {
  Serial.begin(115200);
  matrixbit::begin();
  // Clear peripheral state that can survive an ESP32 reset or firmware upload.
  matrixbit::beginRGB();
  matrixbit::endRGB();
  if (!matrixbit::imu().end()) Serial.println("[IMU] startup disable failed");
  oledReady = matrixbit::beginDisplay();
  buttonA.raw = buttonA.stable = matrixbit::buttonA();
  buttonB.raw = buttonB.stable = matrixbit::buttonB();
  // A button already held during boot must be released before triggering input.
  buttonA.held = buttonA.stable; buttonB.held = buttonB.stable;
  Serial.printf("\nMatrix:bit menu; OLED=%d\n", oledReady);
  Serial.println("[HELP] A=next, hold A=previous, B=open/action, hold B=back; serial n/p/o/b, w=Wi-Fi");
  draw();
}

void loop() {
  const uint32_t now = millis();
  static uint32_t drawnAt = 0;
  updateButton(buttonA, now);
  updateButton(buttonB, now);
  while (Serial.available()) {
    switch (Serial.read()) {
      case 'n': input(true, false); break;
      case 'p': input(true, true); break;
      case 'o': input(false, false); break;
      case 'b': back(); break;
      case 'w':
        menu.select(featureCount - 1);
        openFeature(demo::wifiFeature);
        break;
    }
  }
  // Input may replace the active feature. Only that feature gets updates.
  session.update(millis());
  if (demo::due(millis(), drawnAt, session.active() ? 100 : 33)) draw();
  delay(1);
}
