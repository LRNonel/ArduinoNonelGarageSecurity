#include "DoorSensor.h"

DoorSensor::DoorSensor(byte pin, SensorType type)
    : _pin(pin), 
      _sensorType(type),
      _state(false),
      _lastStableState(false),
      _lastRawReading(false),
      _lastDebounceTime(0),
      _debounceDelay(50),
      _lastStateChangeTime(0),
      _stateChangeCallback(nullptr) 
{
    pinMode(_pin, INPUT_PULLUP);
    _state = this->readSensor();  // Явное указание this->
    _lastStableState = _state;
}

bool DoorSensor::readSensor() {
    bool reading = digitalRead(_pin);
    bool interpreted = (_sensorType == SensorType::NORMALLY_OPEN) ? !reading : reading;

    if (reading != _lastRawReading) {
        _lastDebounceTime = millis() / 1000;
    }
    _lastRawReading = reading;

    if ((millis()/1000 - _lastDebounceTime) > (_debounceDelay/1000)) {
        return interpreted;
    }
    return _state;
	
	if (millis()/1000 < _lastDebounceTime) { // Произошло переполнение
    _lastDebounceTime = 0;
}
}

DoorSensor::StateChange DoorSensor::update() {
    bool newState = readSensor();

    if (newState != _state) {
        StateChange change = newState ? StateChange::OPENED : StateChange::CLOSED;
        _lastStateChangeTime = millis() / 1000; // Сохраняем секунды
        _lastStableState = _state;
        _state = newState;
        
        if (_stateChangeCallback) {
            _stateChangeCallback(change);
        }
        return change;
    }
    return StateChange::NO_CHANGE;
}

bool DoorSensor::isOperational() const {
    bool currentReading = digitalRead(_pin);
    return (currentReading == HIGH || currentReading == LOW);
}

