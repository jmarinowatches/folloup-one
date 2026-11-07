// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include "activities/Activity.h"
#include "Sm2.h"
#include <array>
#include <ArduinoJson.h>

namespace folloup {
class OneActivity final : public Activity {
 public:
  enum class Screen { Home, Tutor, Card };
  OneActivity(GfxRenderer& r, MappedInputManager& i) : Activity("FolloupOne", r, i) {}
  void onEnter() override;
  void onExit() override;
  void loop() override;
  void render(RenderLock&&) override;
  bool isHomeActivity() const override { return screen == Screen::Home; }
 private:
  Screen screen = Screen::Home;
  int selected = 0, group = 0, card = -1;
  bool revealed = false, reverse = false, romaji = true, armed = false;
  bool storageError = false, clockValid = false;
  uint32_t generation = 0;
  uint64_t fallbackTime = 0;
  unsigned long lastTick = 0;
  std::array<Review, 1024> reviews{};
  JsonDocument deck;
  size_t count = 0;
  uint64_t now();
  bool save();
  void next();
};
void drawJapanese(GfxRenderer& renderer, int x, int y, const char* text);
}
