#ifndef BUZZER_H
#define BUZZER_H

#include <Arduino.h>

  
class Buzzer {
public:
    enum class PlayMode {
        SINGLE,
        REPEAT
    };

    explicit Buzzer(byte pin);
    
    // Basic control
    void on();
    void off();
    void toggle();
    
    // Beep methods
    void beep(uint16_t duration, uint16_t frequency = 0);
    void shortBeep();
    void shortBeep(uint8_t count);
    void longBeep();
    void longBeep(uint8_t count);

    
private:
    byte _pin;
    bool _state;
    uint16_t _frequency = 2400;
    uint16_t _shortBeepDuration = 50;
    uint16_t _longBeepDuration = 1000;
    // Sequence playback
    const uint16_t* _frequencies = nullptr;
    uint8_t _sequenceLength = 0;
    uint8_t _currentStep = 0;
    unsigned long _stepStartTime = 0;
    PlayMode _playMode = PlayMode::SINGLE;
    
    void init();
    void stop();
};

#endif