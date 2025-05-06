#ifndef MY_SMOKE_RELAY_H
#define MY_SMOKE_RELAY_H

#include <Arduino.h>

class SmokeRelay {
public:
    // Relay electrical configuration types
    enum class RelayType {
        NORMALLY_OPEN,  // NO - Open circuit when inactive
        NORMALLY_CLOSED // NC - Closed circuit when inactive (default)
    };

    // Smoke detection states
    enum class SmokeStatus {
        CLEAR,          // No smoke detected
        DETECTED,       // Smoke detected
        ERROR           // Sensor fault detected
    };

    // Callback function type
    typedef void (*SmokeCallback)(SmokeStatus status);

private:
    byte _pin;                      // Sensor input pin
    bool _state;                    // Current debounced state
    bool _lastStableState;          // Previous confirmed state
    unsigned long _lastDebounceTime = 0;  // Debounce timer
    unsigned long _lastStateChangeTime = 0; // State change timestamp
    uint16_t _debounceDelay = 50;   // Debounce period (ms)
    RelayType _relayType = RelayType::NORMALLY_CLOSED;
    uint16_t _errorThreshold = 3000; // Error detection threshold (ms)
    
    // Event callbacks
    SmokeCallback _detectionCallback = nullptr;  // Smoke detected callback
    SmokeCallback _clearCallback = nullptr;      // Clear air callback
    SmokeCallback _errorCallback = nullptr;      // Error callback
    
    void init();
    bool readHardwareState();
    bool applyDebounce(bool rawState);
    void notifyCallbacks(SmokeStatus previousStatus, SmokeStatus newStatus);

public:
    explicit SmokeRelay(byte pin, RelayType type = RelayType::NORMALLY_CLOSED);
    
    // Core functions
    SmokeStatus getStatus() const;      // Get current status
    bool isActive() const;              // Check if relay is activated
    bool isSmokeDetected() const;       // Check for smoke detection
    bool isError() const;               // Check for sensor error
    
    // Timing functions
    unsigned long getCurrentStateDuration() const; // Current state duration (ms)
    unsigned long getLastChangeTime() const;       // Last state change timestamp
    
    // Configuration
    void setDebounceDelay(uint16_t delay);  // Set debounce period (ms)
    void setRelayType(RelayType type);      // Set relay electrical type
    void setErrorThreshold(uint16_t threshold); // Set error detection threshold
    
    // Event callbacks
    void onDetection(SmokeCallback callback);   // Smoke detected callback
    void onClear(SmokeCallback callback);       // Clear air callback
    void onError(SmokeCallback callback);       // Error callback
    void onStatusChange(SmokeCallback callback); // Any state change callback
    
    // State management
    bool update(); // Update state, returns true if changed
};

#endif