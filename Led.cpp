#include "Led.h"

Led::Led(uint8_t pin, uint16_t shortOn, uint16_t shortOff, uint16_t longOn, uint16_t longOff) 
    : _pin(pin), _shortBlinkOn(shortOn), _shortBlinkOff(shortOff), _longBlinkOn(longOn), _longBlinkOff(longOff) {}

void Led::begin() {
    pinMode(_pin, OUTPUT);
    off();
}

void Led::on() {
    _state = HIGH;
    digitalWrite(_pin, _state);
}

void Led::off() {
    _state = LOW;
    digitalWrite(_pin, _state);
}

void Led::toggle() {
    _state = !_state;
    digitalWrite(_pin, _state);
}

void Led::reset() {
    _lastBlinkTime = millis();
    off();
}

bool Led::getState() const {
    return _state;
}

uint8_t Led::getPin() const {
    return _pin;
}

void Led::shortBlink() {
    unsigned long now = millis();
    uint16_t duration = _state ? _shortBlinkOn : _shortBlinkOff;
    if (now - _lastBlinkTime >= duration) {
        toggle();
        _lastBlinkTime = now;
    }
}

void Led::longBlink() {
    unsigned long now = millis();
    uint16_t duration = _state ? _longBlinkOn : _longBlinkOff;
    if (now - _lastBlinkTime >= duration) {
        toggle();
        _lastBlinkTime = now;
    }
}

void Led::setShortBlink(uint16_t onTime, uint16_t offTime) {
    _shortBlinkOn = onTime;
    _shortBlinkOff = offTime;
}

void Led::setLongBlink(uint16_t onTime, uint16_t offTime) {
    _longBlinkOn = onTime;
    _longBlinkOff = offTime;
}

void Led::blink(uint16_t interval, uint16_t offTime, uint8_t count) {
    // Handle all cases in one method
    if (count > 0) {
        // Counted blinking mode
        if (_remainingBlinks == 0) {
            _remainingBlinks = count * 2;
        }
        
        unsigned long now = millis();
        if (now - _lastBlinkTime >= interval) {
            toggle();
            _lastBlinkTime = now;
            if (_remainingBlinks > 0) {
                _remainingBlinks--;
            }
        }
        
        if (_remainingBlinks == 0) {
            off();
        }
    } 
    else if (offTime > 0) {
        // Asymmetric blinking (different on/off times)
        unsigned long now = millis();
        uint16_t duration = _state ? interval : offTime;
        if (now - _lastBlinkTime >= duration) {
            toggle();
            _lastBlinkTime = now;
        }
    }
    else {
        // Symmetric blinking (same on/off time)
        unsigned long now = millis();
        if (now - _lastBlinkTime >= interval) {
            toggle();
            _lastBlinkTime = now;
        }
    }
}

void Led::stopBlinking() {
    off();
    _remainingBlinks = 0;
}
