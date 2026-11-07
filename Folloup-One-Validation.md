# Folloup One — technical validation

**Status: compiled engineering candidate; full project acceptance is not established.**

One ESP32-S3 application contains the original CrossPoint EPUB reader, an
integrated SD voice-notes/Gemini workflow, and an offline Japanese tutor. The
final source revision compiled and linked successfully. No physical device was
connected, no recording was captured from hardware, and no live Gemini request
was made with a user API key. The original Folloup UI also has scope gaps listed
below. This report does not certify a complete, hardware-verified product.

Date: November 7, 2026. Target: Waveshare ESP32-S3-ePaper-3.97 Kit, SKU 33811.

## Build evidence

| Check | Observed result |
| --- | --- |
| Final command | `python -m platformio run -e folloup-one -j 4` |
| PlatformIO Core | 6.1.19 |
| Pinned platform | pioarduino espressif32 55.03.37 |
| Arduino core | 3.3.7 |
| Prebuilt SDK | ESP-IDF v5.5.2-729-g87912cd291 |
| Xtensa compiler | GCC 14.2.0 |
| Final build exit | 0 — SUCCESS |
| Final incremental duration | 82.57 seconds |
| Program usage reported by PlatformIO | 5,877,551 / 6,553,600 bytes — 89.7% |
| Actual application BIN length | 5,878,064 bytes |
| Application-slot space remaining | 675,536 bytes, using actual BIN length |
| Static RAM reported by PlatformIO | 65,204 / 327,680 bytes — 19.9% |
| Merged first-install image length | 5,943,600 bytes |
| Image target / checksum | ESP32-S3 chip ID 9; image checksum and SHA validation hash valid |
| Image flash settings | DIO, 80 MHz, 16 MB |
| PSRAM configuration | `qio_opi` memory type; octal, 80 MHz, boot initialization and malloc integration enabled |

Static RAM is not peak heap consumption. PSRAM presence, free internal heap,
fragmentation, task stack high-water marks and memory use during TLS/recording
have not been measured on hardware. The tutor's review table is bounded at 1,024
direction-specific states; its deck is limited to 512 cards. Dynamic deck and
snapshot data avoid putting large JSON objects on the task stack. The audio
capture buffer is 960 PCM16 samples; file-backed uploads use a bounded chunk
buffer rather than allocating the whole recording.

The Arduino application descriptor inherits `arduino-lib-builder` and its SDK
compile timestamp from the prebuilt core. That descriptor is not the time this
application's source was built. The supplied build log and ELF image hash are
the relevant application-build evidence.

## Image and partition validation

The final image is merged from the generated bootloader, generated partition
table, Espressif OTA-selection initializer, and compiled application. The source
package's packaging script checks the merged application against the exact
application BIN, checks the embedded partition bytes, verifies the ESP32-S3 chip
ID and enforces the application-slot size. SHA-256 values are supplied in
`Folloup-One-SHA256SUMS.txt` and `Folloup-One-Manifest.json`.

| Partition | Offset | Size | End (exclusive) |
| --- | --- | --- | --- |
| NVS | `0x9000` | `0x5000` | `0xE000` |
| OTA data | `0xE000` | `0x2000` | `0x10000` |
| app0 | `0x10000` | `0x640000` | `0x650000` |
| app1 | `0x650000` | `0x640000` | `0xC90000` |
| SPIFFS (not mounted by the app) | `0xC90000` | `0x360000` | `0xFF0000` |
| Coredump | `0xFF0000` | `0x10000` | `0x1000000` |

The partition checks passed: no overlaps, app offsets aligned to 64 KiB, and the
last partition ends at 16 MiB. Flash the **merged image at 0x0**. The source
application-only image uses **0x10000**. These are different files.

## Hardware grounding

Sources inspected:

