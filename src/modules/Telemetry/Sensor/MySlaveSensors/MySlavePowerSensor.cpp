#if (!MESHTASTIC_EXCLUDE_ENVIRONMENTAL_SENSOR || !MESHTASTIC_EXCLUDE_POWER_TELEMETRY) && defined(HAS_SLAVE_SENSOR)
#include "MySlavePowerSensor.h"

MySlavePowerSensor::MySlavePowerSensor() : MySlaveSensor("MySlavePowerSensor"){}

uint16_t MySlavePowerSensor::getBusVoltageMv() {
    return hasPower ? getBatteryVoltage() : 0;
}
#endif