#ifndef MY_SMOKESENSOR_H
#define MY_SMOKESENSOR_H

#include <Arduino.h>

class SmokeSensor {
public:

	bool isOperational() const;
	
    // Sensor operational states
    enum class SensorState {
        HEATING,    // Heating cycle active
        COOLING,    // Cooling period
        MEASURING,  // Taking measurement
        IDLE        // Inactive state
    };

    // Callback types
    typedef void (*MeasurementCallback)(float ppm);
    typedef void (*StateChangeCallback)(SensorState state);
	typedef void (*AlertCallback)(float ppm, bool isCritical);
	void setAlertThreshold(float threshold);
	void checkLevels();
private:
    byte _pinIn;                    // Analog input pin
    byte _pinHeat;                  // Heater control pin
    unsigned long _heatingDuration = 60000;  // Heating time (ms)
    unsigned long _coolingDuration = 90000;  // Cooling time (ms)
    unsigned long _phaseStartTime = 0;       // Phase timer
    SensorState _currentState = SensorState::IDLE;
    float _ppm = 0.0;               // Current PPM reading
    bool _isEnabled = false;        // Measurement active flag
    float _alertThreshold = 50.0;   // Alert threshold (PPM)
    float _criticalThreshold = 100.0; // Critical threshold (PPM)
    float _warningThreshold = 30.0;  // Warning threshold (PPM)
    
    // Callbacks
    MeasurementCallback _measurementCallback = nullptr;
    StateChangeCallback _stateChangeCallback = nullptr;
    AlertCallback _alertCallback = nullptr;
    
    void init();
    void startHeating();
    void startCooling();
    void takeMeasurement();
    float convertToPPM(int rawValue);
    void checkAlert(float ppm);

public:
    // Constructor with I/O pins
    explicit SmokeSensor(byte pinIn, byte pinHeat);
    
    // Measurement control
    void beginMeasurement();
    void stopMeasurement();
    void update();
    
    // Configuration
    void setHeatingDuration(unsigned long duration);
    void setCoolingDuration(unsigned long duration);
	void setThresholds(float warning, float critical);
    
    // Data access
    float getPPM() const;
    int getRawValue() const;
    SensorState getState() const;
    bool isMeasuring() const;
    bool isSmokeDetected() const;
    bool isCriticalLevel() const;
    // Callbacks
    void onMeasurement(MeasurementCallback callback);
    void onStateChange(StateChangeCallback callback);
	void onAlert(AlertCallback callback);	
    
    // Calibration
    void calibrateCleanAir();
};

#endif