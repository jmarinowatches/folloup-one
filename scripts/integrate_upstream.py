"""Apply the integration against the pinned upstream source checkouts."""
from pathlib import Path
import shutil, json
ROOT=Path(__file__).resolve().parents[1]
if '[env:folloup-one]' in (ROOT/'platformio.ini').read_text():
    raise SystemExit('Integration already applied; use the checked-in sources.')
NOTES=ROOT.parent/'git-notes'
ADF=ROOT.parent/'esp-adf/components/esp_codec_dev'
dest=ROOT/'lib/FolloupUpstream'
dest.mkdir(exist_ok=True)
for component in ('audio_hal','gemini_service','task_config'):
    for p in (NOTES/'components'/component).rglob('*'):
        if p.is_file() and p.suffix in ('.cc','.cpp','.h'):
            shutil.copy2(p,dest/p.name)
shutil.copy2(NOTES/'components/recording_service/include/recording_service.h',dest/'recording_service.h')
shutil.copy2(NOTES/'components/board/include/waveshare_board_config.h',dest/'waveshare_board_config.h')
shutil.copy2(NOTES/'LICENSE',dest/'LICENSE')
(dest/'library.json').write_text(json.dumps(dict(name='FolloupUpstream',version='0.1.0',
    build=dict(srcDir='.',includeDir='.',flags=['-I.']))),encoding='utf-8')
# File-backed clips reuse the upstream Gemini upload implementation without loading
# whole recordings into PSRAM. The vector constructor remains available.
p=dest/'recording_service.h';s=p.read_text()
s=s.replace('#include <vector>','#include <vector>\n#include <string>')
s=s.replace('RecordedClip(uint32_t sample_rate_hz, size_t sample_count, ChunkList chunks);',
'''RecordedClip(uint32_t sample_rate_hz, size_t sample_count, ChunkList chunks);
    RecordedClip(uint32_t rate, size_t count, std::string path);
    bool read_failed() const { return read_failed_; }''')
s=s.replace('ChunkList chunks_;','ChunkList chunks_;\n    std::string path_;\n    mutable bool read_failed_ = false;')
p.write_text(s)
codec=ROOT/'lib/esp_codec_dev'
shutil.copytree(ADF,codec,dirs_exist_ok=True)
sources=['esp_codec_dev.c','esp_codec_dev_vol.c','esp_codec_dev_if.c','audio_codec_sw_vol.c',
         'codec_dev_mirror.c','device/common/codec_ref_mgr.c','device/es8311/es8311.c',
         'platform/audio_codec_gpio.c','platform/audio_codec_ctrl_i2c.c',
         'platform/audio_codec_data_i2s.c','platform/esp_codec_dev_os.c']
(codec/'library.json').write_text(json.dumps(dict(name='esp_codec_dev',version='1.5.10-vendored',
    build=dict(srcDir='.',includeDir='include',flags=['-Iinterface','-Idevice/include','-Idevice/priv_include'],
    srcFilter=['-<*>']+['+<'+s+'>' for s in sources]))),encoding='utf-8')
# These paths are also needed by dependent libraries; PlatformIO does not export
# a library's private include flags.
p=ROOT/'platformio.ini';s=p.read_text()
s+='''
[env:folloup-one]
extends = env:waveshare397
board_build.arduino.memory_type = qio_opi
board_build.psram_type = opi
build_flags =
  ${env:waveshare397.build_flags}
  -DFOLLOUP_ONE=1
  -DCONFIG_CODEC_ES8311_SUPPORT=1
  -DCONFIG_CODEC_I2C_BACKWARD_COMPATIBLE=0
  -Ilib/esp_codec_dev/interface
  -Ilib/esp_codec_dev/device/include
  -Ilib/esp_codec_dev/device/priv_include
'''
p.write_text(s)
p=ROOT/'src/activities/ActivityManager.cpp';s=p.read_text()
s=s.replace('#include "ActivityManager.h"','#include "ActivityManager.h"\n#include "folloup/OneActivity.h"\n#include "folloup/NotesEngine.h"')
s=s.replace('replaceActivity(std::make_unique<HomeActivity>(renderer, mappedInput, initialMenuItem, cleanInitialRefresh));',
'''#ifdef FOLLOUP_ONE
  replaceActivity(makeUniqueNoThrow<folloup::OneActivity>(renderer, mappedInput));
#else
  replaceActivity(std::make_unique<HomeActivity>(renderer, mappedInput, initialMenuItem, cleanInitialRefresh));
#endif''')
s=s.replace('bool ActivityManager::preventAutoSleep() const {',
'''bool ActivityManager::preventAutoSleep() const {
#ifdef FOLLOUP_ONE
  if (folloup::notesEngine.busy()) return true;
#endif''')
p.write_text(s)
p=ROOT/'src/folloup/OneActivity.cpp';s=p.read_text().replace('wrapTextWithWordBreaks','wrappedText').replace('B::Left','B::NavPrevious').replace('B::Right','B::NavNext');p.write_text(s)
labels={
'TITLE':'Folloup One','READER':'Reader','NOTES':'AI Notes','TUTOR':'Japanese Tutor',
'WIFI':'Wi-Fi','SETTINGS':'Settings','HIRAGANA':'Hiragana','KATAKANA':'Katakana',
'VOCAB':'Conversation vocabulary','DUE':'All due cards','DIRECTION':'Change practice direction',
'ROMAJI':'Toggle romaji','EN_JP':'English to Japanese','JP_EN':'Japanese to English',
'ROMAJI_ON':'Romaji: on','ROMAJI_OFF':'Romaji: off','CAUGHT_UP':'No cards due. Confirm to return.',
'GRADE':'Up: incorrect   Down / press: correct','REVEAL':'Press to reveal answer',
'STORAGE_ERROR':'SD write failed. Answer has not been saved.',
'CLOCK_WARNING':'Clock unset: offline elapsed time only. Set clock in AI setup.',
'CONTROLS':'Rocker: move   Press: select   Hold center: back',
'RECORD':'Press to record / stop','SYNC':'Synchronize queued recordings',
'CONFIGURE':'Configure AI service / clock','TRANSCRIPT':'Press to read transcript / summary',
'RECORDING':'Recording - press center to save','STOPPING':'Saving recording...',
'EMPTY_NOTES':'No recordings yet','CONFIG_HINT':'Connect to the setup Wi-Fi and open:',
'ERROR':'Operation failed; recording retained','PROCESSING':'Processing with Gemini',
'QUEUED':'Saved offline; queued for synchronization','FINISHED':'Transcription and summary saved',
'IDLE':'Ready','WIFI_REQUIRED':'Connect to Wi-Fi from Home first',
'SAVED':'Settings saved','NEXT_PAGE':'Rocker: scroll   Hold center: back',
}
p=ROOT/'lib/I18n/translations/english.yaml'
with p.open('a',encoding='utf-8') as f:
    f.write('\n# Folloup One\n')
    for key,text in labels.items(): f.write('STR_ONE_'+key+': '+json.dumps(text)+'\n')
print('Upstream services and unified activity routing integrated')
