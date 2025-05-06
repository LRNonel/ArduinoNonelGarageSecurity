#include "SmokeRelay.h"

SmokeRelay::SmokeRelay(byte pin, RelayType type) 
    : _pin(pin), _relayType(type) {
    init();
}

void SmokeRelay::init() {
    pinMode(_pin, INPUT_PULLUP);
    _state = readHardwareState();
    _lastStableState = _state;
    _lastStateChangeTime = millis();
}

bool SmokeRelay::readHardwareState() {
    return digitalRead(_pin);
}

bool SmokeRelay::applyDebounce(bool rawState) {
    if(rawState != _lastStableState) {
        _lastDebounceTime = millis();
    }
    
    if(millis() - _lastDebounceTime > _debounceDelay) {
        return rawState;
    }
    return _state;
}

void SmokeRelay::notifyCallbacks(SmokeStatus previousStatus, SmokeStatus newStatus) {
    if(newStatus != previousStatus) {
        switch(newStatus) {
            case SmokeStatus::DETECTED:
                if(_detectionCallback) _detectionCallback(newStatus);
                break;
            case SmokeStatus::CLEAR:
                if(_clearCallback) _clearCallback(newStatus);
                break;
            case SmokeStatus::ERROR:
                if(_errorCallback) _errorCallback(newStatus);
                break;
        }
    }
}

bool SmokeRelay::update() {
    SmokeStatus previousStatus = getStatus();
    bool rawState = readHardwareState();
    bool newState = applyDebounce(rawState);
    bool stateChanged = false;
    
    if(newState != _state) {
        _state = newState;
        _lastStateChangeTime = millis();
        _lastStableState = _state;
        stateChanged = true;
        notifyCallbacks(previousStatus, getStatus());
    }
    
    return stateChanged;
}

SmokeRelay::SmokeStatus SmokeRelay::getStatus() const {
    if(millis() - _lastStateChangeTime > _errorThreshold) {
        return SmokeStatus::ERROR;
    }
    return isActive() ? SmokeStatus::DETECTED : SmokeStatus::CLEAR;
}

bool SmokeRelay::isActive() const {
    return (_relayType == RelayType::NORMALLY_CLOSED) ? !_state : _state;
}

bool SmokeRelay::isSmokeDetected() const {
    return getStatus() == SmokeStatus::DETECTED;
}

bool SmokeRelay::isError() const {
    return getStatus() == SmokeStatus::ERROR;
}

unsigned long SmokeRelay::getCurrentStateDuration() const {
    return millis() - _lastStateChangeTime;
}

unsigned long SmokeRelay::getLastChangeTime() const {
    return _lastStateChangeTime;
}

void SmokeRelay::setDebounceDelay(uint16_t delay) {
    _debounceDelay = delay;
}

void SmokeRelay::setRelayType(RelayType type) {
    _relayType = type;
}

void SmokeRelay::setErrorThreshold(uint16_t threshold) {
    _errorThreshold = threshold;
}

void SmokeRelay::onDetection(SmokeCallback callback) {
    _detectionCallback = callback;
}

void SmokeRelay::onClear(SmokeCallback callback) {
    _clearCallback = callback;
}

void SmokeRelay::onError(SmokeCallback callback) {
    _errorCallback = callback;
}

void SmokeRelay::onStatusChange(SmokeCallback callback) {
    _detectionCallback = callback;
    _clearCallback = callback;
    _errorCallback = callback;
}