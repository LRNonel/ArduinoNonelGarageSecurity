#include "SmokeSensor.h"

// Calibration constants
#define RLOAD 10.0          // Load resistance (kΩ)
#define PARA 116.6020682    // Parameter A for PPM conversion
#define PARB 2.769034857    // Parameter B for PPM conversion
float RZERO = 76.63;        // Calibration resistance (will be updated)

SmokeSensor::SmokeSensor(byte pinIn, byte pinHeat) 
    : _pinIn(pinIn), _pinHeat(pinHeat) {
    init();
}

void SmokeSensor::init() {
    pinMode(_pinIn, INPUT);
    pinMode(_pinHeat, OUTPUT);
    digitalWrite(_pinHeat, LOW);
}

void SmokeSensor::beginMeasurement() {
    _isEnabled = true;
    startHeating();
}

void SmokeSensor::stopMeasurement() {
    _isEnabled = false;
    digitalWrite(_pinHeat, LOW);
    _currentState = SensorState::IDLE;
    if(_stateChangeCallback) {
        _stateChangeCallback(_currentState);
    }
}

void SmokeSensor::update() {
    if(!_isEnabled) return;

    unsigned long currentTime = millis();
    SensorState previousState = _currentState;
    
    switch(_currentState) {
        case SensorState::HEATING:
            if(currentTime - _phaseStartTime >= _heatingDuration) {
                startCooling();
            }
            break;
            
        case SensorState::COOLING:
            if(currentTime - _phaseStartTime >= _coolingDuration) {
                takeMeasurement();
                startHeating();
            }
            break;
            
        default: break;
    }
    
    if(_currentState != previousState && _stateChangeCallback) {
        _stateChangeCallback(_currentState);
    }
}

void SmokeSensor::startHeating() {
    digitalWrite(_pinHeat, HIGH);
    _phaseStartTime = millis();
    _currentState = SensorState::HEATING;
}

void SmokeSensor::startCooling() {
    digitalWrite(_pinHeat, LOW);
    _phaseStartTime = millis();
    _currentState = SensorState::COOLING;
}

void SmokeSensor::takeMeasurement() {
    int rawValue = analogRead(_pinIn);
    _ppm = convertToPPM(rawValue);
    _currentState = SensorState::MEASURING;
    
    if(_measurementCallback) {
        _measurementCallback(_ppm);
    }
    
    checkAlert(_ppm);
}

float SmokeSensor::convertToPPM(int rawValue) {
    float voltage = rawValue * (5.0 / 1023.0);
    float rs = ((5.0 * RLOAD) / voltage) - RLOAD;
    float ratio = rs / RZERO;
    return PARA * pow(ratio, -PARB);
}

void SmokeSensor::checkAlert(float ppm) {
    if(_alertCallback) {
        bool isCritical = (ppm >= _criticalThreshold);
        _alertCallback(ppm, isCritical);
    }
}

void SmokeSensor::onMeasurement(MeasurementCallback callback) {
    _measurementCallback = callback;
}

void SmokeSensor::onStateChange(StateChangeCallback callback) {
    _stateChangeCallback = callback;
}

void SmokeSensor::onAlert(AlertCallback callback) {
    _alertCallback = callback;
}

float SmokeSensor::getPPM() const {
    return _ppm;
}

int SmokeSensor::getRawValue() const {
    return analogRead(_pinIn);
}

SmokeSensor::SensorState SmokeSensor::getState() const {
    return _currentState;
}

bool SmokeSensor::isMeasuring() const {
    return _isEnabled;
}

void SmokeSensor::calibrateCleanAir() {
    float rs = 0.0;
    for(int i = 0; i < 100; i++) {
        rs += ((5.0 * RLOAD) / (analogRead(_pinIn) * (5.0 / 1023.0))) - RLOAD;
        delay(10);
    }
    RZERO = rs / 9.8; // Clean air calibration factor
}

void SmokeSensor::setHeatingDuration(unsigned long duration) {
    _heatingDuration = duration;
}

void SmokeSensor::setCoolingDuration(unsigned long duration) {
    _coolingDuration = duration;
}

void SmokeSensor::setAlertThreshold(float threshold) {
    _alertThreshold = threshold;
}

void SmokeSensor::setThresholds(float warning, float critical) {
    _warningThreshold = warning;
    _criticalThreshold = critical;
    _alertThreshold = warning; // Or use separate logic
}

void SmokeSensor::checkLevels() {
    if (_alertCallback) {
        bool critical = isCriticalLevel();
        if (critical || isSmokeDetected()) {
            _alertCallback(_ppm, critical);
        }
    }
}

bool SmokeSensor::isOperational() const {
    // Check if pins are valid
    if(_pinIn == _pinHeat) return false;  // Invalid pin configuration
    
    // Check if sensor is stuck in one state
    const unsigned long MAX_STATE_DURATION = _heatingDuration * 3;  // 3x heating cycle
    if(_currentState != SensorState::IDLE && 
       millis() - _phaseStartTime > MAX_STATE_DURATION) {
        return false;
    }
    
    // Check for valid readings
    int rawValue = analogRead(_pinIn);
    if(rawValue <= 0 || rawValue >= 1023) {  // Stuck at min/max
        return false;
    }
    
    // Check heater circuit
    if(_isEnabled && _currentState == SensorState::HEATING) {
        float voltage = analogRead(_pinIn) * (5.0 / 1023.0);
        if(voltage < 0.1) {  // Should see some response when heating
            return false;
        }
    }
    
    return true;
}

bool SmokeSensor::isSmokeDetected() const {
    return _ppm >= _alertThreshold;
}

bool SmokeSensor::isCriticalLevel() const {
    return _ppm >= _criticalThreshold;
}