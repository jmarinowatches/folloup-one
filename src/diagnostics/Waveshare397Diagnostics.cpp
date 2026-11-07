#if defined(WAVESHARE397_DIAGNOSTICS)

#include <Arduino.h>
#include <BoardConfig.h>
#include <HalClock.h>
#include <HalDisplay.h>
#include <HalGPIO.h>
#include <HalHardwareDiagnostics.h>
#include <HalPowerManager.h>
#include <HalStorage.h>
#include <HalSystem.h>
#include <HalTiltSensor.h>
#include <Logging.h>

#include <cstring>

namespace {

void printHelp() { LOG_INF("DIAG", "Commands: R=rerun report, D=display refresh test, H=help"); }

void reportRtc() {
  Rtc::DateTime dt;
  if (halClock.getDateTime(dt)) {
    LOG_INF("DIAG", "RTC: %04u-%02u-%02u %02u:%02u:%02u weekday=%u", dt.year, dt.month, dt.day, dt.hour, dt.minute,
            dt.second, dt.weekday);
  } else {
    LOG_ERR("DIAG", "RTC: unavailable or oscillator-stop flag is set");
  }
}

void reportImu() {
  Imu::Sample sample;
  if (halTiltSensor.readDiagnosticSample(sample)) {
    LOG_INF("DIAG", "IMU: detected accel=(%.3f,%.3f,%.3f)g gyro=(%.2f,%.2f,%.2f)dps", sample.ax, sample.ay, sample.az,
            sample.gx, sample.gy, sample.gz);
  } else {
    LOG_ERR("DIAG", "IMU: NOT FOUND at 0x6B or 0x6A");
  }
}

void runReport() {
  LOG_INF("DIAG", "=== Waveshare 3.97 hardware report ===");
  LOG_INF("DIAG", "Board profile: %s display=%ux%u I2C SDA=%d SCL=%d", BoardConfig::ACTIVE.name,
          BoardConfig::ACTIVE.displayWidth, BoardConfig::ACTIVE.displayHeight, BoardConfig::ACTIVE.sensors.i2cSda,
          BoardConfig::ACTIVE.sensors.i2cScl);
  HalHardwareDiagnostics::reportI2cDevices();
  HalHardwareDiagnostics::reportPower();
  reportRtc();
  LOG_INF("DIAG", "SD card: %s", Storage.ready() ? "MOUNTED" : "NOT MOUNTED");
  reportImu();
  LOG_INF("DIAG", "Buttons: rotate/press the rocker and tap PWR; edges print below");
  LOG_INF("DIAG", "Display: press D to run the visible waveform test");
}

void runDisplayTest() {
  uint8_t* fb = display.getFrameBuffer();
  const uint32_t size = display.getBufferSize();
  LOG_INF("DIAG", "Display test: FULL white");
  memset(fb, 0xFF, size);
  display.displayBuffer(HalDisplay::FULL_REFRESH);
  LOG_INF("DIAG", "Display test: HALF black");
  memset(fb, 0x00, size);
  display.displayBuffer(HalDisplay::HALF_REFRESH);
  LOG_INF("DIAG", "Display test: 10 alternating FAST updates; update 9 should promote to FULL");
  for (uint8_t pass = 1; pass <= 10; ++pass) {
    memset(fb, (pass & 1u) ? 0xAA : 0x55, size);
    LOG_INF("DIAG", "Display FAST pass %u/10", pass);
    display.displayBuffer(HalDisplay::FAST_REFRESH);
  }
  memset(fb, 0xFF, size);
  display.displayBuffer(HalDisplay::FULL_REFRESH);
  LOG_INF("DIAG", "Display test: PASS if patterns were complete, stable, and border stayed clean");
}

void reportButtonEdges() {
  static constexpr uint8_t BUTTONS[] = {HalGPIO::BTN_BACK, HalGPIO::BTN_CONFIRM, HalGPIO::BTN_LEFT, HalGPIO::BTN_RIGHT,
                                        HalGPIO::BTN_UP,   HalGPIO::BTN_DOWN,    HalGPIO::BTN_POWER};
  for (const uint8_t button : BUTTONS) {
    if (gpio.wasPressed(button)) LOG_INF("DIAG", "Button PRESS: %s", InputManager::getButtonName(button));
    if (gpio.wasReleased(button)) LOG_INF("DIAG", "Button RELEASE: %s", InputManager::getButtonName(button));
  }
}

}  // namespace

void setup() {
  BoardConfig::holdPowerRails();
  delay(250);
  Serial.begin(115200);
  delay(250);
  HalSystem::begin();
  gpio.begin();
  powerManager.begin();
  halClock.begin();
  halTiltSensor.begin();
  const bool sdMounted = Storage.begin();
  LOG_INF("DIAG", "SD initialization: %s", sdMounted ? "PASS" : "FAIL");
  display.begin();
  runReport();
  printHelp();
}

void loop() {
  gpio.update();
  reportButtonEdges();
  if (logSerial.available()) {
    const char command = static_cast<char>(logSerial.read());
    if (command == 'R' || command == 'r') runReport();
    if (command == 'D' || command == 'd') runDisplayTest();
    if (command == 'H' || command == 'h' || command == '?') printHelp();
  }
  delay(10);
}

#endif  // WAVESHARE397_DIAGNOSTICS
