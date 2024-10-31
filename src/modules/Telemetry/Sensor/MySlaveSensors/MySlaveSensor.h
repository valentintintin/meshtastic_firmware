#ifndef MESHTASTIC_MYSLAVESENSOR_H
#define MESHTASTIC_MYSLAVESENSOR_H

#include "configuration.h"

#if HAS_TELEMETRY && !MESHTASTIC_EXCLUDE_ENVIRONMENTAL_SENSOR

#include "../mesh/generated/meshtastic/telemetry.pb.h"
#include "../TelemetrySensor.h"

#define REG_BATTERY_VOLTAGE 0x0
#define REG_BATTERY_CURRENT 0x2
#define REG_SOLAR_VOLTAGE 0x4
#define REG_SOLAR_CURRENT 0x6
#define REG_TEMPERATURE 0x8
#define REG_PRESSURE 0xA
#define REG_HUMIDITY 0xC
#define REG_PING 0xF0

#define HAS_POWER 0b1
#define HAS_ENVIRONMENT 0b10

class MySlaveSensor : public TelemetrySensor {
public:
    MySlaveSensor(const char *sensorName);
    virtual int32_t runOnce() override;
    virtual bool getMetrics(meshtastic_Telemetry *measurement) override;
protected:
    bool hasEnvironment = false;
    bool hasPower = false;

    virtual void setup() override;

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