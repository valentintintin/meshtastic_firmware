#pragma once
#ifdef HAS_SLAVE_SENSOR

#include "configuration.h"
#include "../mesh/generated/meshtastic/telemetry.pb.h"
#include "../TelemetrySensor.h"

#define REG_BATTERY_VOLTAGE 0x0
#define REG_BATTERY_CURRENT 0x1
#define REG_SOLAR_VOLTAGE 0x2
#define REG_SOLAR_CURRENT 0x3
#define REG_TEMPERATURE 0x4
#define REG_PRESSURE 0x5
#define REG_HUMIDITY 0x6
#define REG_SECONDS 0x7
#define REG_MINUTES 0x8
#define REG_HOURS 0x9
#define REG_DAYS 0xA
#define REG_MONTHS 0xB
#define REG_YEARS 0xC

#define REG_PING 0x1A

#define REG_COMMAND_RECEIVE_FROM_SLAVE 0x20
#define REG_COMMAND_RESPONSE_TRANSMIT_FROM_MASTER 0x21
#define REG_COMMAND_RECEIVED_FROM_MASTER 0x22
#define REG_COMMAND_RESPONSE_TO_MASTER 0x23

#define HAS_POWER 0b1
#define HAS_ENVIRONMENT 0b10
#define HAS_DATETIME 0b100

#define BUFFER_LENGTH 255

class MySlaveSensor : public TelemetrySensor {
public:
    explicit MySlaveSensor(const char *sensorName);
    int32_t runOnce() override;
    bool getMetrics(meshtastic_Telemetry *measurement) override;

    bool getDatetime(tm *datetime);
    uint16_t getBatteryVoltage();
    uint16_t getBatteryCurrent();
    uint16_t getSolarVoltage();
    uint16_t getSolarCurrent();
    int16_t getTemperature();
    uint16_t getPressure();
    uint16_t getHumidity();
    uint8_t ping();
    // void sendCommandToSlave(const char *command);
    // void receiveCommandOrResponse(char *commandOrResponse) const;

    char commandResponseFromMaster[BUFFER_LENGTH + 1];
    char commandReceivedFromMaster[BUFFER_LENGTH + 1];
protected:
    bool hasEnvironment = false;
    bool hasPower = false;
    bool hasRtc = false;

    // char commandToSendToMaster[BUFFER_LENGTH + 1];
    // char commandResponseToSendToMaster[BUFFER_LENGTH + 1];

    // char buffer[BUFFER_LENGTH + 1]{};

    void setup() override;
private:
    uint32_t getData(uint8_t what);
};

#endif