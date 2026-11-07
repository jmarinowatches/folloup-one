#include "HalHardwareDiagnostics.h"

#include <BatteryMonitor.h>
#include <BoardConfig.h>
#include <Logging.h>
#include <Wire.h>

#if FREEINK_CAP_AXP2101
#include <Axp2101.h>
#endif

namespace HalHardwareDiagnostics {

void reportI2cDevices() {
  const auto& sensors = BoardConfig::ACTIVE.sensors;
  Wire.begin(sensors.i2cSda, sensors.i2cScl, sensors.i2cHz);
  uint8_t count = 0;
  for (uint8_t addr = 1; addr < 0x7F; ++addr) {
    Wire.beginTransmission(addr);
    if (Wire.endTransmission() == 0) {
      LOG_INF("DIAG", "I2C device: 0x%02X", addr);
      ++count;
    }
  }
  LOG_INF("DIAG", "I2C scan complete: %u device(s)", count);
}

void reportPower() {
  BatteryMonitor battery;
  const auto status = battery.readStatus();
  LOG_INF("DIAG", "Battery: supported=%u percent=%s%u mV=%s%u charging=%s%u external=%s%u", status.supported,
          status.percentageKnown ? "" : "?", status.percentage, status.millivoltsKnown ? "" : "?", status.millivolts,
          status.chargingKnown ? "" : "?", status.charging, status.externalPowerKnown ? "" : "?", status.externalPower);
#if FREEINK_CAP_AXP2101
  freeink::Axp2101::Status axp;
  if (freeink::Axp2101::readStatus(axp)) {
    LOG_INF("DIAG", "AXP2101: battery=%u VBUS=%u/%umV temp=%.1fC irq=0x%08llX", axp.batteryPresent, axp.vbusPresent,
            axp.vbusMillivolts, axp.temperatureC, static_cast<unsigned long long>(axp.irqStatus));
  } else {
    LOG_ERR("DIAG", "AXP2101: NOT FOUND");
  }
#endif
}

}  // namespace HalHardwareDiagnostics
