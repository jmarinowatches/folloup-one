# Folloup One

Experimental firmware for the **Waveshare ESP32-S3-ePaper-3.97 Kit (SKU 33811)**:

- Original CrossPoint EPUB reader, library, bookmarks and layout controls
- Offline SD voice recording, queued Gemini transcription, summaries and action items
- Offline Japanese tutor with 291 starter cards, Japanese glyphs and SM-2 reviews

## Status

**Compiled engineering candidate, not a completed or hardware-validated release.**

The ESP32-S3 build passed. All 138 upstream reader host regression tests passed,
along with new scheduling, interrupted-save, deck and partition checks. No physical
board recording/display/power test or live Gemini account workflow has been verified.
Full preservation of the original Folloup interface is unfinished.

Read [the validation report](Folloup-One-Validation.md) for missing features and
hardware/end-to-end acceptance requirements before flashing.

## Downloads and installation

The merged first-install image is in [firmware/Folloup-One-Firmware.bin](firmware/Folloup-One-Firmware.bin).
Download the raw binary. Flash the merged image at **0x0**; do not use the
application-only offset (0x10000) for this file.

Follow [Windows installation and first-run setup](Folloup-One-Installation.md).
Use your own Wi-Fi credentials and Gemini API key; no credentials are in the source.
The starter deck is generated automatically if absent from the microSD card.

## Build

```sh
git clone --recursive https://github.com/jmarinowatches/folloup-one.git
cd folloup-one
python -m pip install platformio==6.1.19 pyyaml pillow
python -m platformio run -e folloup-one -j 4
```

If you cloned without `--recursive`, run `git submodule update --init --recursive`.
The FreeInk SDK is pinned to the exact revision used for the compiled image.
PlatformIO installs the pinned platform and external libraries on the first build.
Use a short project/cache path on Windows and set `PYTHONIOENCODING=utf-8`.

The checked-in glyph and starter-deck headers are ready to build. To regenerate
Japanese assets, first follow [font setup](assets/japanese/README.md).

## Source organization

- `src/folloup/`: unified home, tutor, notes UI and streaming recording/queue service
- `lib/Epub/`, `src/activities/reader/`: original reader implementation
- `lib/FolloupUpstream/`: reused/adapted Folloup audio and Gemini components
- `lib/esp_codec_dev/`: Espressif codec component
- `freeink-sdk/`: pinned upstream hardware/display SDK submodule
- `sdcard/`: starter data to copy to a FAT32 microSD card
- `validation/`: build, test, static analysis and image evidence
- `firmware/`: merged first-install firmware and integrity information

## License

The combined firmware is **GPLv3**; see [COPYING](COPYING).
The original root [LICENSE](LICENSE) is the upstream CrossPoint MIT license.
Original MIT, GPL, Apache and SIL OFL notices remain in force for their components.
See [attribution and pinned revisions](FOLLOUP-ONE-NOTICES.md).

Original reader documentation is retained in [UPSTREAM-README.md](UPSTREAM-README.md).