- [Official board documentation](https://docs.waveshare.com/ESP32-S3-ePaper-3.97)
- [Official schematic](https://files.waveshare.com/wiki/ESP32-S3-ePaper-3.97/ESP32-S3_e-Paper-3.97-schematic.pdf)
- [Official example source](https://github.com/waveshareteam/ESP32-S3-ePaper-3.97), revision `9b12d40731a80213b927ee8a421cae4082952819`
- The exact CrossPoint, FreeInk, Folloup and CrossMux revisions in `FOLLOUP-ONE-NOTICES.md`.

| Resource | Configuration grounded in source / schematic |
| --- | --- |
| ESP32 module | ESP32-S3-WROOM-1-N16R8; 16 MB flash, 8 MB PSRAM |
| E-paper | SSD1677, 800 × 480; SCLK 11, MOSI 12, CS 10, DC 9, reset 46, busy 3 |
| SDMMC | Four-bit; CLK 16, CMD 17, D0 15, D1 7, D2 8, D3 18 |
| Shared I²C | SDA 41, SCL 42; owned by Arduino Wire on I²C controller 0 |
| Audio | ES8311 at 7-bit address 0x18; I²S0, MCLK 13, BCLK 14, WS 47, device-to-codec data 48, codec-to-device data 21 |
| Amplifier | GPIO39, from official audio example and Folloup board config |
| Rocker / BOOT | Up 4, center 5, down 6, BOOT 0 |
| RTC / IMU | PCF85063 and QMI8658 through the existing FreeInk board profile |
| PMIC | Schematic labels TG28; source drivers use the AXP2101-compatible register implementation at 0x34, IRQ38 |

The PMIC label/driver discrepancy is recorded rather than silently assuming a
different board. The upstream reader probes its expected chip ID and configures
power rails/charging. Its correctness for the user's board revision remains a
physical acceptance item. The new audio adapter borrows Wire's existing IDF bus
handle and does not create Folloup's separate bus or SD mount.

## Integration implementation

| Area | Implemented source path / behavior |
| --- | --- |
| Unified home and app switching | `src/folloup/OneActivity.*`; shared ActivityManager, rocker navigation and persistent reader settings |
| Reader | Existing `lib/Epub/` and reader activities retained; no simplified text-viewer replacement |
| Reader features | EPUB library, recent books, page/chapter navigation, bookmarks, font/layout controls, cached progress and inherited network tools remain compiled |
| Reader return to unified home | ActivityManager home routing and a back action on the inherited reader home |
| Audio codec reuse | Original Folloup `audio_codec.cc` and `board_es8311_codec.cc`; Espressif codec component vendored |
| SD capture and recovery | `NotesEngine.cpp`; mono 16 kHz PCM16 WAV, streaming capture, roughly one-second flushes, ten-minute clip limit, `.part` header repair on next initialization |
| Offline queue | Finalized WAV files without a complete note result are rediscovered from SD; failures do not remove WAVs |
| Transcription | Original Folloup Gemini resumable upload and generation code; file-backed RecordedClip adapter |
| Summaries / actions | Text-generation request, validated JSON summary/actions, transcript checkpoint before summary, checksummed result snapshots |
| On-device notes | Recording browser, queued/recording/saving/processing/error status, summary/action/transcript pagination |
| First-run settings | Temporary password-protected AP, browser form for API key, model and UTC clock; no embedded credentials |
| Japanese | 291 runtime SD cards: 104 hiragana/contracted-sound cards, 104 katakana/contracted-sound cards, 83 conversational vocabulary/phrase cards |
| Japanese glyphs | 208 glyphs generated from Noto Sans JP; 49,920 bitmap bytes; every starter-deck character has an atlas entry |
| Learning | SM-2, independent Japanese→English and English→Japanese histories, romaji toggle, correct/incorrect counts, due reviews |
| Persistence | Alternating checksummed generations, flushed and read back before advancing to the next card |
| Power / display | Original reader power/sleep implementation; active notes work prevents auto-sleep; software sleep stops capture; periodic full refreshes in new screens |

The ELF symbol inspection confirms the reader rendering code, OneActivity,
NotesActivity, NotesEngine capture/queue code and Gemini transcription function
are present in this single linked executable. This proves inclusion, not runtime
operation of those functions.

## Tests actually executed

| Test category | Result | What it does / does not establish |
| --- | --- | --- |
| Upstream reader host regression tests | **138 passed, 0 failed** | Original JSON parsers, token boundaries, Unicode composition/bidi, credentials, filenames, XML utilities and related logic; not a physical EPUB/display test |
| New executable SM-2 C++ test | Passed | First intervals, lapse/relearn behavior, ease floor, interval cap, independent histories and checksum vectors |
| New executable persistence C++ test | Passed | Real Store.h with in-memory storage adapter; 80 byte-cut interruptions, generation guard and checksum fallback |
| Python asset test | Passed | Deck schema, IDs, groups, fields, count and glyph coverage |
| Python partition test | Passed | Address bounds, overlap avoidance and alignment for the actual table |
| Cppcheck 2.11 on new application modules | Exit 0, no diagnostics | Warning/performance/portability checks; missing external includes suppressed; not a proof of FreeRTOS timing safety |
| Final ESP32-S3 cross-compile/link | Passed | Final application and firmware BIN generated |
| Esptool image inspection | Passed | Correct ESP32-S3 target, valid image checksum/hash, 16 MB DIO/80 MHz header |

The C++ host tests used Clang 22 through Zig on Windows, with the actual SM-2 and
snapshot headers. The upstream test framework was GoogleTest v1.17.0. Host tests
do not emulate an ESP32, SD card controller, ES8311, or e-paper panel. No full
device simulator was run. The test mocks are used only by host tests and are not
compiled into the firmware.

Build repair history included unavailable shell networking (resolved through a
working runtime), installation of Python/PlatformIO, Windows long-path handling,
a PlatformIO 6.2/SCons mismatch (pinned back to 6.1.19), inherited C3-specific
core-rebuild settings (replaced with a separate S3 environment), the upstream BLE
library exclusion, C++ complete-type inclusion, and translation-enum API usage.
The HTTP macro collision between Arduino WebServer and ESP-IDF was fixed by
keeping the optional IDF portal type out of the public Gemini header. Earlier
failure logs were not represented as successful builds.

## Scope gaps and known limitations

These prevent describing this as full preservation of every upstream Folloup
feature or as a completed, device-accepted release:

1. **The entire original Folloup UI was not ported.** Its timeline/dashboard,
   idea/to-do tagging, vibe-check flow, pinned sticky presentation, task completion
   workflow, playback/system sounds, USB mass-storage export, and its own
   device-management portal are not preserved as that UI. The integrated notes
   screen implements recording, queueing, processing and reading results. Shared
   reader Wi-Fi, file transfer and settings supply some management capabilities.
2. **No physical hardware or live AI test was performed.** Compiled capture and
   upload code is not evidence of an actual microphone recording, valid signal
   levels, Wi-Fi throughput, successful Gemini account access, or displayed output.
3. Kana/vocabulary grading is self-assessment. Pronunciation playback, AI chat,
   handwriting recognition, Duolingo import and a custom-deck import screen are
   not implemented. These were optional in the requested product.
4. The embedded Japanese atlas covers the starter deck, not arbitrary Japanese
   books or arbitrary AI-generated Japanese. The reader's original SD-font
   support remains available for books needing broader Unicode coverage.
5. The on-device recording list shows 200 files. Recordings are capped at ten
   minutes each; longer recording, deletion, and storage-full management need
   additional product work. Original audio files remain accessible on the card.
6. FAT32 and consumer SD controllers cannot guarantee recovery from all power
   cuts. Two snapshots protect against partial application writes; actual media
   behavior and up to roughly the last unflushed second of audio remain risks.
7. A valid RTC is needed to count days while powered off. Without one, the tutor
   warns and uses persisted elapsed time. API keys are saved to ordinary NVS;
   this build does not provision flash encryption or secure boot.
8. The inherited reader OTA menu targets upstream reader releases. It is not a
   Folloup One release channel; using it can replace the integrated firmware.
9. New strings have English fallback. Full product localization, formal
   accessibility review, and long-running endurance testing are outstanding.

## Physical / end-to-end acceptance still required

- Verify actual module flash/PSRAM detection, PMIC identity/rails/charging,
  USB-powered and battery-powered boots, sleep/wake, and current draw.
- Verify panel orientation, partial/full waveforms, ghosting and input mapping.
- Open representative EPUBs, change fonts, set bookmarks, reboot and resume.
- Record the microphone offline, inspect WAV sample format/amplitude, stop it,
  simulate a power cut during capture, then confirm recovery and playback on a PC.
- Provide a valid Gemini key/model, synchronize a recording, verify transcript,
  summary and actions, then test dropped Wi-Fi, quota/auth failures and retries.
- Verify flashcards render, both directions schedule independently, and an
  answer survives reboot; repeat with an RTC-only offline interval.
- Stress SD-full/read-only/removal cases and observe heap/stack use while
  switching applications and synchronizing repeatedly.

The remaining engineering work for full upstream-feature parity is to port the
specific original Folloup UI/services in item 1 onto shared storage, network,
power and display ownership, then perform these physical/end-to-end checks and
fix any failures they expose. The supplied source and image are a substantive
compiled integration candidate, not a claim that those remaining tasks are done.
