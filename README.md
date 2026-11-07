# Folloup One

An experimental firmware integration for the Waveshare ESP32-S3-ePaper-3.97 Kit (SKU 33811):

- CrossPoint EPUB reader
- Offline SD voice recording and Folloup/Gemini AI notes
- Offline Japanese tutor with 291 starter cards and SM-2 reviews

## Status

**Compiled engineering candidate — not a completed or hardware-validated release.**

The firmware compiled for ESP32-S3. The upstream reader's 138 host regression tests, new scheduling/persistence tests, and firmware image checks passed. No physical-board recording, display, power, or live Gemini workflow has been verified. Full original Folloup interface parity remains unfinished.

Read `Folloup-One-Validation.md` before flashing. The merged first-install image is written at **0x0**; the application-only build uses **0x10000**.

## Licensing and attribution

The combined firmware is GPLv3. CrossPoint/FreeInk MIT notices, Folloup GPL notices, Espressif Apache notices, and Noto font licensing must remain with the sources. See `COPYING` and `FOLLOUP-ONE-NOTICES.md` in the source package.
