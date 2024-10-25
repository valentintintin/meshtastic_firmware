#include "MySlavePowerSensor.h"

MySlavePowerSensor::MySlavePowerSensor() : MySlaveSensor("MySlavePowerSensor"){}

uint16_t MySlavePowerSensor::getBusVoltageMv() {
    return hasPower ? getBatteryVoltage() : 0;
}