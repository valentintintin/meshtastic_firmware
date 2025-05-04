#pragma once
#if !MESHTASTIC_EXCLUDE_ENVIRONMENTAL_SENSOR && defined(HAS_SLAVE_SENSOR)

#include "configuration.h"
#include "MySlaveSensor.h"

class MySlaveEnvironmentSensor : public MySlaveSensor {
public:
    MySlaveEnvironmentSensor();
};

#endif