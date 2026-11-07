// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <cstdint>
#include <cstddef>
#include <algorithm>

namespace folloup {
// SM-2, integer ease (1000 == 1.0). UTC seconds avoid timezone/DST shifts.
struct Review {
  uint32_t repetitions = 0, intervalDays = 0, correct = 0, incorrect = 0;
  uint32_t ease = 2500;
  uint64_t due = 0;
};
inline Review grade(Review r, uint8_t quality, uint64_t now) {
  quality = std::min<uint8_t>(quality, 5);
  if (quality < 3) {
    ++r.incorrect;
    r.repetitions = 0;
    r.intervalDays = 0;
    r.due = now + 60;  // relearn a missed card after one minute
  } else {
    ++r.correct;
    r.intervalDays = r.repetitions == 0 ? 1 : r.repetitions == 1 ? 6 :
        std::min<uint64_t>(36500, (uint64_t(r.intervalDays) * r.ease + 500) / 1000);
    ++r.repetitions;
    r.due = now + uint64_t(r.intervalDays) * 86400;
  }
  const int d = 5 - quality;
  r.ease = std::max(1300, int(r.ease) + 100 - d * (80 + d * 20));
  return r;
}
inline uint32_t checksum(const char* data, size_t length) {
  uint32_t result = 2166136261u;
  for (size_t i = 0; i < length; ++i) result = (result ^ uint8_t(data[i])) * 16777619u;
  return result;
}
}
