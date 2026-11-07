# Waveshare 3.97 experimental release notes

This is a community port of CrossPoint Reader 1.5.0 for the integrated Waveshare
ESP32-S3 e-Paper 3.97 board. It is not an official Waveshare or upstream CrossPoint
release. Installation and recovery steps are in [the board guide](waveshare-397-install.md).

## Tested

- SSD1677 display, orientation used during testing, full/fast cleaning policy.
- Rocker navigation, center click/hold, EPUB discovery, covers, and page turns.
- FAT32 SD over native 4-bit SDMMC and custom SD fonts.
- AXP2101 battery/charging telemetry, USB/battery operation, hardware shutdown.
- Short PWR sleep and BOOT wake; five repeated cycles retained reading position.
- PCF85063 detection and rejection of unset/stopped clock data, QMI8658 detection
  and orientation-dependent samples. These sensor checks do not validate every
  reader feature that could consume them.
- Owner reports over a week of use, one unexplained crash, and 72% battery
  without charging since setup. This is not a measured battery-life specification.

## Limits

- Tested on one board with no visible revision stamp on the photographed side.
- One unexplained crash; the saved report has no panic reason or stack trace.
- One later power-on difficulty recovered after USB/BOOT guidance; more cold-start
  testing across different power states is welcome.
- No CrossPoint speaker, microphone, or environmental-sensor implementation.
- No instrumented sleep-current measurement or full orientation/grayscale/network
  feature qualification. Full backup restoration has not been device-tested.
- Charging settings are fixed at 4.2 V / 400 mA for the main battery, with 3.0 V
  RTC backup-cell charging enabled. Use compatible batteries; see the guide.
- Online update checks still point to upstream CrossPoint. Use the fork's
  Waveshare USB installation package.

Both `waveshare397` and `waveshare397-diag` passed compilation and cppcheck on
2026-08-28. The initial package uses those compiler outputs. A comparison with
the installed reader found differences only in five gzip timestamps, the embedded
ELF hash, and the final checksum/hash; no other application bytes differed.
The packaged file is therefore not byte-identical to the device backup, and
should be treated as an experimental build. Private device backups and SD files
are excluded from release assets.

## Credits

CrossPoint Reader and its contributors provide the reading application.
FreeInk and its contributors provide the SDK and display-driver foundation;
see the included licenses and SDK NOTICE. Waveshare's reference firmware and
schematic, the FolloUp Waveshare branch, and XPowersLib informed this board port.
Links are recorded in [the hardware notes](waveshare-397-support.md#primary-references).
