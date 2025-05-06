#include "iButtonAccess.h"

iButtonAccess::iButtonAccess(uint8_t pin) : _pin(pin), _oneWire(pin) {}

void iButtonAccess::begin() {
    pinMode(_pin, INPUT_PULLUP);
    _keyCount = 0;
    changeStatus(SystemStatus::DISARMED);
}

void iButtonAccess::update() {
    if (_status == SystemStatus::ARMING && millis() - _armingStartTime >= _armingDelay * 1000) {
        changeStatus(SystemStatus::ARMED);
    }

    uint8_t keyId[8];
    if (readKey(keyId)) {
        if (isKeyAuthorized(keyId)) {
            if (_accessGrantedCallback) _accessGrantedCallback(keyId);
            if (_status == SystemStatus::ARMED || _status == SystemStatus::ALARM) disarmSystem();
            else armSystem(10);
        } else {
            if (_accessDeniedCallback) _accessDeniedCallback(keyId);
            if (_status == SystemStatus::ARMED) triggerAlarm();
        }
    }
}

bool iButtonAccess::readKey(uint8_t* keyId) {
    if (!_oneWire.reset() || _oneWire.read() != 0x33) return false;
    for (uint8_t i = 0; i < 8; i++) keyId[i] = _oneWire.read();
    return _oneWire.crc8(keyId, 7) == keyId[7];
}

bool iButtonAccess::addKey(const uint8_t* keyId) {
    if (_keyCount >= 10) return false;
    memcpy(_authorizedKeys[_keyCount], keyId, 8);
    _keyCount++;
    return true;
}

bool iButtonAccess::removeKey(const uint8_t* keyId) {
    for (uint8_t i = 0; i < _keyCount; i++) {
        if (compareKeys(_authorizedKeys[i], keyId)) {
            for (uint8_t j = i; j < _keyCount - 1; j++) {
                memcpy(_authorizedKeys[j], _authorizedKeys[j + 1], 8);
            }
            _keyCount--;
            return true;
        }
    }
    return false;
}

bool iButtonAccess::isKeyAuthorized(const uint8_t* keyId) {
    for (uint8_t i = 0; i < _keyCount; i++) {
        if (compareKeys(_authorizedKeys[i], keyId)) return true;
    }
    return false;
}

void iButtonAccess::armSystem(uint16_t delaySec) {
    if (_status == SystemStatus::ARMED) return;
    _armingDelay = delaySec;
    _armingStartTime = millis();
    changeStatus(delaySec > 0 ? SystemStatus::ARMING : SystemStatus::ARMED);
}

void iButtonAccess::disarmSystem() {
    changeStatus(SystemStatus::DISARMED);
}

void iButtonAccess::triggerAlarm() {
    changeStatus(SystemStatus::ALARM);
}

void iButtonAccess::changeStatus(SystemStatus newStatus) {
    if (_status != newStatus) {
        _status = newStatus;
        if (_statusChangeCallback) _statusChangeCallback(_status);
    }
}

void iButtonAccess::setAccessGrantedCallback(AccessCallback callback) {
    _accessGrantedCallback = callback;
}

void iButtonAccess::setAccessDeniedCallback(AccessCallback callback) {
    _accessDeniedCallback = callback;
}

void iButtonAccess::setStatusChangeCallback(StatusCallback callback) {
    _statusChangeCallback = callback;
}

bool iButtonAccess::compareKeys(const uint8_t* key1, const uint8_t* key2) {
    return memcmp(key1, key2, 8) == 0;
}

void iButtonAccess::printKey(const uint8_t* keyId) {
    for (uint8_t i = 0; i < 8; i++) {
        if (keyId[i] < 0x10) Serial.print("0");
        Serial.print(keyId[i], HEX);
        if (i < 7) Serial.print(":");
    }
}