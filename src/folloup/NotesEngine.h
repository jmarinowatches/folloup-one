// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <Arduino.h>
#include <atomic>
#include <mutex>
#include <string>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

namespace folloup {
class NotesEngine {
 public:
  enum class Status { Idle, Recording, Saving, Queued, Processing, Done, Error };
  bool begin();
  bool start();
  void stop();
  void sync();
  bool busy() const {return working.load() || capture.load();}
  bool recording() const {return capture.load();}
  Status status() const {return state.load();}
  uint32_t seconds() const {return duration.load();}
  std::string error();
 private:
  TaskHandle_t task = nullptr;
  std::atomic<bool> capture{false},working{false},synchronize{false},recordRequested{false};
  std::atomic<Status> state{Status::Idle};
  std::atomic<uint32_t> duration{0};
  std::mutex mutex;
  std::string lastError;
  static void trampoline(void* self);
  void run();
  void record();
  void processQueue();
  void fail(const std::string& error);
  void recover();
};
extern NotesEngine notesEngine;
}
