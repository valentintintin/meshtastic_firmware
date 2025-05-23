#include "Anemometer.h"

#include "configuration.h"

#if !MESHTASTIC_EXCLUDE_ENVIRONMENTAL_SENSOR && HAS_ANEMOMETER

#include "../mesh/generated/meshtastic/telemetry.pb.h"
#include "TelemetrySensor.h"

volatile byte Anemometer::s_nbTrou = 0;
volatile long Anemometer::s_comptageTours = 0;

Anemometer::Anemometer(uint8_t pinOpto, uint8_t pinAlim, float rayon, float facteur)
: TelemetrySensor(meshtastic_TelemetrySensorType_AHT10, "Anemometer"), m_dernierCalcul(0),
m_pinOpto(pinOpto), m_pinAlim(pinAlim), m_rayon(rayon), m_facteur(facteur), m_vitesseKmh(0){}

int32_t Anemometer::runOnce() {
    LOG_INFO("Init sensor: %s", sensorName);

    if (!hasSensor()) {
        return DEFAULT_SENSOR_MINIMUM_WAIT_TIME_BETWEEN_READS;
    }

    setup();
    initialized = true;

    return DEFAULT_SENSOR_MINIMUM_WAIT_TIME_BETWEEN_READS;
}

void Anemometer::setup() {
    pinMode(m_pinOpto, INPUT_PULLUP);
    pinMode(m_pinAlim, OUTPUT);
    digitalWrite(m_pinAlim, HIGH);
    attach();
    m_dernierCalcul = millis();
}

bool Anemometer::getMetrics(meshtastic_Telemetry *measurement) {
    LOG_DEBUG("Anemometer getMetrics");

    detach();
    digitalWrite(m_pinAlim, LOW);

    const float toursParSec = s_comptageTours / ((millis() - m_dernierCalcul) / 1000.0);
    const float distanceParTour = 2 * PI * m_rayon * m_facteur;
    m_vitesseKmh = distanceParTour * toursParSec * 3.6;

    s_comptageTours = 0;
    s_nbTrou = 0;
    m_dernierCalcul = millis();

    digitalWrite(m_pinAlim, HIGH);
    attach();

    measurement->variant.environment_metrics.has_wind_speed = true;
    measurement->variant.environment_metrics.wind_speed = getVitesseMs();

    return true;
}

float Anemometer::getVitesseKmh() const {
    return m_vitesseKmh;
}

float Anemometer::getVitesseMs() const {
    return m_vitesseKmh / 3.6;
}

void Anemometer::onInterrupt() {
    s_nbTrou++;
    if (s_nbTrou >= 4) {
        s_comptageTours++;
        s_nbTrou = 0;
    }
}

void Anemometer::detach() const {
    detachInterrupt(digitalPinToInterrupt(m_pinOpto));
}

void Anemometer::attach() const {
    attachInterrupt(digitalPinToInterrupt(m_pinOpto), Anemometer::onInterrupt, FALLING);
}

#endif