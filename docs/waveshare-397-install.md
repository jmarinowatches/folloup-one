# CrossPoint on the Waveshare ESP32-S3 e-Paper 3.97

Experimental firmware for the integrated Waveshare board marked
`ESP32-S3-ePaper-3.97`: ESP32-S3, 16 MB flash, 8 MB PSRAM, 800x480 SSD1677 panel,
AXP2101 power management, and a three-way rocker. It is not for a standalone
Waveshare panel, Xteink X3/X4, Seeed Sticky, or another Waveshare reader size.

One unmarked board has passed initial tests and over a week of owner use, with
one unexplained crash. See [validation and pinout](waveshare-397-support.md).

## Before installing

Use a USB-C **data** cable and a FAT32 microSD card. Our tested card is a 64 GB
card with a 30 GB FAT32 partition; exFAT has not been validated for this port.
Formatting erases data, so back up an existing card first. Connect batteries
only according to the board's marked polarity and battery specification.

The current driver configures the main charger for 4.2 V / 400 mA and enables
3.0 V RTC backup-battery charging. Do not attach a non-rechargeable cell to the
RTC connector. Speaker operation was tested only with the factory firmware;
CrossPoint does not currently implement audio on this board.

Save the original firmware before replacing it. With Python and esptool 5
installed, replace `COM4` below with your reader's serial port (Linux/macOS use
their `/dev/...` device path):

```sh
python -m pip install "esptool>=5,<6"
python -m esptool --chip esp32s3 --port COM4 --baud 115200 read-flash --flash-size 16MB 0 0x1000000 original-flash-16mb.bin
```

Keep that image private: a device dump can contain credentials and device state.
Separately copy the entire SD card, including hidden `.crosspoint` contents;
the flash backup does not contain your SD books or reading progress.

## Install the packaged firmware

Download and extract the Waveshare experimental ZIP from this fork's Releases.
Check its files against `SHA256SUMS.txt` (PowerShell: `Get-FileHash <file> -Algorithm SHA256`).
The package contains `reader/` and `diagnostic/` folders with separate flash images.

For your first board test, install diagnostics:

```sh
python -m esptool --chip esp32s3 --port COM4 write-flash 0x0 diagnostic/bootloader.bin 0x8000 diagnostic/partitions.bin 0xe000 diagnostic/boot_app0.bin 0x10000 diagnostic/firmware-waveshare397-diag.bin
```

Open a 115200-baud serial monitor, then follow the
[arrival-day checklist](waveshare-397-support.md#arrival-day-checklist).
`R` reports hardware status; `D` starts the visible display test; `H` shows help.
An unset RTC is expected until a valid time has been set.
Close the serial monitor before flashing again.

Install the reader with:

```sh
python -m esptool --chip esp32s3 --port COM4 write-flash 0x0 reader/bootloader.bin 0x8000 reader/partitions.bin 0xe000 reader/boot_app0.bin 0x10000 reader/firmware-waveshare397.bin
```

These commands replace the bootloader, partition table, OTA selection data, and
first application slot. They do not erase the SD card. Do not substitute a full
device dump or firmware for another board into these commands. An erase-all
operation is not required for the documented install.

Copy DRM-free EPUBs onto the SD card. Optional `.cpfont` families go in
`/fonts/<family>/`. Eject the card from the PC and insert it while the reader is
powered off. Boot the reader and test menu navigation, a book, and sleep/wake.

## Buttons (default mappings)

| Action | Result |
| --- | --- |
| Rocker up | Previous page / previous menu item |
| Rocker down | Next page / next menu item |
| Center click | Select / confirm |
| Center hold | Back |
| Brief PWR press | Sleep |
| Brief BOOT press | Wake from sleep |
| Hold PWR for 6-7 seconds | Full hardware shutdown |
| From full shutdown, hold PWR about 2 seconds, release | Cold boot |

E-paper retains its last image when off. A frozen image alone does not tell you
whether the reader is asleep or powered off. Button remapping can change reader
actions; the hardware PWR shutdown and GPIO0/BOOT behavior are separate.

After eight fast refresh requests the next fast request is promoted to a full
cleaning refresh. A stronger flash around every ninth page turn is expected.
Menus and other screen changes also count, so this is not strictly a page counter.

## If it does not respond

1. For a sleeping reader, briefly press BOOT and wait for the screen to update.
2. For a fully powered-off reader, hold PWR about 2 seconds, release, and wait.
3. If that fails, attach USB power with a known data cable and try a brief BOOT press.
4. If flashing cannot connect automatically, unplug USB, hold BOOT while
   reconnecting USB, then release BOOT and retry the flash command. With a
   battery attached, reconnecting USB may not reset the ESP; if necessary, fully
   shut down first and hold BOOT through the next power-on. Holding BOOT during
   reset selects the ROM downloader and does not boot the reader normally.

The Windows connect/disconnect sound is not proof that a boot completed.
After successful flashing, release BOOT and let the board restart normally.
Retest sleep/wake, USB-to-battery operation, and book resume on your board.

To restore your own original full flash backup:

```sh
python -m esptool --chip esp32s3 --port COM4 write-flash 0x0 original-flash-16mb.bin
```

This overwrites internal flash, including its saved settings. Restore the SD
backup separately if needed. Full-image restoration has not been exercised in
our hardware tests; keep your original backup and a working data cable.

## Building from source

Clone this fork recursively so the pinned Waveshare SDK changes are present.
Use the release tag or the `codex/waveshare-397-port` branch. The GitHub auto-generated
source ZIP does not include submodule contents; use Git instead.

```sh
git clone --recursive --branch codex/waveshare-397-port https://github.com/devofreck/crosspoint-reader.git
cd crosspoint-reader
git submodule update --init --recursive
pio run -e waveshare397 -e waveshare397-diag
pio check -e waveshare397 -e waveshare397-diag
```

Use the pinned SDK commit rather than updating the submodule to upstream main.
Build in a path without spaces; on Windows use a short checkout path such as
`C:/src/crosspoint-reader`. PlatformIO dependencies are declared in `platformio.ini`.
Serial diagnostics use the same board drivers as the reader.

## Updates and bug reports

Use this fork's Waveshare release files and USB flashing instructions for updates.
The inherited online updater still checks upstream CrossPoint; it is not wired
to this fork. Do not select X3/X4 or Sticky binaries in a web flasher.

For a bug report include firmware version, board markings/revision if visible,
battery versus USB power, steps to reproduce, and whether it rebooted or froze.
Preserve `/crash_report.txt` if present. Review serial logs for personal data
before posting. Avoid sharing full flash dumps, credentials, or copyrighted books.
