// SPDX-License-Identifier: GPL-3.0-only
#include "NotesActivity.h"
#include "Store.h"
#include "fontIds.h"
#include <I18n.h>
#include <Rtc.h>
#include <WiFi.h>
#include <gemini_service.h>
#include <esp_random.h>
#include <sys/time.h>
#include <ctime>
#include <Preferences.h>

namespace folloup {
using enum StrId;
using B=MappedInputManager::Button;
namespace {
constexpr const char setupHtml[] PROGMEM=R"HTML(<!doctype html><meta name="viewport" content="width=device-width"><title>Folloup One setup</title>
<style>body{font:18px system-ui;max-width:560px;margin:40px auto;padding:16px}input,button{font:inherit;padding:10px;width:100%;box-sizing:border-box;margin:10px 0}</style>
<h1>Folloup One</h1><p>Recordings stay on your SD card. Synchronization sends audio to Google Gemini. Enter your own Gemini API key from Google AI Studio.</p>
<form method="post" action="/save"><input type="hidden" name="token" value="TOKEN"><label>Gemini API key (leave empty to keep the saved key)<input name="key" type="password" maxlength="256" autocomplete="off"></label>
<label>Gemini audio-capable model resource<input name="model" value="models/gemini-2.5-flash-lite" maxlength="127" pattern="models/[A-Za-z0-9._-]+"></label><input id="epoch" type="hidden" name="epoch"><p>Your computer's UTC clock will also set the device clock.</p><button>Save AI settings and clock</button></form><script>document.getElementById('epoch').value=Math.floor(Date.now()/1000)</script>
<p>Hold the device's center button to close setup. Connect Wi-Fi using Home → Wi-Fi, then open AI Notes and synchronize.</p>)HTML";
StrId statusLabel(NotesEngine::Status s){
  switch(s){
    case NotesEngine::Status::Recording:return STR_ONE_RECORDING;
    case NotesEngine::Status::Saving:return STR_ONE_STOPPING;
    case NotesEngine::Status::Queued:return STR_ONE_QUEUED;
    case NotesEngine::Status::Processing:return STR_ONE_PROCESSING;
    case NotesEngine::Status::Done:return STR_ONE_FINISHED;
    case NotesEngine::Status::Error:return STR_ONE_ERROR;
    default:return STR_ONE_IDLE;
  }
}
}
void NotesActivity::refresh(){
  recordings.clear();recordings.reserve(200);
  auto dir=Storage.open("/folloup/recordings");
  while(auto f=dir.openNextFile()){
    char name[96];f.getName(name,sizeof(name));
    if(String(name).endsWith(".wav")){
      recordings.emplace_back(name[0]=='/'?name:(String("/folloup/recordings/")+name).c_str());
      if(recordings.size()>=200)break;
    }
  }
  if(selected>=int(recordings.size())+3)selected=0;
}
void NotesActivity::onEnter(){
  Activity::onEnter();armed=false;
  notesEngine.begin();refresh();
  if(!gemini_service::HasApiKey())configure();
  requestUpdate();
}
void NotesActivity::onExit(){notesEngine.stop();stopPortal();Activity::onExit();}
void NotesActivity::stopPortal(){
  if(portal){portal->stop();portal.reset();WiFi.softAPdisconnect(true);}
}
void NotesActivity::configure(){
  if(notesEngine.busy())return;
  char id[16];snprintf(id,sizeof(id),"%08lx",(unsigned long)esp_random());
  apName=String("FolloupOne-")+String(id).substring(0,4);
  apPassword=String(id)+String(esp_random(),HEX);token=apPassword+String(esp_random(),HEX);
  WiFi.mode(WiFi.status()==WL_CONNECTED?WIFI_AP_STA:WIFI_AP);
  if(!WiFi.softAP(apName.c_str(),apPassword.c_str()))return;
  portal.reset(new(std::nothrow) WebServer(80));
  if(!portal)return;
  portal->on("/",HTTP_GET,[this](){String html=FPSTR(setupHtml);html.replace("TOKEN",token);portal->sendHeader("Cache-Control","no-store");portal->send(200,"text/html; charset=utf-8",html);});
  portal->on("/save",HTTP_POST,[this](){
    if(portal->arg("token")!=token){portal->send(403,"text/plain","Invalid setup session");return;}
    const String key=portal->arg("key");
    if(key.length()>256){portal->send(400,"text/plain","API key too long");return;}
    const String model=portal->arg("model");
    if(!model.startsWith("models/")||model.length()>127){portal->send(400,"text/plain","Invalid model resource name");return;}
    for(size_t i=7;i<model.length();++i){const char c=model[i];if(!isalnum(c)&&c!='-'&&c!='_'&&c!='.'){
      portal->send(400,"text/plain","Invalid model resource name");return;}}
    Preferences settings;
    if(!settings.begin("gemini",false)){portal->send(500,"text/plain","Cannot open settings");return;}
    const size_t saved=settings.putString("model_name",model);settings.end();
    if(!saved){portal->send(500,"text/plain","Cannot store model");return;}
    if(key.length()){
      gemini_service::SettingsPatch patch;patch.has_api_key=true;patch.api_key=key.c_str();
      const auto result=gemini_service::ApplySettingsPatch(patch);
      if(!result.success){portal->send(400,"text/plain",result.message.c_str());return;}
    }
    const int64_t epoch=strtoll(portal->arg("epoch").c_str(),nullptr,10);
    if(epoch>=1700000000 && epoch<4102444800LL){
      timeval value{time_t(epoch),0};settimeofday(&value,nullptr);
      time_t stamp=epoch;tm utc{};gmtime_r(&stamp,&utc);Rtc rtc;
      Rtc::DateTime dt;dt.year=utc.tm_year+1900;dt.month=utc.tm_mon+1;dt.day=utc.tm_mday;
      dt.hour=utc.tm_hour;dt.minute=utc.tm_min;dt.second=utc.tm_sec;dt.weekday=utc.tm_wday;
      if(rtc.begin())rtc.set(dt);
    }
    portal->sendHeader("Cache-Control","no-store");portal->send(200,"text/plain","Saved. Close setup on the device, connect Wi-Fi from Home, then synchronize.");
  });
  portal->begin();requestUpdate();
}
void NotesActivity::openNote(){
  if(selected<3||selected-3>=int(recordings.size()))return;
  JsonDocument note;uint32_t gen=0;
  const String base=String(recordings[selected-3].c_str())+".note";
  lines.clear();lines.reserve(40);
  if(!loadSnapshot(base.c_str(),note,gen))lines.emplace_back(tr(STR_ONE_QUEUED));
  else {
    std::string text=note["summary"]|"";text+="\n\nAction items:\n";
    for(const auto item:note["actions"].as<JsonArray>()){text+="- ";text+=item.as<std::string>();text+="\n";}
    text+="\nTranscript:\n";text+=note["transcript"]|"";
    // Keep paragraphs/newlines while wrapping; no truncation of the saved data.
    size_t begin=0;
    while(begin<text.size()){
      const size_t end=text.find('\n',begin);const std::string paragraph=text.substr(begin,end==std::string::npos?end:end-begin);
      if(paragraph.empty())lines.emplace_back("");
      else {
        auto wrapped=renderer.wrappedText(UI_10_FONT_ID,paragraph.c_str(),renderer.getScreenWidth()-48,200);
        lines.insert(lines.end(),wrapped.begin(),wrapped.end());
      }
      if(end==std::string::npos)break;begin=end+1;
    }
  }
  detail=true;page=0;requestUpdate();
}
void NotesActivity::loop(){
  RenderLock stateLock;
  if(portal)portal->handleClient();
  const auto state=notesEngine.status();
  if(state!=lastStatus){lastStatus=state;refresh();requestUpdate();}
  if(!armed){if(!mappedInput.isPressed(B::Confirm)&&!mappedInput.isPressed(B::Back))armed=true;return;}
  if(mappedInput.wasReleased(B::Back)){
    if(notesEngine.recording()){notesEngine.stop();requestUpdate();return;}
    if(notesEngine.busy())return;
    if(portal){stopPortal();requestUpdate();return;}
    if(detail){detail=false;requestUpdate();return;}
    finish();return;
  }
  if(portal)return;
  if(detail){
    if(mappedInput.wasReleased(B::NavNext)&&size_t((page+1)*detailRows())<lines.size()){++page;requestUpdate();}
    if(mappedInput.wasReleased(B::NavPrevious)&&page>0){--page;requestUpdate();}
    return;
  }
  const int count=recordings.size()+3;
  if(mappedInput.wasReleased(B::NavPrevious)){selected=(selected+count-1)%count;requestUpdate();}
  if(mappedInput.wasReleased(B::NavNext)){selected=(selected+1)%count;requestUpdate();}
  if(!mappedInput.wasReleased(B::Confirm))return;
  if(notesEngine.recording()){notesEngine.stop();requestUpdate();return;}
  if(notesEngine.busy())return;
  if(selected==0)notesEngine.start();
  else if(selected==1){notesEngine.sync();}
  else if(selected==2)configure();
  else openNote();
  requestUpdate();
}
void NotesActivity::render(RenderLock&&){
  renderer.clearScreen();const int width=renderer.getScreenWidth(),height=renderer.getScreenHeight();
  renderer.drawText(UI_12_FONT_ID,24,24,tr(STR_ONE_NOTES));
  renderer.drawText(SMALL_FONT_ID,24,64,I18N.get(statusLabel(notesEngine.status())));
  if(portal){
    renderer.drawText(UI_10_FONT_ID,24,130,tr(STR_ONE_CONFIG_HINT));
    renderer.drawText(UI_10_FONT_ID,24,180,apName.c_str());
    renderer.drawText(UI_10_FONT_ID,24,225,apPassword.c_str());
    const String url=String("http://")+WiFi.softAPIP().toString();
    renderer.drawText(UI_10_FONT_ID,24,285,url.c_str());
  } else if(detail){
    const int rows=detailRows();
    for(int row=0;row<rows && size_t(page*rows+row)<lines.size();++row)
      renderer.drawText(UI_10_FONT_ID,24,105+row*34,lines[page*rows+row].c_str());
  } else {
    constexpr StrId labels[]={STR_ONE_RECORD,STR_ONE_SYNC,STR_ONE_CONFIGURE};
    const int visible=std::max(3,(height-230)/54);
    const int first=selected<visible?0:selected-visible+1;
    for(int row=0;row<visible && first+row<int(recordings.size())+3;++row){
      const int index=first+row,y=110+row*54;
      if(index==selected)renderer.drawRect(14,y-5,width-28,48);
      const char* label=index<3?I18N.get(labels[index]):strrchr(recordings[index-3].c_str(),'/')+1;
      renderer.drawText(UI_10_FONT_ID,24,y,label);
    }
    if(notesEngine.status()==NotesEngine::Status::Error){
      const auto text=notesEngine.error();auto wrapped=renderer.wrappedText(SMALL_FONT_ID,text.c_str(),width-48,3);
      int y=height-140;for(const auto& line:wrapped){renderer.drawText(SMALL_FONT_ID,24,y,line.c_str());y+=22;}
    }
  }
  renderer.drawText(SMALL_FONT_ID,24,height-40,I18N.get(detail?STR_ONE_NEXT_PAGE:STR_ONE_CONTROLS));
  static unsigned refreshCount=0;
  renderer.displayBuffer((++refreshCount%12)==1?HalDisplay::FULL_REFRESH:HalDisplay::FAST_REFRESH);
}
}
