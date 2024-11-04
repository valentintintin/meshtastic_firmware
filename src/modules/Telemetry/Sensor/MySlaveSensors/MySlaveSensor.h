#ifndef MESHTASTIC_MYSLAVESENSOR_H
#define MESHTASTIC_MYSLAVESENSOR_H

#include "configuration.h"

#if HAS_TELEMETRY && !MESHTASTIC_EXCLUDE_ENVIRONMENTAL_SENSOR

#include "../mesh/generated/meshtastic/telemetry.pb.h"
#include "../TelemetrySensor.h"
#include <time.h>

#define REG_BATTERY_VOLTAGE 0x0
#define REG_BATTERY_CURRENT 0x2
#define REG_SOLAR_VOLTAGE 0x4
#define REG_SOLAR_CURRENT 0x6
#define REG_TEMPERATURE 0x8
#define REG_PRESSURE 0xA
#define REG_HUMIDITY 0xC
#define REG_SECONDS 0xE
#define REG_MINUTES 0x10
#define REG_HOURS 0x12
#define REG_DAYS 0x14
#define REG_MONTHS 0x16
#define REG_YEARS 0x18
#define REG_PING 0x1A

#define HAS_POWER 0b1
#define HAS_ENVIRONMENT 0b10
#define HAS_DATETIME 0b100

class MySlaveSensor : public TelemetrySensor {
public:
    explicit MySlaveSensor(const char *sensorName);
    int32_t runOnce() override;
    bool getMetrics(meshtastic_Telemetry *measurement) override;
    bool getDatetime(tm *datetime);
protected:
    bool hasEnvironment = false;
    bool hasPower = false;
    bool hasRtc = false;

    void setup() override;

    uint16_t getBatteryVoltage();
    uint16_t getBatteryCurrent();
    uint16_t getSolarVoltage();
    uint16_t getSolarCurrent();
    int16_t getTemperature();
    uint16_t getPressure();
    uint16_t getHumidity();
private:
    uint32_t getData(uint8_t what);
};

#endif
#endif //MESHTASTIC_MYSLAVESENSOR_H