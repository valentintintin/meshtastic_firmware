#ifndef MESHTASTIC_MySlaveEnvironmentSensor_H
#define MESHTASTIC_MySlaveEnvironmentSensor_H

#include "configuration.h"

#if HAS_TELEMETRY && !MESHTASTIC_EXCLUDE_ENVIRONMENTAL_SENSOR

#include "MySlaveSensor.h"

class MySlaveEnvironmentSensor : public MySlaveSensor {
public:
    MySlaveEnvironmentSensor();
};

#endif
#endif //MESHTASTIC_MySlaveEnvironmentSensor_H