#if !MESHTASTIC_EXCLUDE_ENVIRONMENTAL_SENSOR && defined(HAS_SLAVE_SENSOR)
#include "MySlaveEnvironmentSensor.h"

MySlaveEnvironmentSensor::MySlaveEnvironmentSensor() : MySlaveSensor("MySlaveEnvironmentSensor"){}
#endif