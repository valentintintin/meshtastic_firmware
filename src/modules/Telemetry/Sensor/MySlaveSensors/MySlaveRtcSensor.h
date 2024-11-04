#ifndef MESHTASTIC_MySlaveRtcSensor_H
#define MESHTASTIC_MySlaveRtcSensor_H

#include "configuration.h"

#if HAS_TELEMETRY && !MESHTASTIC_EXCLUDE_ENVIRONMENTAL_SENSOR

#include "MySlaveSensor.h"

class MySlaveRtcSensor : public MySlaveSensor {
public:
    MySlaveRtcSensor();
};

#endif
#endif //MESHTASTIC_MySlaveRtcSensor_H