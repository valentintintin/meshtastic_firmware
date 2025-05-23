#pragma once
#include <cstdint>

#include "MeshTypes.h"

#define MAX_FILTERING 4
#define PORTSNUM_TO_SAVE_RECEIVED_TIMING meshtastic_PortNum_NODEINFO_APP, meshtastic_PortNum_TELEMETRY_APP, meshtastic_PortNum_POSITION_APP, meshtastic_PortNum_TRACEROUTE_APP

#define NB_SETTINGS (15 + (4 * MAX_FILTERING))
#define SETTINGS_VERSION 1

enum SettingsType { Boolean, Int8, Int16, Int32, Int64, UInt8, UInt16, UInt32, UInt64, Char, Float, Double, CharString };

typedef struct CustomSettingsFiltering {
    bool enabled;
    meshtastic_PortNum portNum;
    uint8_t hopRelayAllowed;
    uint32_t timeBetweenFramesSec;
    bool onlyBroadcast;
} CustomSettingsFiltering;

typedef struct CustomSettingsHops {
    uint8_t hopsNodeInfo = UINT8_MAX;
    uint8_t hopsPosition = UINT8_MAX;
    uint8_t hopsDeviceTelemetry = 0;
    uint8_t hopsEnvironmentTelemetry = 0;
    uint8_t hopsAirQualityTelemetry = 0;
    uint8_t hopsPowerTelemetry = 0;
    uint8_t hopsNeighbor = 0;
} CustomSettingsHops;

typedef struct CustomSettingsClientHidden {
    bool enabled = true;
    bool onlyForAdmin = true;
    bool changePower = false;
} CustomSettingsClientHidden;

typedef struct CustomSettings {
    uint16_t version = SETTINGS_VERSION;
    bool decrementHops = true;
    bool sendDeviceTelemetry = false;
    bool switchBetweenDeviceAndLocalTelemetry = true;
    CustomSettingsHops hops{};
    CustomSettingsClientHidden clientHidden{};
    bool useFiltering = true;
    CustomSettingsFiltering filtering[MAX_FILTERING] = {
        { true, meshtastic_PortNum_NODEINFO_APP, HOP_MAX, 6 * 60 * 60, true },
        { true, meshtastic_PortNum_POSITION_APP, HOP_MAX, 60 * 60, true },
        { true, meshtastic_PortNum_TELEMETRY_APP, 0, 4 * 60 * 60, true },
        { true, meshtastic_PortNum_TRACEROUTE_APP, HOP_MAX, 5 * 60, false },
    };
} CustomSettings;

typedef struct {
    char name[32 + 1];
    SettingsType type;
    void* pointer;
    uint16_t size;
} SettingsGetSetFunction;