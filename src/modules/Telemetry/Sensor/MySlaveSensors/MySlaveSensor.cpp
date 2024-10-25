#include "MySlaveSensor.h"
#include "Wire.h"

MySlaveSensor::MySlaveSensor(const char *sensorName) : TelemetrySensor(meshtastic_TelemetrySensorType_SENSOR_UNSET, sensorName){}

void MySlaveSensor::setup() {}

int32_t MySlaveSensor::runOnce()
{
    LOG_INFO("Init sensor: %s", sensorName);
    if (!hasSensor()) {
        return DEFAULT_SENSOR_MINIMUM_WAIT_TIME_BETWEEN_READS;
    }

    uint8_t ping = getData(REG_PING);

    LOG_DEBUG("MySlaveSensor status : %d", ping);

    status = ping > 0;

    hasPower = (status & HAS_POWER) > 0;
    hasWeather = (status & HAS_WEATHER) > 0;

    return initI2CSensor();
}

bool MySlaveSensor::getMetrics(meshtastic_Telemetry *measurement)
{
    switch (measurement->which_variant) {
        case meshtastic_Telemetry_environment_metrics_tag:
            measurement->variant.environment_metrics.has_temperature = hasWeather;
            measurement->variant.environment_metrics.has_relative_humidity = hasWeather;
            measurement->variant.environment_metrics.has_barometric_pressure = hasWeather;

            LOG_DEBUG("MySlaveSensor::getMetrics");

            if (hasWeather) {
                LOG_DEBUG("MySlaveSensor::getWeather");
                measurement->variant.environment_metrics.temperature = getTemperature() / 100.0F;
                measurement->variant.environment_metrics.relative_humidity = getHumidity() / 100.0F;
                measurement->variant.environment_metrics.barometric_pressure = getPressure() / 100.0F;
                return true;
            }
            return false;
        case meshtastic_Telemetry_power_metrics_tag:
            measurement->variant.power_metrics.has_ch1_voltage = hasPower;
            measurement->variant.power_metrics.has_ch1_current = hasPower;
            measurement->variant.power_metrics.has_ch2_voltage = hasPower;
            measurement->variant.power_metrics.has_ch2_current = hasPower;
            measurement->variant.power_metrics.has_ch3_voltage = false;
            measurement->variant.power_metrics.has_ch3_current = false;

            if (hasPower) {
                LOG_DEBUG("MySlaveSensor::getPower");
                measurement->variant.power_metrics.ch1_voltage = getBatteryVoltage() / 100.0F;
                measurement->variant.power_metrics.ch1_current = getBatteryCurrent() / 100.0F;
                measurement->variant.power_metrics.ch2_voltage = getSolarVoltage() / 100.0F;
                measurement->variant.power_metrics.ch2_current = getSolarCurrent() / 100.0F;
                return true;
            }
            return false;
    }

    return false;
}

uint32_t MySlaveSensor::getData(uint8_t what) {
    uint32_t value = 0;
    uint8_t length = 2;

    TwoWire *wire = nodeTelemetrySensorsMap[sensorType].second;

    wire->beginTransmission(I2C_SLAVE_ADDRESS);
    wire->write(what);
    wire->endTransmission();
    wire->requestFrom(I2C_SLAVE_ADDRESS, length);

    while (wire->available()) {
        length--;
        value += wire->read() << (length * 8);
    }

    return value;
}

uint16_t MySlaveSensor::getBatteryVoltage() {
    return getData(REG_BATTERY_VOLTAGE);
}

uint16_t MySlaveSensor::getBatteryCurrent() {
    return getData(REG_BATTERY_CURRENT);
}

uint16_t MySlaveSensor::getSolarVoltage() {
    return getData(REG_SOLAR_VOLTAGE);
}

uint16_t MySlaveSensor::getSolarCurrent() {
    return getData(REG_SOLAR_CURRENT);
}

int16_t MySlaveSensor::getTemperature() {
    return (int16_t) getData(REG_TEMPERATURE);
}

uint16_t MySlaveSensor::getPressure() {
    return getData(REG_PRESSURE);
}

uint16_t MySlaveSensor::getHumidity() {
    return getData(REG_HUMIDITY);
}