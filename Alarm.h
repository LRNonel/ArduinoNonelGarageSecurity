#pragma once
#include <Arduino.h>

class Alarm {
public:
    enum class Mode {
        OFF,
        EMERGENCY
    };

private:
    byte _pin;
    bool _state;
    Mode _currentMode = Mode::OFF;
    
    // Timing variables (defaults as constexpr for easy maintenance)
    static constexpr uint16_t DEFAULT_EMERGENCY_INTERVAL = 1000;
    
    unsigned long _lastUpdateTime = 0;
    uint16_t _emergencyInterval = DEFAULT_EMERGENCY_INTERVAL;
    
    // Emergency-specific
    uint8_t _remainingBlinks = 0;
    
    void setOutput(bool state);

public:
    explicit Alarm(byte pin);
    
    // Basic control
	void init();
    void on();
    void off();
    void toggle();
    
    // Mode control
    void emergencySignal(uint8_t count = 3);
    
    // Configuration
    void setEmergencyInterval(uint16_t interval);
    
    // Main update function
    void update();
    
    // Status
    bool getState() const;
    Mode getMode() const;
};

