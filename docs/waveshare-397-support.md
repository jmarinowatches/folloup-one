# Waveshare ESP32-S3 e-Paper 3.97 support

Status: experimental community port. Initial hardware bring-up passed on an
unmarked production board on 2026-08-28. By 2026-09-06 the owner reported over a
week of use with one unexplained crash. This is evidence from one board, not a
guarantee for other revisions. Instrumented power measurements and unsupported
peripherals remain future work. See the [installation guide](waveshare-397-install.md).

## Build targets

Build from a path without spaces. The ESP-IDF library-builder stage passes the
absolute project path through CMake and fails for a directory such as
`3D Prints`.

```sh
# CrossPoint Reader firmware
pio run -e waveshare397

# Standalone, non-destructive arrival-day diagnostics
pio run -e waveshare397-diag
pio run -e waveshare397-diag -t upload
pio device monitor -b 115200
```

The target uses `esp32-s3-devkitc1-n16r8` (16 MB flash, 8 MB PSRAM) and
`FREEINK_DEVICE_WAVESHARE397`.

## Board-profile reconciliation

The profile is reconciled with the Waveshare schematic/reference firmware and
the FolloUp Waveshare branch. The values that matter for initial CrossPoint
bring-up are:

| Function | CrossPoint configuration |
| --- | --- |
| Display | SSD1677, 800x480, SCLK 11, MOSI 12, CS 10, DC 9, RST 46, BUSY 3 |
| Controls | Up 4, center/confirm 5, down 6, BOOT/wake 0; active-low |
| Storage | SDMMC 4-bit: CLK 16, CMD 17, D0 15, D1 7, D2 8, D3 18 |
| System I2C | SDA 41, SCL 42, 400 kHz |
| PMIC | AXP2101 at `0x34`, IRQ 38 |
| RTC | PCF85063 at `0x51`, IRQ 45 |
| IMU | QMI8658, preferred `0x6B`, fallback `0x6A`, IRQ 40 |

GPIO0 is deliberately only the ESP deep-sleep wake input. Holding BOOT while
resetting selects the ESP32-S3 ROM downloader. The rotary center button emits
CONFIRM on a click and BACK when held; the PMIC power-key click is also exposed
as a POWER input.

## Implemented hardware support

- AXP2101 rail setup, battery voltage/percentage, charge and VBUS state,
  power-key interrupts, and charge limits. A short PMIC PWR click enters ESP
  deep sleep without cutting the PMIC rails; GPIO0/BOOT wakes the ESP. Holding
  the physical PWR key for six seconds remains the full PMIC shutdown path.
- SSD1677 full (`0xF7`), differential fast/UI (`0xFF`), and half-clean
  (`0xD7`) waveforms. Fast
  refresh is treated as a full-plane operation and automatically promotes every
  ninth fast request to a full refresh to limit accumulated ghosting.
- QMI8658 detection at both known addresses, with `0x6B` tried first.
- PCF85063 date validation, oscillator-stop rejection, disabled CLKOUT, IRQ pin
  setup, and explicit alarm/timer interrupt clearing before sleep.
- A separate serial diagnostics image that scans I2C, reports PMIC/battery,
  reads RTC and IMU, mounts SD, reports input edges, and runs an opt-in display
  waveform test.

Audio (ES8311/NS4150B), microphone input, and the SHTC3 environmental sensor are
outside the reader bring-up. Factory-firmware speaker output passed, but reader
audio is not implemented. Long-session ghosting and sleep-current measurements
remain to be characterized.

## Arrival-day checklist

1. Before flashing, boot the factory image and verify display, buttons, SD,
   speaker, USB charging, and battery operation. Photograph both PCB sides and
   its revision label.
2. Insert a FAT-formatted SD card, connect USB, and start a 115200-baud monitor.
3. Flash `waveshare397-diag`. Save the complete boot report. Expected core I2C
   devices are AXP2101 `0x34`, PCF85063 `0x51`, QMI8658 `0x6B` or `0x6A`, and
   SHTC3 `0x70`; ES8311 `0x18` may also be present.
4. Confirm the PMIC reports a battery and plausible battery/VBUS voltage. Unplug
   USB and press `R`; charging and external-power state should change without a
   reset.
5. Confirm RTC time is plausible. An unset factory RTC may show an error until
   CrossPoint writes a valid time; it must not return corrupt calendar fields.
6. Confirm `SD card: MOUNTED`, then exercise up/down, center click/hold, and the
   PWR key. Each logical press and release should be printed once.
7. Rotate the board through several axes and press `R`; the IMU values should
   change and the acceleration magnitude at rest should be near 1 g.
8. Send `D` only while watching the panel. Verify complete white, black, and
   alternating patterns; note rotation, mirroring, border artifacts, update
   time, flicker, and ghosting. The ninth requested fast update should visibly
   use the full-clean waveform.
9. Flash `waveshare397` and test book discovery/page turns. Finally test sleep,
   BOOT wake, PWR input, battery-only boot, USB-powered boot, charging, and
   shutdown. Current measurements are useful only after these pass.

## Hardware validation record (2026-08-28)

- Board silkscreen: `ESP32-S3-ePaper-3.97`; no revision stamp was visible on the
  photographed component side. A rear-side revision photograph is still
  outstanding.
