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

    ping();

    return initI2CSensor();
}

bool MySlaveSensor::getMetrics(meshtastic_Telemetry *measurement)
{
    if (!status) {
        ping();
    }

    switch (measurement->which_variant) {
        case meshtastic_Telemetry_environment_metrics_tag:
            measurement->variant.environment_metrics.has_temperature = hasEnvironment;
            measurement->variant.environment_metrics.has_relative_humidity = hasEnvironment;
            measurement->variant.environment_metrics.has_barometric_pressure = hasEnvironment;

            if (hasEnvironment) {
                LOG_DEBUG("MySlaveSensor::getEnvironment");
                measurement->variant.environment_metrics.temperature = getTemperature() / 100.0F;
                measurement->variant.environment_metrics.relative_humidity = getHumidity();
                measurement->variant.environment_metrics.barometric_pressure = getPressure();
                return measurement->variant.environment_metrics.temperature != 0
                && measurement->variant.environment_metrics.relative_humidity != 0
                && measurement->variant.environment_metrics.barometric_pressure != 0;
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
                measurement->variant.power_metrics.ch1_voltage = getBatteryVoltage() / 1000.0F;
                measurement->variant.power_metrics.ch1_current = getBatteryCurrent();
                measurement->variant.power_metrics.ch2_voltage = getSolarVoltage() / 1000.0F;
                measurement->variant.power_metrics.ch2_current = getSolarCurrent();
                return measurement->variant.power_metrics.ch1_voltage != 0
                && measurement->variant.power_metrics.ch1_current != 0
                && measurement->variant.power_metrics.ch2_voltage != 0
                && measurement->variant.power_metrics.ch2_current != 0;
            }
            return false;
    }

    return false;
}

uint32_t MySlaveSensor::getData(uint8_t what) {
    uint32_t value = 0;
    uint8_t length = 2;

    TwoWire *wire = nodeTelemetrySensorsMap[sensorType].second;

    wire->beginTransmission(MY_SLAVE_SENSOR_ADDR);
    wire->write(what);
    wire->endTransmission();
    wire->requestFrom(MY_SLAVE_SENSOR_ADDR, length);

    while (wire->available()) {
        length--;
        value += wire->read() << (length * 8);
    }

    return value;
}

uint8_t MySlaveSensor::ping() {
    uint8_t ping = getData(REG_PING);

    status = ping > 0;
    hasPower = (ping & HAS_POWER) == HAS_POWER;
    hasEnvironment = (ping & HAS_ENVIRONMENT) == HAS_ENVIRONMENT;
    hasRtc = (ping & HAS_DATETIME) == HAS_DATETIME;

    LOG_DEBUG("MySlaveSensor status: %d. hasPower: %d, hasEnvironment: %d, hasRtc: %d", ping, hasPower, hasEnvironment, hasRtc);

    return ping;
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

bool MySlaveSensor::getDatetime(tm *datetime) {
    datetime->tm_sec = (int) getData(REG_SECONDS);
    datetime->tm_min = (int) getData(REG_MINUTES);
    datetime->tm_hour = (int) getData(REG_HOURS);
    datetime->tm_mday = (int) getData(REG_DAYS);
    datetime->tm_mon = (int) getData(REG_MONTHS) - 1;
    datetime->tm_year = (int) getData(REG_YEARS) - 1900;

    LOG_DEBUG("MySlaveSensor::getDatetime, year: %d", 1900 + datetime->tm_year);

    return 1900 + datetime->tm_year >= 2024;
}
