#pragma once
#if (!MESHTASTIC_EXCLUDE_ENVIRONMENTAL_SENSOR || !MESHTASTIC_EXCLUDE_POWER_TELEMETRY) && defined(HAS_SLAVE_SENSOR)

#include "configuration.h"
#include "MySlaveSensor.h"
#include "../VoltageSensor.h"

class MySlavePowerSensor : public MySlaveSensor, VoltageSensor {
public:
    MySlavePowerSensor();
    uint16_t getBusVoltageMv() override;
};

#endif