#pragma once

#include "configuration.h"

#if !MESHTASTIC_EXCLUDE_ENVIRONMENTAL_SENSOR && HAS_ANEMOMETER

#include "../mesh/generated/meshtastic/telemetry.pb.h"
#include "TelemetrySensor.h"
#include <Arduino.h>

class Anemometer : public TelemetrySensor {
public:
    Anemometer(uint8_t pinOpto = PIN_ANEMOMETER, uint8_t pinAlim = PIN_ANEMOMETER_ALIM, float rayon = RADIUS_ANEMOMETER, float facteur = FACTOR_ANEMOMETER);
    float getVitesseKmh() const;
    float getVitesseMs() const;

    virtual int32_t runOnce() override;
    virtual bool getMetrics(meshtastic_Telemetry *measurement) override;
protected:
    virtual void setup() override;

private:
    static void onInterrupt();
    void detach() const;
    void attach() const;

    static volatile byte s_nbTrou;
    static volatile long s_comptageTours;

    uint8_t m_pinOpto;
    uint8_t m_pinAlim;
    float m_rayon;
    float m_facteur;
    float m_vitesseKmh;
    unsigned long m_dernierCalcul;
};

#endif