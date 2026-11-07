# Folloup One — Windows installation

Target: **Waveshare ESP32-S3-ePaper-3.97 Kit, SKU 33811**, 16 MB flash,
8 MB octal PSRAM, 800 × 480 SSD1677 display. This is an engineering build.
Read `Folloup-One-Validation.md` before flashing; physical-device acceptance
is separate from compilation and host tests.

## Software

- Windows 10/11 and a USB-C **data** cable.
- Python 3.12 from python.org with the Python Launcher / PATH option enabled.
- For flashing: `py -m pip install esptool==5.1.2`.
- For building source: Git for Windows, then
  `py -m pip install platformio==6.1.19 pyyaml pillow`.

## Back up and connect

1. Copy the existing microSD contents somewhere safe. A firmware flash does not
   normally change the card, but reader settings/cache versions can change.
2. Connect USB-C. In Device Manager, find the board's serial COM port.
3. If the serial port does not appear, hold **BOOT**, reset/power-cycle the
   board, then release BOOT. Holding BOOT at reset selects the ESP32 ROM loader.
4. Close any serial monitor using that COM port. Substitute your port for COM7.
5. Back up the full chip before replacing an existing installation:

```powershell
py -m esptool --chip esp32s3 --port COM7 read-flash 0x0 0x1000000 factory-backup.bin
```

## Flash the merged first-install image

`Folloup-One-Firmware.bin` is a **merged first-install image**, written at
**0x000000**, not an application-only image. Only use this command if that file
was supplied together with a successful validation report and checksum.

```powershell
py -m esptool --chip esp32s3 --port COM7 --baud 460800 write-flash --flash-mode dio --flash-freq 80m --flash-size 16MB 0x0 Folloup-One-Firmware.bin
```

Reset or power-cycle after the flash completes. Reduce the baud rate to 115200
if a cable or USB controller produces transfer errors. Do not use `erase-flash`
as a routine step: it also erases NVS settings and AI credentials.

The complete image layout is:

| Segment | Offset | Source build output |
| --- | --- | --- |
| ESP32-S3 bootloader | `0x000000` | `bootloader.bin` |
| Partition table | `0x008000` | `partitions.bin` |
| Initial OTA-selection data | `0x00E000` | Espressif `boot_app0.bin` |
| Application slot 0 | `0x010000` | `firmware.bin` |

The source partition table defines NVS at `0x9000` (20 KiB), OTA selection at
`0xE000` (8 KiB), two 6.25 MiB app slots at `0x10000` and `0x650000`, SPIFFS
at `0xC90000` and a 64 KiB coredump partition at `0xFF0000`. The application
uses microSD for user data; it does not mount SPIFFS.

For an already-installed **matching Folloup One partition layout**, an
application-only image is located in the source build at
`.pio/build/folloup-one/firmware.bin`; its flash offset is `0x10000`.
Do not apply that offset to the merged first-install image. Do not install an
unmodified CrossPoint release through its inherited OTA menu; it would replace
Folloup One with the reader-only firmware.

## First boot and controls

Insert a FAT32 microSD card before booting. The board has no touchscreen.

- Rocker up/down: navigate menus or pages.
- Center click: select, reveal an answer, or start/stop recording.
- Hold center: back. From reader screens, back to the reader home and then to
  the unified home through the existing navigation.
- PWR uses the reader's existing sleep behavior. While a recording is active,
  software sleep requests stop and flush it before sleeping. A six-second
  hardware PWR hold cuts power directly and cannot be intercepted by firmware.
- BOOT is the ESP32 boot strap and wake control in the reader's board profile.
  Keep it released during normal startup.

Home provides **Reader**, **AI Notes**, **Japanese Tutor**, **Wi-Fi**, and
**Settings**. No reflashing or reboot is required for normal app switching.

## Wi-Fi and Gemini setup

1. Open **Home → Wi-Fi**. Use the inherited reader keyboard and controls to
   select a 2.4 GHz network and save its credentials.
2. Open **AI Notes**. With no saved Gemini key, setup opens automatically.
   Otherwise choose **Configure AI service / clock**.
3. The screen shows a temporary Wi-Fi network name, a randomly generated
   password, and a URL, normally `http://192.168.4.1`.
4. Connect your phone or computer to that temporary network, then open the URL.
   If it says the network has no Internet, keep the connection for setup.
5. Enter a Gemini API key from https://aistudio.google.com/ and an available
   audio-capable Gemini model resource. The default is the upstream project's
   `models/gemini-2.5-flash-lite`; the field is editable because service model
   availability can change. The API must support resumable audio upload and
   `generateContent`. Your account's billing/quota applies.
6. Save. Your browser also supplies UTC time; the device writes it to its RTC.
7. Hold center to close setup. Connect the usual Wi-Fi from Home if necessary.
8. In AI Notes, choose **Synchronize queued recordings**. Audio is sent directly
   to Google Gemini over certificate-validated HTTPS. No separate Folloup server
   or backend deployment is required.

API keys are stored in NVS, not in firmware or the starter files. Leaving the
API-key field empty keeps the saved key. Recordings work without Wi-Fi or a key.
An offline queue is discovered from the SD files. Failed uploads retain their
recordings. Successful transcripts are checkpointed before summary generation,
so a summary retry can reuse the transcript.

