#include "MovingSensor.h"

MovingSensor::MovingSensor(byte pin, bool activeLevel) 
    : _pin(pin), _activeLevel(activeLevel) {}

void MovingSensor::begin() {
    pinMode(_pin, INPUT);
    _state = false;
    _lastStableState = false;
}

bool MovingSensor::readSensor() {
    return digitalRead(_pin) == _activeLevel;
}

bool MovingSensor::applySensitivity(bool rawState) {
    if(!rawState) return false;
    
    switch(_sensitivity) {
        case Sensitivity::LOWSENS:
            // Only trigger if no recent detections (3 seconds)
            return (seconds() - _lastDetectionTime > 3); 
        case Sensitivity::HIGHSENS:
            // Always trigger on detection
            return true; 
        default: // MEDIUM
            // Trigger unless very recent detection (1 second)
            return (seconds() - _lastDetectionTime > 1);
    }
}

void MovingSensor::handleDetection(bool detected) {
    if(detected && !_lastStableState && _onDetectCallback) {
        _onDetectCallback();
    } 
    else if(!detected && _lastStableState && _onEndCallback) {
        _onEndCallback();
    }
}

bool MovingSensor::update() {
    bool rawState = readSensor();
    bool processedState = applySensitivity(rawState);
    
    // Debounce logic
    if(processedState != _lastStableState) {
        _lastDebounceTime = seconds();
    }
    
    if(seconds() - _lastDebounceTime > _debounceDelay) {
        if(processedState != _state) {
            _state = processedState;
            if(_state) _lastDetectionTime = seconds();
            handleDetection(_state);
            return true;
        }
    }
    
    _lastStableState = processedState;
    return false;
}

bool MovingSensor::isMotionDetected() {
    update();
    return _state && (_detectionMode == DetectionMode::PULSE || 
                     (seconds() - _lastDetectionTime <= _holdDuration));
}

bool MovingSensor::isActive() const {
    return _state;
}

unsigned long MovingSensor::getLastDetectionTime() const {
    return _lastDetectionTime;
}

unsigned long MovingSensor::getInactiveDuration() const {
    return _state ? 0 : seconds() - _lastDetectionTime;
}

void MovingSensor::setDebounceDelay(uint16_t delay) {
    _debounceDelay = delay;
}

void MovingSensor::setHoldDuration(uint16_t duration) {
    _holdDuration = duration;
}

void MovingSensor::setSensitivity(Sensitivity level) {
    _sensitivity = level;
}

void MovingSensor::setActiveLevel(bool level) {
    _activeLevel = level;
}

void MovingSensor::setDetectionMode(DetectionMode mode) {
    _detectionMode = mode;
}

void MovingSensor::setOnDetectCallback(void (*callback)()) {
    _onDetectCallback = callback;
}

void MovingSensor::setOnEndCallback(void (*callback)()) {
    _onEndCallback = callback;
}
