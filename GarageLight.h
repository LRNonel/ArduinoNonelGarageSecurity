#ifndef GARAGE_LIGHT_H
#define GARAGE_LIGHT_H

#include <Arduino.h>

class GarageLight {
public:
    // Constructor (relay pin, status pin, pulse duration in ms)
    GarageLight(uint8_t relayPin, uint8_t statusPin, uint16_t pulseDuration = 200);
    
    // Initialize pins
    void begin();
    
    // Send pulse to toggle light
    void toggleLight();
    
    // Read state via relay contacts (LOW = light ON)
    bool isLightOn() const;
    
    // Pulse auto-reset (call in loop())
    void update();

private:
    uint8_t _relayPin;
    uint8_t _statusPin;
    uint16_t _pulseDuration;
    unsigned long _lastPulseTime = 0;
    bool _pulseActive = false;
};

#endif