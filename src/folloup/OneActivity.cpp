// SPDX-License-Identifier: GPL-3.0-only
#include "OneActivity.h"
#include "Store.h"
#include "StarterDeck.h"
#include "JapaneseGlyphs.h"
#include "NotesActivity.h"
#include "RecentBooksStore.h"
#include "activities/home/HomeActivity.h"
#include "activities/network/WifiSelectionActivity.h"
#include "activities/ActivityManager.h"
#include "fontIds.h"
#include <HalPowerManager.h>
#include <Memory.h>
#include <Rtc.h>
#include <I18n.h>
#include <WiFi.h>
#include <ctime>
#include <sys/time.h>

namespace folloup {
using enum StrId;
using B = MappedInputManager::Button;
static constexpr StrId homeLabels[] = {STR_ONE_READER,STR_ONE_NOTES,STR_ONE_TUTOR,STR_ONE_WIFI,STR_ONE_SETTINGS};
static constexpr StrId tutorLabels[] = {STR_ONE_HIRAGANA,STR_ONE_KATAKANA,STR_ONE_VOCAB,STR_ONE_DUE,STR_ONE_DIRECTION,STR_ONE_ROMAJI};

void drawJapanese(GfxRenderer& r,int x,int y,const char* text) {
  const auto* p = reinterpret_cast<const uint8_t*>(text);
  const int right = r.getScreenWidth()-24;
  while(*p) {
    uint32_t cp = *p++;
    if(cp >= 0xe0 && cp <= 0xef && p[0] && p[1]) {cp=((cp&15)<<12)|((p[0]&63)<<6)|(p[1]&63);p+=2;}
    else if(cp>=0xc0 && cp<=0xdf && p[0]) {cp=((cp&31)<<6)|(p[0]&63);++p;}
    if(x+40>right) {x=24;y+=52;}
    for(const auto& glyph:japaneseGlyphs) if(glyph.cp==cp) {
      for(int row=0;row<48;++row) for(int col=0;col<40;++col)
        if(glyph.bitmap[row*5+col/8]&(0x80>>(col%8))) r.drawPixel(x+col,y+row);
      break;
    }
    x+=40;
  }
}

uint64_t OneActivity::now() {
  const time_t system = time(nullptr);
  if(system>1700000000) {clockValid=true;fallbackTime=system;lastTick=millis();return system;}
  Rtc rtc;
  Rtc::DateTime dt;
  if(rtc.begin() && rtc.now(dt) && dt.year>=2024 && dt.year<=2099) {
    tm value{};value.tm_year=dt.year-1900;value.tm_mon=dt.month-1;value.tm_mday=dt.day;
    value.tm_hour=dt.hour;value.tm_min=dt.minute;value.tm_sec=dt.second;
    fallbackTime=mktime(&value);lastTick=millis();clockValid=true;
    timeval utc{time_t(fallbackTime),0};settimeofday(&utc,nullptr);
  } else {
    fallbackTime+=(millis()-lastTick)/1000;
    lastTick=millis();clockValid=false;
  }
  return fallbackTime;
}

void OneActivity::onEnter() {
  Activity::onEnter();
  armed=false;
  lastTick=millis();
  Storage.mkdir("/folloup/tutor");
  if(!Storage.exists("/folloup/tutor/starter.json")) {
    JsonDocument seed;
    seed["schema"]=1;
    for(const auto& c:starter) {
      auto entry=seed["cards"].add<JsonObject>();
      entry["id"]=c.id;entry["japanese"]=c.japanese;entry["romaji"]=c.romaji;
      entry["english"]=c.english;entry["group"]=c.group==0?"hiragana":c.group==1?"katakana":"vocabulary";
    }
    auto file=Storage.open("/folloup/tutor/starter.json",O_WRONLY|O_CREAT|O_TRUNC);
    if(!file || serializeJson(seed,file)!=measureJson(seed)) storageError=true;
    file.flush();
  }
  const String raw=Storage.readFile("/folloup/tutor/starter.json");
  if(deserializeJson(deck,raw) || deck["schema"]!=1 || deck["cards"].size()>512) storageError=true;
  else count=deck["cards"].size();
  JsonDocument state;
  if(loadSnapshot("/folloup/tutor/progress",state,generation)) {
    reverse=state["reverse"]|false;romaji=state["romaji"]|true;
    fallbackTime=state["clock"]|uint64_t(0);
    for(size_t i=0;i<count;++i) for(int d=0;d<2;++d) {
      const String key=String(deck["cards"][i]["id"].as<const char*>())+(d?"-en":"-jp");
      const auto v=state["reviews"][key];auto& r=reviews[i*2+d];
      r.due=v["due"]|uint64_t(0);r.ease=v["ease"]|2500u;r.repetitions=v["repetitions"]|0u;
      r.intervalDays=v["interval"]|0u;r.correct=v["correct"]|0u;r.incorrect=v["incorrect"]|0u;
    }
  }
  now();
  requestUpdate();
}

bool OneActivity::save() {
  JsonDocument state;
  state["schema"]=1;state["reverse"]=reverse;state["romaji"]=romaji;state["clock"]=now();
  for(size_t i=0;i<count;++i) for(int d=0;d<2;++d) {
    const String key=String(deck["cards"][i]["id"].as<const char*>())+(d?"-en":"-jp");
    auto v=state["reviews"][key].to<JsonObject>();const auto& r=reviews[i*2+d];
    v["due"]=r.due;v["ease"]=r.ease;v["repetitions"]=r.repetitions;
    v["interval"]=r.intervalDays;v["correct"]=r.correct;v["incorrect"]=r.incorrect;
  }
  return saveSnapshot("/folloup/tutor/progress",state,generation);
}

void OneActivity::onExit() {Activity::onExit();}

void OneActivity::next() {
  card=-1;revealed=false;
  const uint64_t stamp=now();uint64_t earliest=UINT64_MAX;
  for(size_t i=0;i<count;++i) {
    const char* category=deck["cards"][i]["group"]|"";
    const bool match=group==3 || (group==0&&!strcmp(category,"hiragana")) ||
      (group==1&&!strcmp(category,"katakana")) || (group==2&&!strcmp(category,"vocabulary"));
    const auto& r=reviews[i*2+reverse];
    if(match && r.due<=stamp && r.due<earliest) {card=i;earliest=r.due;}
  }
  requestUpdate();
}

void OneActivity::loop() {
  RenderLock stateLock;
  if(!armed) {if(!mappedInput.isPressed(B::Confirm)&&!mappedInput.isPressed(B::Back)) armed=true;return;}
  if(mappedInput.wasReleased(B::Back)) {
    screen=screen==Screen::Card?Screen::Tutor:Screen::Home;selected=0;requestUpdate();return;
  }
  if(screen==Screen::Card) {
    if(card<0) {if(mappedInput.wasReleased(B::Confirm)){screen=Screen::Tutor;requestUpdate();}return;}
    if(!revealed && mappedInput.wasReleased(B::Confirm)) {revealed=true;requestUpdate();return;}
    if(revealed && (mappedInput.wasReleased(B::Confirm)||mappedInput.wasReleased(B::NavNext)||mappedInput.wasReleased(B::NavPrevious))) {
      auto& r=reviews[card*2+reverse];const Review previous=r;
      r=grade(r,mappedInput.wasReleased(B::NavPrevious)?1:4,now());
      if(!save()) {r=previous;storageError=true;requestUpdate();return;}
      storageError=false;next();
    }
    return;
  }
  const int items=screen==Screen::Home?5:6;
  if(mappedInput.wasReleased(B::NavPrevious)) {selected=(selected+items-1)%items;requestUpdate();}
  if(mappedInput.wasReleased(B::NavNext)) {selected=(selected+1)%items;requestUpdate();}
  if(!mappedInput.wasReleased(B::Confirm)) return;
  if(screen==Screen::Home) {
    if(selected==0) startActivityForResult(makeUniqueNoThrow<HomeActivity>(renderer,mappedInput),[this](const auto&){armed=false;});
    if(selected==1) startActivityForResult(makeUniqueNoThrow<NotesActivity>(renderer,mappedInput),[this](const auto&){armed=false;});
    if(selected==2) {screen=Screen::Tutor;selected=0;requestUpdate();}
    if(selected==3) startActivityForResult(makeUniqueNoThrow<WifiSelectionActivity>(renderer,mappedInput),[this](const auto&){armed=false;});
    if(selected==4) activityManager.goToSettings();
  } else {
    if(selected<4) {group=selected;screen=Screen::Card;next();}
    if(selected==4) {reverse=!reverse;storageError=!save();requestUpdate();}
    if(selected==5) {romaji=!romaji;storageError=!save();requestUpdate();}
  }
}

void OneActivity::render(RenderLock&&) {
  renderer.clearScreen();
  const int width=renderer.getScreenWidth(), height=renderer.getScreenHeight();
  renderer.drawText(UI_12_FONT_ID,24,24,I18N.get(screen==Screen::Home?STR_ONE_TITLE:STR_ONE_TUTOR));
  char status[48];snprintf(status,sizeof(status),"%u%%  %s",powerManager.getBatteryPercentage(),WiFi.status()==WL_CONNECTED?"Wi-Fi":"Offline");
  renderer.drawText(SMALL_FONT_ID,24,62,status);
  if(screen!=Screen::Card) {
    const int items=screen==Screen::Home?5:6;
    const int step=std::min(54,(height-230)/items);
    for(int i=0;i<items;++i) {
      const int y=100+i*step;
      if(i==selected) renderer.drawRect(14,y-6,width-28,step-4);
      renderer.drawText(UI_10_FONT_ID,28,y,I18N.get(screen==Screen::Home?homeLabels[i]:tutorLabels[i]));
    }
    if(screen==Screen::Tutor) {
      renderer.drawText(SMALL_FONT_ID,24,height-130,I18N.get(reverse?STR_ONE_EN_JP:STR_ONE_JP_EN));
      renderer.drawText(SMALL_FONT_ID,24,height-104,I18N.get(romaji?STR_ONE_ROMAJI_ON:STR_ONE_ROMAJI_OFF));
    }
  } else if(card<0) renderer.drawText(UI_10_FONT_ID,24,150,tr(STR_ONE_CAUGHT_UP));
  else {
    const auto c=deck["cards"][card];
    // English definitions are wrapped at word boundaries by the renderer helper.
    auto english=[&](int y){auto lines=renderer.wrappedText(UI_10_FONT_ID,c["english"]|"",width-48,3);
      for(const auto& line:lines){renderer.drawText(UI_10_FONT_ID,24,y,line.c_str());y+=30;}};
    if(reverse) english(100);else drawJapanese(renderer,24,100,c["japanese"]|"");
    if(revealed) {
      const int answerY=std::max(220,height/2-70);
      if(reverse) drawJapanese(renderer,24,answerY,c["japanese"]|"");else english(answerY);
      if(romaji) renderer.drawText(UI_10_FONT_ID,24,height-155,c["romaji"]|"");
      renderer.drawText(SMALL_FONT_ID,24,height-105,tr(STR_ONE_GRADE));
    } else renderer.drawText(SMALL_FONT_ID,24,height-105,tr(STR_ONE_REVEAL));
  }
  if(storageError) renderer.drawText(SMALL_FONT_ID,24,height-76,tr(STR_ONE_STORAGE_ERROR));
  else if(screen!=Screen::Home&&!clockValid) renderer.drawText(SMALL_FONT_ID,24,height-76,tr(STR_ONE_CLOCK_WARNING));
  renderer.drawText(SMALL_FONT_ID,24,height-42,tr(STR_ONE_CONTROLS));
  static unsigned refreshCount=0;
  renderer.displayBuffer((++refreshCount%12)==1?HalDisplay::FULL_REFRESH:HalDisplay::FAST_REFRESH);
}
}
