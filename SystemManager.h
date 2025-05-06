#pragma once
#ifndef SYSTEM_MANAGER_H
#define SYSTEM_MANAGER_H

#include <Alarm.h>
#include <Arduino.h>
#include <Buzzer.h>
#include <DoorSensor.h>
#include <EventLogger.h>
#include "GarageLight.h"
#include <GSMController.h>
#include <iButtonAccess.h>
#include <Led.h>
#include <MovingSensor.h>
#include <MultiDS18B20.h>
#include <SmokeRelay.h>
#include <SmokeSensor.h>
#include <avr/pgmspace.h>

class SystemManager {
public:
    enum class SystemState : uint8_t {
        DISARMED,       // 0
        ARMING,         // 1
        ARMED,          // 2
        FIRE_ALERT,     // 3
        INTRUSION_ALERT,// 4
        MAINTENANCE     // 5
    };
    
    enum class MsgID : uint8_t {
	ALRM_FIRE,
	ALRM_INTRUSION,
	BAD_IBUTTON,
	HEALTH_FAIL,
	KEY_ADDED,
	KEY_REMOVED,
	SMOKE_MISMATCH,
	SYS_ARMED,
	SYS_ARMING,
	SYS_DISARMED,
	SYS_READY,
	TEMP_READINGS
	};
    
    typedef void (*SystemCallback)(SystemState state, const char* message);
    typedef void (*AlertCallback)(SystemState state, const char* message);

    SystemManager(GSMController& gsm, Alarm& alarm, SmokeSensor& smoke1, 
                SmokeSensor& smoke2, DoorSensor& door, DoorSensor& gate, 
                iButtonAccess& ibutton, EventLogger& logger, Buzzer& buzzer, 
                MultiDS18B20& temps, SmokeRelay& smokeRelay, Led& redLed, 
                Led& yellowLed, Led& greenLed, MovingSensor& motion, GarageLight& garageLight);
    
    void begin();
    void update();
	
    // System control
    bool armSystem(uint16_t delaySec = 30);
    bool cancelArming();
    bool disarmSystem();
    void triggerEmergency();
    void enterMaintenanceMode();
    
    // System state
    SystemState getState() const;
    const char* getStateString() const;
    unsigned long getStateDuration() const;
    unsigned long getArmingRemaining() const;
    
    // Callbacks
    void onStateChange(SystemCallback callback);
    void onAlert(AlertCallback callback);
    
    // Configuration
    void setArmingDelay(uint16_t delay);
    void setAlertThresholds(float smokeWarning, float smokeCritical);
    void setSmokeDifferential(float differential);
    void setAdminPhoneNumbers(const char* primary, const char* secondary = "");
    void setUserPhoneNumbers(const char* primary, const char* secondary = "");

    // Security
    bool verifyIButtonKey(const uint8_t* key);
    bool addAuthorizedKey(const uint8_t* key);
    bool removeAuthorizedKey(const uint8_t* key);
    
    const char* getAdminPhone1() const { return _adminPhone1; }
    const char* getAdminPhone2() const { return _adminPhone2; }
    const char* getUserPhone1() const { return _userPhone1; }
    const char* getUserPhone2() const { return _userPhone2; }
    
    bool hasAdminPhone2() const { return _adminPhone2[0] != '\0'; }
    bool hasUserPhone1() const { return _userPhone1[0] != '\0'; }
    bool hasUserPhone2() const { return _userPhone2[0] != '\0'; }

    bool verifyPhoneNumber(const String& number) const; 
    bool verifyPhoneNumber(const char* number) const;
    void getTemperatureReadings(char* buffer) const;
	void handleIncomingCall(const String& number);
	bool checkSystemHealth();
	
private:
	EventLogger::EventType _determineEventType(MsgID msgId) const;
    // External dependencies
    GSMController& _gsm;
    Alarm& _alarm;
    SmokeSensor& _smoke1;
    SmokeSensor& _smoke2;
    DoorSensor& _door;
    DoorSensor& _gate;
    iButtonAccess& _ibutton;
    EventLogger& _logger;
    Buzzer& _buzzer;
    MultiDS18B20& _temps;
    SmokeRelay& _smokeRelay;
    Led& _redLed;
    Led& _yellowLed;
    Led& _greenLed;
    MovingSensor& _motion;
	GarageLight& _garageLight;
    // System state
    SystemState _state = SystemState::DISARMED;
    SystemState _previousState = SystemState::DISARMED;
    unsigned long _stateChangeTime = 0;
    unsigned long _armingStartTime = 0;
    char _healthStatus[32];
	
    // Configuration
    float _smokeWarningThreshold = 30.0;
    float _smokeCriticalThreshold = 50.0;
    float _smokeDifferential = 15.0;
    uint16_t _armingDelay = 30;
    
    // Security
    char _adminPhone1[12] = "+79210308335"; // +1234567890\0
    char _adminPhone2[12] = "";
    char _userPhone1[12] = "";
    char _userPhone2[12] = "";
    uint8_t _authorizedKeys[3][8]; // Store keys directly
    uint8_t _numKeys = 0;
    
    // Callbacks
    SystemCallback _stateCallback = nullptr;
    AlertCallback _alertCallback = nullptr;

    // Constants
    static constexpr uint16_t ALARM_DURATION = 300000;
    static constexpr uint16_t ALARM_BLINK_INTERVAL = 500;

    // Static instances for callbacks
    static SystemManager* _smoke1Instance;
    static SystemManager* _smoke2Instance;
    static SystemManager* _motionInstance;
    static SystemManager* _instanceForIButton;
    static SystemManager* _doorInstance;
    static SystemManager* _gateInstance;

    // Private methods
    void _changeState(SystemState newState, MsgID msgId, const char* extra = nullptr);
    void _handleSensorEvents();
    void _handleSecurityBreach(MsgID msgId);
    void _handleFireAlert(float ppm1, float ppm2);
    void _sendAlertNotification(MsgID msgId, const char* extra = nullptr);
    void _logEvent(MsgID msgId, const char* extra = nullptr);
    bool _checkSmokeConsistency(float ppm1, float ppm2) const;
    bool _checkSystemHealth();

    // Message handling
    const char* _getMessage(MsgID id) const;
    
    // Static callback wrappers
    static void _handleSmoke1AlertStatic(float ppm, bool isCritical);
    static void _handleSmoke2AlertStatic(float ppm, bool isCritical);
    static void _handleMotionStatic();
    static void _handleIButtonAccessStatic(const uint8_t* keyId);
    static void _handleDoorEventStatic(DoorSensor::StateChange change);
    static void _handleGateEventStatic(DoorSensor::StateChange change);

    // Instance handlers
    void _handleSmoke1Alert(float ppm, bool isCritical);
    void _handleSmoke2Alert(float ppm, bool isCritical);
    void _handleMotion();
    void _handleIButtonAccess(const uint8_t* keyId);
    void _handleDoorEvent(DoorSensor::StateChange change);
    void _handleGateEvent(DoorSensor::StateChange change);
	void _handleLightToggle();
};

#endif