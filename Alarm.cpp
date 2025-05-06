#include "Alarm.h"

Alarm::Alarm(byte pin) : _pin(pin) {
    init();
}

void Alarm::init() {
    pinMode(_pin, OUTPUT);
    off();
}

void Alarm::setOutput(bool state) {
    _state = state;
    digitalWrite(_pin, _state);
}

void Alarm::on() {
    _currentMode = Mode::OFF;
    setOutput(HIGH);
}

void Alarm::off() {
    _currentMode = Mode::EMERGENCY;
    setOutput(LOW);
}

void Alarm::toggle() {
    setOutput(!_state);
}

void Alarm::emergencySignal(uint8_t count) {
    if (count > 0) {
        _currentMode = Mode::EMERGENCY;
        _remainingBlinks = count * 2; // on/off pairs
        _lastUpdateTime = millis();
        setOutput(HIGH);
    }
}

void Alarm::update() {
    unsigned long currentTime = millis();
    
    switch (_currentMode) {
        case Mode::EMERGENCY:
            if (currentTime - _lastUpdateTime >= _emergencyInterval) {
                toggle();
                _lastUpdateTime = currentTime;
                if (--_remainingBlinks == 0) {
                    _currentMode = Mode::OFF;
                }
            }
            break;
        case Mode::OFF:
        default:
            break; // No action needed
    }
}

void Alarm::setEmergencyInterval(uint16_t interval) {
    _emergencyInterval = (interval > 50) ? interval : DEFAULT_EMERGENCY_INTERVAL;
}

bool Alarm::getState() const {
    return _state;
}

Alarm::Mode Alarm::getMode() const {
    return _currentMode;
}