- Factory firmware passed display, rocker, center control, speaker, battery
  operation, USB operation, charging continuity, and PWR shutdown. Its SD menu
  was not navigated because the UI was in Chinese; the slot was subsequently
  validated in both diagnostic and reader firmware.
- Diagnostic I2C scan found `0x18`, `0x34`, `0x51`, `0x6B`, `0x70`, and `0x7E`.
  The expected ES8311, AXP2101, PCF85063, QMI8658, and SHTC3 addresses were all
  present; `0x7E` remains an additional unidentified responder.
- AXP2101 reported a battery at 4.051-4.087 V, 80-84%, VBUS at 4.910-4.919 V,
  active charging/external power, and 45.1-45.3 C internal temperature. The
  increasing battery voltage/percentage corroborated charging.
- The factory RTC was unset. It first returned the reset-date range and later
  asserted oscillator-stop; the driver rejected the stopped state instead of
  returning corrupt calendar fields.
- A 64 GB card with a 30 GB FAT32 `CROSSPOINT` partition mounted over 4-bit
  SDMMC. The full reader discovered five EPUBs and three `/fonts` families with
  eight `.cpfont` sizes each.
- Up/down, center click/hold, and PMIC PWR produced one logical press/release
  pair each. Full-reader menu navigation, Back, EPUB cover decoding, book
  discovery, forward page turns, and backward page turns passed.
- QMI8658 rest samples changed from approximately
  `(-0.272,-0.045,-0.948) g` to `(-0.011,-0.921,-0.141) g` after a 90-degree
  rotation; the acceleration magnitude was near 1 g.
- SSD1677 white, half-black, and alternating-pattern tests were complete,
  stable, correctly oriented, and border-clean. Full/half refreshes took about
  1.82 s, fast refreshes about 0.55 s, and the ninth requested fast refresh
  promoted to a visible full clean at about 4.02 s.
- Hardware testing exposed two sleep-path integration issues and verified their
  fixes: a PMIC PWR click now enters deep sleep without issuing AXP shutdown,
  and Waveshare GPIO0 wake accepts the wake edge without generic held-button
  verification. Short PWR sleep and brief BOOT wake passed on USB and battery.
- USB-only boot, USB-to-battery takeover, battery-only reading, battery-only
  wake, seven-second PMIC shutdown, and two-second battery cold boot passed.

## Follow-up observations (2026-09-06)

- The owner reports over a week of use and one crash. A saved crash report
  contains startup logs but no panic reason or stack trace; it does not establish
  the cause or whether it belongs to that incident.
- Battery reported 72% without charging since setup. Reading hours and exact
  starting charge were not controlled, so no runtime or sleep-current claim is made.
- The owner observed a cleaning flash on the ninth next-page press, consistent
  with the driver's eight-fast-update limit. Refresh requests from menus or other
  UI changes can also contribute to that count.
- Five consecutive sleep/wake cycles preserved the reading position.
- A full private flash backup and a verified SD file backup were saved before
  further experiments. These contain personal state and are not release assets.
- After SD reinsertion, the owner reported an unsuccessful power-on attempt;
  USB connection followed by BOOT guidance recovered operation. The exact cause
  was not captured, so cold-start reliability across all power states remains
  an item for additional community testing.

## Expected diagnostic transcript

Exact values and the order of extra I2C devices can differ:

```text
[DIAG] SD initialization: PASS
[DIAG] === Waveshare 3.97 hardware report ===
[DIAG] Board profile: WaveShare 3.97 display=800x480 I2C SDA=41 SCL=42
[DIAG] I2C device: 0x34
[DIAG] I2C device: 0x51
[DIAG] I2C device: 0x6B
[DIAG] I2C device: 0x70
[DIAG] I2C scan complete: 4 device(s)
[DIAG] Battery: supported=1 percent=75 mV=3890 charging=1 external=1
[DIAG] AXP2101: battery=1 VBUS=1/5000mV temp=32.0C irq=0x00000000
[DIAG] RTC: 2026-08-22 20:00:00 weekday=6
[DIAG] SD card: MOUNTED
[DIAG] IMU: detected accel=(0.010,-0.020,0.998)g gyro=(0.10,-0.20,0.05)dps
[DIAG] Buttons: rotate/press the rocker and tap PWR; edges print below
[DIAG] Display: press D to run the visible waveform test
[DIAG] Commands: R=rerun report, D=display refresh test, H=help
```

Failure strings such as `AXP2101: NOT FOUND`, `SD card: NOT MOUNTED`, or
`IMU: NOT FOUND at 0x6B or 0x6A` are intentionally explicit. Capture them
together with the preceding I2C list before changing code or wiring.

## Primary references

- [FolloUp Waveshare branch](https://github.com/alxv2016/folloup-sticky/tree/folloup-waveshare)
- [Waveshare product documentation](https://docs.waveshare.com/ESP32-S3-ePaper-3.97)
- [Waveshare reference firmware](https://github.com/waveshareteam/ESP32-S3-ePaper-3.97)
- [Waveshare schematic](https://files.waveshare.com/wiki/ESP32-S3-ePaper-3.97/ESP32-S3_e-Paper-3.97-schematic.pdf)
- [NXP PCF85063A datasheet](https://www.nxp.com/docs/en/data-sheet/PCF85063A.pdf)
- [XPowersLib](https://github.com/lewisxhe/XPowersLib)
- [CrossPoint Reader](https://github.com/crosspoint-reader/crosspoint-reader)
