#include "GarageLight.h"

GarageLight::GarageLight(uint8_t relayPin, uint8_t statusPin, uint16_t pulseDuration) 
    : _relayPin(relayPin), _statusPin(statusPin), _pulseDuration(constrain(pulseDuration, 50, 2000)) {}

void GarageLight::begin() {
    pinMode(_relayPin, OUTPUT);
    digitalWrite(_relayPin, LOW);
    pinMode(_statusPin, INPUT_PULLUP);
}

void GarageLight::toggleLight() {
    if (millis() - _lastPulseTime >= _pulseDuration + 100) { // Anti-bounce guard
        digitalWrite(_relayPin, HIGH);
        _lastPulseTime = millis();
        _pulseActive = true;
    }
}

bool GarageLight::isLightOn() const {
    return digitalRead(_statusPin) == LOW; // LOW = relay closed = light ON
}

void GarageLight::update() {
    if (_pulseActive && (millis() - _lastPulseTime >= _pulseDuration)) {
        digitalWrite(_relayPin, LOW);
        _pulseActive = false;
    }
}

