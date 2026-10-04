#pragma once
#include <Adafruit_SSD1306.h>

// Three-row layout adapted from upir's Arduino OLED Menu (MIT).
// https://github.com/lsongdev/arduino-oled-menu
// License: third_party/arduino-oled-menu/LICENSE
namespace oled_menu {
struct Item { const char *label; const uint8_t *icon; };
class Menu {
public:
  Menu(const Item *items, uint8_t count) : items_(items), count_(count) {}
  uint8_t selected() const { return selected_; }
  void next() { if (count_) selected_ = (selected_ + 1) % count_; }
  void previous() { if (count_) selected_ = (selected_ + count_ - 1) % count_; }
  void draw(Adafruit_SSD1306 &screen) const {
    screen.clearDisplay(); screen.setTextSize(1); screen.setTextWrap(false);
    screen.setTextColor(SSD1306_WHITE);
    if (!count_) return;
    for (uint8_t row = 0; row < 3; ++row) {
      const uint8_t index = (selected_ + count_ + row - 1) % count_;
      const int y = row * 22;
      if (row == 1) screen.drawRoundRect(0, y, 120, 21, 3, SSD1306_WHITE);
      if (items_[index].icon) screen.drawXBitmap(4, y + 2, items_[index].icon, 16, 16, SSD1306_WHITE);
      screen.setCursor(25, y + 6); screen.print(items_[index].label);
    }
    screen.drawRect(124, 0, 4, 64, SSD1306_WHITE);
    const int height = max(3, 62 / count_);
    const int y = 1 + (count_ > 1 ? (62 - height) * selected_ / (count_ - 1) : 0);
    screen.fillRect(125, y, 2, height, SSD1306_WHITE);
  }
private:
  const Item *items_;
  uint8_t count_, selected_ = 0;
};
} // namespace oled_menu
