#if (HAS_TELEMETRY && (!MESHTASTIC_EXCLUDE_ENVIRONMENTAL_SENSOR || !MESHTASTIC_EXCLUDE_POWER_TELEMETRY))

#include "MySlavePowerSensor.h"

MySlavePowerSensor::MySlavePowerSensor() : MySlaveSensor("MySlavePowerSensor"){}

uint16_t MySlavePowerSensor::getBusVoltageMv() {
    return hasPower ? getBatteryVoltage() : 0;
}

#endif