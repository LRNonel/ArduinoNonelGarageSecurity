#ifndef MY_LED_H
#define MY_LED_H

#include <Arduino.h>

class Led {
public:
    Led(uint8_t pin, uint16_t shortOn = 200, uint16_t shortOff = 600, uint16_t longOn = 400, uint16_t longOff = 1500);
    void begin();
    void on();
    void off();
    void toggle();
    void reset();
    bool getState() const;
    uint8_t getPin() const;
    void shortBlink();
    void longBlink();
    void setShortBlink(uint16_t onTime, uint16_t offTime);
    void setLongBlink(uint16_t onTime, uint16_t offTime);
    void blink(uint16_t interval, uint16_t offTime = 0, uint8_t count = 0);
    void stopBlinking();
	void set(bool state) {
		if(state) on();
        else off();
    }

private:
    uint8_t _pin;
    bool _state;
    unsigned long _lastBlinkTime = 0;
    uint16_t _shortBlinkOn, _shortBlinkOff, _longBlinkOn, _longBlinkOff;
	uint8_t _remainingBlinks = 0;
};

#endif