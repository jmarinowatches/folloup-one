// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <ArduinoJson.h>
#include <HalStorage.h>
#include "Sm2.h"

namespace folloup {
// Two independent flushed snapshots. Never remove the last good generation.
// A checksum rejects interrupted JSON writes even if they remain parseable.
inline bool readSnapshot(const char* path, JsonDocument& result, uint32_t& generation) {
  const String raw = Storage.readFile(path);
  JsonDocument envelope;
  if (deserializeJson(envelope, raw)) return false;
  const char* data = envelope["data"] | "";
  if (!*data || checksum(data, strlen(data)) != envelope["checksum"].as<uint32_t>()) return false;
  if (deserializeJson(result, data)) return false;
  generation = envelope["generation"] | 0u;
  return true;
}
inline bool loadSnapshot(const char* base, JsonDocument& result, uint32_t& generation) {
  JsonDocument a, b;
  uint32_t ga = 0, gb = 0;
  const bool va = readSnapshot((String(base) + ".a.json").c_str(), a, ga);
  const bool vb = readSnapshot((String(base) + ".b.json").c_str(), b, gb);
  if (!va && !vb) return false;
  if (va && (!vb || ga >= gb)) { result.set(a); generation = ga; }
  else { result.set(b); generation = gb; }
  return true;
}
inline bool saveSnapshot(const char* base, const JsonDocument& doc, uint32_t& generation) {
  String payload;
  if (!serializeJson(doc, payload)) return false;
  JsonDocument envelope;
  envelope["generation"] = generation + 1;
  envelope["data"] = payload;
  envelope["checksum"] = checksum(payload.c_str(), payload.length());
  const String path = String(base) + ((generation + 1) % 2 ? ".a.json" : ".b.json");
  auto file = Storage.open(path.c_str(), O_WRONLY | O_CREAT | O_TRUNC);
  if (!file) return false;
  const size_t expected = measureJson(envelope);
  const size_t written = serializeJson(envelope, file);
  file.flush();
  file.close();
  JsonDocument check;
  uint32_t found = 0;
  if (written != expected || !readSnapshot(path.c_str(), check, found) || found != generation + 1) return false;
  generation = found;
  return true;
}
}
