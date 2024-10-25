#ifndef MESHTASTIC_MYSLAVEPOWERSENSOR_H
#define MESHTASTIC_MYSLAVEPOWERSENSOR_H

#include "configuration.h"

#if HAS_TELEMETRY && !MESHTASTIC_EXCLUDE_ENVIRONMENTAL_SENSOR

#include "MySlaveSensor.h"
#include "../VoltageSensor.h"

class MySlavePowerSensor : public MySlaveSensor, VoltageSensor {
public:
    MySlavePowerSensor();
    uint16_t getBusVoltageMv() override;
};

#endif
#endif //MESHTASTIC_MYSLAVEPOWERSENSOR_H