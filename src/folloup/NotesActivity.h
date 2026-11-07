// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include "activities/Activity.h"
#include "NotesEngine.h"
#include <WebServer.h>
#include <vector>
#include <string>

namespace folloup {
class NotesActivity final : public Activity {
 public:
  NotesActivity(GfxRenderer& r,MappedInputManager& i):Activity("OneNotes",r,i){}
  void onEnter() override;
  void onExit() override;
  void loop() override;
  void render(RenderLock&&) override;
  bool preventAutoSleep() override {return notesEngine.busy()||portal!=nullptr;}
  bool handleHomeGesture() override {notesEngine.stop();return notesEngine.busy();}
 private:
  int selected=0,page=0;
  bool detail=false,armed=false;
  unsigned long statusAt=0;
  NotesEngine::Status lastStatus=NotesEngine::Status::Idle;
  std::vector<std::string> recordings,lines;
  std::unique_ptr<WebServer> portal;
  String apName,apPassword,token;
  void refresh();
  void openNote();
  void configure();
  void stopPortal();
  int detailRows() const {return std::max(1,(renderer.getScreenHeight()-170)/34);}
};
}
