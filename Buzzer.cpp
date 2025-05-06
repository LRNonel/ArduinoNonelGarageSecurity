
#include "Buzzer.h"

Buzzer::Buzzer(byte pin) : _pin(pin), _state(false), _frequency(2400) {
    pinMode(_pin, OUTPUT);
    off();
}

void Buzzer::on() {
    tone(_pin, _frequency);
    _state = true;
}

void Buzzer::off() {
    noTone(_pin);
    _state = false;
}

void Buzzer::toggle() {
    if (_state) {
        off();
    } else {
        on();
    }
}

void Buzzer::beep(uint16_t duration, uint16_t frequency) {
    if (duration == 0) return;
    uint16_t freq = (frequency > 0) ? frequency : _frequency;
    tone(_pin, freq, duration);
    _state = true;
}

void Buzzer::shortBeep() {
    beep(_shortBeepDuration);
}

void Buzzer::longBeep() {
    beep(_longBeepDuration);
}

// Multiple beeps
void Buzzer::shortBeep(uint8_t count) {
    for (uint8_t i = 0; i < count; i++) {
        shortBeep();
        if (i < count - 1) delay(_shortBeepDuration * 2);
    }
}

void Buzzer::longBeep(uint8_t count) {
    for (uint8_t i = 0; i < count; i++) {
        longBeep();
        if (i < count - 1) delay(_longBeepDuration * 2);
    }
}