## Reader

Copy DRM-free EPUB books to `/books/` (or another folder on the card). The
Reader opens the original CrossPoint home/library; its EPUB engine, recent
books, chapter navigation, bookmarks, font size/layout settings and reading
progress are retained. Use the reader menu for font/layout/bookmark controls.
Commercial DRM-protected EPUBs are not decrypted by this firmware.

## AI Notes

Choose **Press to record / stop**, click center, speak, then click center again
to save. Recordings are mono PCM16 WAV at 16 kHz. The integration streams to SD
and flushes about once per second, with a maximum clip length of ten minutes.
Select a recording in the list to read the summary, action items and transcript.
Use the rocker to scroll. Up to 200 recordings are shown in the on-device list;
all WAV files remain on the card and the synchronization queue scans them.

During a power interruption a `.part` file may remain. On the next AI Notes
initialization, recovery repairs its WAV length from the retained PCM payload
and renames it to `.wav`. The final unflushed audio and filesystem metadata can
still be lost during a hard cut; SD-card controller failures cannot be repaired
by the application. Keep backups of important recordings.

## Japanese Tutor

The 291-card starter deck includes basic and voiced hiragana/katakana, common
contracted sounds, and 83 beginner conversational words/phrases. Japanese glyphs
are embedded, so no font download is needed for this deck.

Choose Hiragana, Katakana, Vocabulary, or All due cards. Click to reveal the
answer, then use **up for incorrect**, **down or center for correct**. This is
self-graded recall, not automatic handwriting/speech recognition. English-to-
Japanese and Japanese-to-English have independent review histories. Romaji is
optional. Correct answers follow SM-2 intervals; an incorrect card returns after
one minute. Every grading result is persisted before moving on.

Set the clock once for calendar-based review scheduling across shutdowns. The
device uses its RTC offline. If the RTC is unset, it warns and uses persisted
elapsed time; time while powered off cannot be inferred without a working RTC.

## SD layout

| Path | Contents |
| --- | --- |
| `/books/*.epub` | Books copied by you |
| `/.crosspoint/` | Original reader cache, settings, bookmarks and progress |
| `/folloup/tutor/starter.json` | Flashcard deck used at runtime |
| `/folloup/tutor/progress.a.json` and `.b.json` | Alternating checksummed learning snapshots |
| `/folloup/recordings/<id>.part` | In-progress / interrupted PCM WAV capture |
| `/folloup/recordings/<id>.wav` | Finalized recording |
| `/folloup/recordings/<id>.wav.note.a.json` and `.b.json` | Transcript / summary / action-item snapshots |

The firmware creates its folders and starter JSON if absent. The source package
also includes `sdcard/`; copying those contents to the card preinstalls the same
deck. The CSV is a human-readable copy, not the runtime database. Preserve card
IDs when editing data. Custom-deck import and glyph generation beyond the
starter set are developer workflows, not an implemented import screen.

## Build from source

Extract the source archive to a short path, for example `C:\folloup-one`.
The source includes the pinned SDK and codec sources. Git is needed for one
pinned JPEG dependency. Internet access is needed on the first build.

```powershell
Set-Location C:\folloup-one
$env:PYTHONIOENCODING = 'utf-8'
$env:PLATFORMIO_CORE_DIR = 'C:\pio'
py -m platformio run -e folloup-one -j 4
```

If your user account cannot write to those example locations, use a short path
that it owns. A `subst` drive mapped to the project parent is another option.
Long build paths can exceed Windows' extraction/command limits. Use the pinned
PlatformIO version above; the newer version encountered a SCons mismatch during
this build. Do not run `scripts/integrate_upstream.py` on the delivered source:
its changes are already applied. The manifest retains upstream board targets,
but **only `folloup-one`** builds this product.

To regenerate the deck and glyph atlas after changing its source data:

```powershell
py scripts/build_japanese.py
py -m platformio run -e folloup-one
```

## Troubleshooting

- **Blank display / boot loop:** use USB power, capture the 115200-baud serial
  log, verify the board identity, then restore the full-chip backup if needed.
  The TG28-labelled schematic and AXP2101-compatible upstream PMIC driver still
  require validation on the actual board revision.
- **SD failure:** use FAT32, reseat the card with the power off, and inspect it on
  a computer. Do not format the card until important data is backed up.
- **No sound captured:** verify that ES8311 acknowledges at I²C address `0x18`;
  the firmware reports missing codec / capture errors. Actual microphone gain
  and signal quality require a device test.
- **AI error:** check Wi-Fi, clock, API key, model availability and quota.
  Recordings remain queued. Select Synchronize again after correcting the issue.
- **Cards reappear / progress missing:** verify the SD card is writable and keep
  both progress snapshots. A failed save is shown and does not advance the card.
- **Clock warning:** run AI setup or the inherited clock synchronization screen.
- **Ghosting:** new screens issue a full refresh periodically; inherited reader
  settings still control its own refresh behavior. Waveform quality needs a
  physical panel check.

No claim of successful flashing, audio capture, network inference or display
operation on your device is implied by the installation instructions.
