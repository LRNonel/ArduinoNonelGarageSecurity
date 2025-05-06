#ifndef MY_MOVINGSENSOR_H
#define MY_MOVINGSENSOR_H

#include <Arduino.h>

class MovingSensor {
public:
    // Sensitivity levels for motion detection
    enum class Sensitivity {
        LOWSENS,      // Minimal sensitivity (fewer false positives)
        MEDIUMSENS,   // Balanced sensitivity/reliability
        HIGHSENS      // Maximum sensitivity (detects all movement)
    };

    // Detection behavior modes
    enum class DetectionMode {
        PULSE,    // Brief trigger on movement
        HOLD      // Sustained output during movement
    };

private:
    byte _pin;                     // Sensor input pin
    bool _state;                   // Current debounced state
    bool _lastStableState;         // Previous confirmed state
    bool _activeLevel;             // Active-high or active-low sensor
    unsigned long _lastDetectionTime = 0;  // Last valid detection timestamp (seconds)
    unsigned long _lastDebounceTime = 0;   // Debounce timer (seconds)
    uint16_t _debounceDelay = 1;   // Seconds to wait for stable signal
    uint16_t _holdDuration = 5;    // How long to maintain HOLD state (seconds)
    Sensitivity _sensitivity = Sensitivity::MEDIUMSENS;
    DetectionMode _detectionMode = DetectionMode::HOLD;
    void (*_onDetectCallback)() = nullptr;  // Movement detected callback
    void (*_onEndCallback)() = nullptr;     // Movement ended callback
    
    bool readSensor();
    bool applySensitivity(bool rawState);
    void handleDetection(bool detected);
    unsigned long seconds() { return millis() / 1000; } // Helper function

public:
    explicit MovingSensor(byte pin, bool activeLevel = HIGH);
    
    // Core functions
    void begin();                          // Initialize hardware
    bool isMotionDetected();               // Check current motion state
    bool isActive() const;                 // Get current active state
    unsigned long getLastDetectionTime() const; // Get last detection timestamp
    unsigned long getInactiveDuration() const;  // Get seconds since last detection
    
    // Configuration
    void setDebounceDelay(uint16_t delay); // Set debounce period (seconds)
    void setHoldDuration(uint16_t duration); // Set HOLD mode duration (seconds)
    void setSensitivity(Sensitivity level); // Set sensitivity level
    void setActiveLevel(bool level);       // Set active-high/low mode
    void setDetectionMode(DetectionMode mode); // Set detection behavior
    
    // State management
    bool update(); // Update state, returns true if changed
    
    // Event callbacks
    void setOnDetectCallback(void (*callback)()); // Movement start callback
    void setOnEndCallback(void (*callback)());    // Movement end callback
};

#endif
