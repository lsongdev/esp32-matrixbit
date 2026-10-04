#pragma once

#include <stdint.h>

class Adafruit_SSD1306;

namespace demo {
enum class Input { Next, Previous, Activate };

struct Feature {
  const char *title;
  void (*enter)();
  void (*update)(uint32_t now);
  void (*draw)(Adafruit_SSD1306 &screen);
  void (*input)(Input event);
  void (*exit)();
};

// A single owner for feature activation, including serial shortcuts.
class FeatureSession {
public:
  FeatureSession() = default;
  FeatureSession(const FeatureSession &) = delete;
  FeatureSession &operator=(const FeatureSession &) = delete;

  const Feature *active() const { return active_; }
  void open(const Feature &feature) {
    close();
    active_ = &feature;
    if (active_->enter) active_->enter();
  }
  void close() {
    const Feature *previous = active_;
    active_ = nullptr;
    if (previous && previous->exit) previous->exit();
  }
  void update(uint32_t now) {
    if (active_ && active_->update) active_->update(now);
  }
  void input(Input event) {
    if (active_ && active_->input) active_->input(event);
  }
private:
  const Feature *active_ = nullptr;
};

inline bool due(uint32_t now, uint32_t &last, uint32_t period) {
  if (now - last < period) return false;
  last = now;
  return true;
}

extern const Feature statusFeature;
extern const Feature displayFeature;
extern const Feature imuFeature;
extern const Feature magnetometerFeature;
extern const Feature lightFeature;
extern const Feature soundFeature;
extern const Feature touchFeature;
extern const Feature rgbFeature;
extern const Feature buzzerFeature;
extern const Feature ioFeature;
extern const Feature wifiFeature;
} // namespace demo
