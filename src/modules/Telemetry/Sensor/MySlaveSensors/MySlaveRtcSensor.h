#pragma once
#ifdef HAS_SLAVE_SENSOR

#include "configuration.h"
#include "MySlaveSensor.h"

class MySlaveRtcSensor : public MySlaveSensor {
public:
    MySlaveRtcSensor();
};

#endif