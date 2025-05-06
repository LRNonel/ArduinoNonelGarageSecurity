#ifndef DOORSENSOR_H
#define DOORSENSOR_H

#include <Arduino.h>

class DoorSensor {
public:
    enum class StateChange {
        OPENED,
        CLOSED,
        NO_CHANGE
    };

    typedef void (*StateChangeCallback)(StateChange change);

    enum class SensorType {
        NORMALLY_OPEN,
        NORMALLY_CLOSED
    };

private:
    const byte _pin;
    bool _state;
    bool _lastStableState;
    bool _lastRawReading;
    uint16_t _lastDebounceTime = 0;  // В секундах
    uint16_t _debounceDelay = 50;    // В миллисекундах
    SensorType _sensorType;
    uint32_t _lastStateChangeTime = 0; // В секундах (хватит на 136 лет работы)
    StateChangeCallback _stateChangeCallback = nullptr;

public:
    explicit DoorSensor(byte pin, SensorType type = SensorType::NORMALLY_OPEN);
    
    // State management
	bool readSensor();
    bool isOpen() const { return _state; }
    bool isClosed() const { return !_state; }
    bool getState() const { return _state; }
    bool getRawState() const { return _lastRawReading; }
    
    // Timing (теперь возвращает секунды)
    uint32_t getCurrentStateDuration() const { return millis()/1000 - _lastStateChangeTime; }
    uint32_t getLastChangeTime() const { return _lastStateChangeTime; }
    
    // Configuration
    void setDebounceDelay(uint16_t delayMs) { _debounceDelay = delayMs; }
    void setSensorType(SensorType type) { _sensorType = type; }
    void setStateChangeCallback(StateChangeCallback callback) { _stateChangeCallback = callback; }
    
    // Operations
    StateChange update();
    bool hasChanged() const { return _state != _lastStableState; }
    bool isOperational() const;
};

#endif