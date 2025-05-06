#ifndef IBUTTONACCESS_H
#define IBUTTONACCESS_H

#include <Arduino.h>
#include <OneWire.h>

class iButtonAccess {
public:
    enum class SystemStatus {
        DISARMED,       // System off
        ARMING,         // Arming in progress
        ARMED,          // System armed
        ALARM           // Alarm triggered
    };

    // Callback types
    typedef void (*AccessCallback)(const uint8_t* keyId);
    typedef void (*StatusCallback)(SystemStatus status);

    explicit iButtonAccess(uint8_t pin);
    
    void begin();
    void update();
    bool addKey(const uint8_t* keyId);
    bool removeKey(const uint8_t* keyId);
    void armSystem(uint16_t delaySec = 0);
    void disarmSystem();
    void triggerAlarm();
    
    // Callback setters
    void setAccessGrantedCallback(AccessCallback callback);
    void setAccessDeniedCallback(AccessCallback callback);
    void setStatusChangeCallback(StatusCallback callback);
    
    // Key utilities
    static bool compareKeys(const uint8_t* key1, const uint8_t* key2);
    static void printKey(const uint8_t* keyId);

private:
    OneWire _oneWire;
    uint8_t _pin;
	SystemStatus _status = SystemStatus::DISARMED;
    unsigned long _armingStartTime = 0;
    uint16_t _armingDelay = 0;
    
    uint8_t _authorizedKeys[2][8] = {0}; // Max 3 keys
    uint8_t _keyCount = 0;
    
    AccessCallback _accessGrantedCallback = nullptr;
    AccessCallback _accessDeniedCallback = nullptr;
    StatusCallback _statusChangeCallback = nullptr;
    
    bool readKey(uint8_t* keyId);
    bool isKeyAuthorized(const uint8_t* keyId);
    void changeStatus(SystemStatus newStatus);
};

#endif