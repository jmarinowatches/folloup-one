# Folloup One — source and licensing notices

This combined firmware is distributed under GNU GPL version 3. The original
CrossPoint/FreeInk source retains its MIT notices; Folloup components retain
their GPL notices. The original root `LICENSE` is the CrossPoint license, not a
replacement for the combined firmware's `COPYING`.

| Component | Source revision | Terms / use |
| --- | --- | --- |
| CrossPoint Waveshare foundation | `devofreck/crosspoint-reader` — `1b27ccdfdda9c9a94caea934e7d746ebca50cc42` | MIT; EPUB engine, library, bookmarks, fonts, settings, storage, navigation |
| FreeInk SDK | `devofreck/freeink-sdk` — `31a59346c1cad33b8fdc160803755b370596df96` | Original notices preserved; board, SDMMC, display, RTC and PMIC drivers |
| Folloup-Sticky | `alxv2016/folloup-sticky` — `f0b56e969ba3cf7a9d1bed0a77472337abd07ee4` | GPL-3.0; audio codec HAL, WAV clip abstraction, Gemini upload / generation / key settings |
| Espressif esp_codec_dev | `espressif/esp-adf` — `68ce88ad4dcf2cadc3f2d45635de5d4305e51498` | Apache-2.0; vendored component, version 1.6.2; upstream Folloup requested ~1.5.4 |
| CrossMux | `0x1abin/crossmux` — `a6fd569f7552db4ea1a6550ced7bc06568ae0e61` | Inspected as a reference for CJK font residency, board support and app integration; no source copied |
| Noto Sans JP | Google Fonts `ofl/notosansjp` | SIL OFL 1.1; full font and notice in `assets/japanese/`; generated glyph atlas derives from this font |
| New Folloup One integration | `src/folloup/`, tests and scripts | GPL-3.0-only; the original authored starter-deck data is CC0-1.0 |
| wolfSSL | Arduino-wolfSSL 5.7.2 | GPL-2.0-or-later, used under GPL-3.0 for the combined executable; full dependency sources included in source archive |

Dependency manifests, original license files and font notices remain with their
sources. PlatformIO dependencies are pinned in `platformio.ini`; the source
archive also includes the resolved registry-library source trees under
`third_party/`. The SDK is included as actual source, not just a submodule pointer.

The Espressif Arduino platform, core, SDK libraries and toolchain are obtained
from the pinned pioarduino platform release. Their source locations and licenses
are supplied by that platform's package metadata:

- https://github.com/pioarduino/platform-espressif32/releases/tag/55.03.37
- https://github.com/espressif/arduino-esp32/releases/tag/3.3.7
- https://github.com/espressif/esp-idf/tree/v5.5.2

Build tools and compiler executables are not application source and are not
embedded in the source ZIP. PlatformIO installs them automatically. No API key,
Wi-Fi password, personal recording or learning history is included.
