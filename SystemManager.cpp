#include "SystemManager.h"
#include <avr/pgmspace.h>

// Initialize static pointers
SystemManager* SystemManager::_smoke1Instance = nullptr;
SystemManager* SystemManager::_smoke2Instance = nullptr;
SystemManager* SystemManager::_motionInstance = nullptr;
SystemManager* SystemManager::_instanceForIButton = nullptr;
SystemManager* SystemManager::_doorInstance = nullptr;
SystemManager* SystemManager::_gateInstance = nullptr;

// PROGMEM Messages (alphabetically ordered)
static const char* const _messages[] PROGMEM = {
    /* ALRM_FIRE */     "FIRE",          // 0
    /* ALRM_INTRUSION */"INTRUSION",     // 1
    /* BAD_IBUTTON */   "BAD_IBTN",      // 2
    /* HEALTH_FAIL */   "HEALTH_FAIL",   // 3
    /* KEY_ADDED */     "KEY_ADD",       // 4
    /* KEY_REMOVED */   "KEY_REM",       // 5
    /* SMOKE_MISMATCH */"SMK_MIS",       // 6
    /* SYS_ARMED */     "ARMED",         // 7
    /* SYS_ARMING */    "ARMING",        // 8
    /* SYS_DISARMED */  "DISARMED",      // 9
    /* SYS_READY */     "READY",         // 10
    /* TEMP_READINGS */ "TEMP"           // 11
};

SystemManager::SystemManager(GSMController& gsm, Alarm& alarm, SmokeSensor& smoke1, 
            SmokeSensor& smoke2, DoorSensor& door, DoorSensor& gate, iButtonAccess& ibutton, 
            EventLogger& logger, Buzzer& buzzer, MultiDS18B20& temps, SmokeRelay& smokeRelay, 
            Led& redLed, Led& yellowLed, Led& greenLed, MovingSensor& motion, GarageLight& garageLight)
    : _gsm(gsm), _alarm(alarm), _smoke1(smoke1), _smoke2(smoke2),
      _door(door), _gate(gate), _ibutton(ibutton), _logger(logger), 
      _buzzer(buzzer), _temps(temps), _smokeRelay(smokeRelay), 
      _redLed(redLed), _yellowLed(yellowLed), _greenLed(greenLed), _motion(motion), _garageLight(garageLight) 
{
    // Initialize default keys
	//01:66:84:27:55:00:00:20
	//01:45:E8:13:00:00:00:F7
    const uint8_t defaultKey1[8] = {0x01,0x66,0x84,0x27,0x55,0x00,0x00,0x20};
    const uint8_t defaultKey2[8] = {0x01,0x45,0xE8,0x13,0x00,0x00,0x00,0xF7};
    memcpy(_authorizedKeys[0], defaultKey1, 8);
    memcpy(_authorizedKeys[1], defaultKey2, 8);
    _numKeys = 2;
}

const char* SystemManager::_getMessage(MsgID id) const {
    return (const char*)pgm_read_ptr(&_messages[static_cast<uint8_t>(id)]);
}

void SystemManager::begin() {
    if(!_checkSystemHealth()) {
        _logEvent(MsgID::HEALTH_FAIL);
        return;
    }

    _alarm.init();
    _ibutton.begin();
    _gsm.begin();
    _alarm.off();
    _redLed.off();
    _yellowLed.off();
    _greenLed.on();
    
    // Setup callbacks
    _smoke1Instance = this;
    _smoke2Instance = this;
    _smoke1.onAlert(_handleSmoke1AlertStatic);
    _smoke2.onAlert(_handleSmoke2AlertStatic);
    
    _doorInstance = this;
    _gateInstance = this;
    _door.setStateChangeCallback(_handleDoorEventStatic);
    _gate.setStateChangeCallback(_handleGateEventStatic);

    _motionInstance = this;
    _motion.setOnDetectCallback(_handleMotionStatic);
    
    _instanceForIButton = this;
    _ibutton.setAccessGrantedCallback(_handleIButtonAccessStatic);
    
    _logEvent(MsgID::SYS_READY);
}

void SystemManager::update() {
    if(!_checkSystemHealth()) {
        _changeState(SystemState::FIRE_ALERT, MsgID::HEALTH_FAIL);
        return;
    }

    _gsm.update();
    _alarm.update();
    _smoke1.update();
    _smoke2.update();
    _smokeRelay.update();
    _door.update();
    _gate.update();
    _ibutton.update();
    _motion.update();
    _temps.update();
    
    // Handle ARMING state timeout
    if(_state == SystemState::ARMING) {
        unsigned long remaining = (_armingDelay * 1000) - (millis() - _armingStartTime);
        if(remaining <= 0) {
            _changeState(SystemState::ARMED, MsgID::SYS_ARMED);
        } else {
            uint16_t blinkInterval = map(remaining, 0, _armingDelay * 1000, 200, 1000);
            _yellowLed.set(millis() % blinkInterval < blinkInterval / 2);
        }
    }
    
    // Handle alert state timeouts
    if((_state == SystemState::FIRE_ALERT || _state == SystemState::INTRUSION_ALERT) &&
       (millis() - _stateChangeTime) >= ALARM_DURATION) {
        _changeState(SystemState::DISARMED, MsgID::SYS_DISARMED, "AUTO");
    }
    
    _handleSensorEvents();
    
    // Pulsing effect for armed state
    if(_state == SystemState::ARMED) {
        unsigned long pulsePhase = (millis() % 4000);
        _greenLed.set(pulsePhase < 1800 && (millis() % 200 < 100));
    }
}

const char* SystemManager::getStateString() const {
    static const char* const states[] PROGMEM = {
        "DISARMED", "ARMING", "ARMED", "FIRE", "INTRUSION", "MAINT"
    };
    return (const char*)pgm_read_ptr(&states[static_cast<int>(_state)]);
}

void SystemManager::getTemperatureReadings(char* buffer) const {
    // Format: "T:GG.OO" (values *10, no decimal)
    uint16_t gTemp = _temps.getGarageTemp() * 10;
    uint16_t oTemp = _temps.getOutdoorTemp() * 10;
    snprintf_P(buffer, 8, PSTR("T:%02d%02d"), gTemp, oTemp);
}

bool SystemManager::armSystem(uint16_t delaySec) {
    if(_state == SystemState::ARMED || _state == SystemState::MAINTENANCE) {
        return false;
    }
    
    _armingDelay = max(10, delaySec);
    _armingStartTime = millis();
    _changeState(SystemState::ARMING, MsgID::SYS_ARMING);
    
    _buzzer.shortBeep(2);
    _yellowLed.blink(500, 3);
    _sendAlertNotification(MsgID::SYS_ARMING);
    
    return true;
}

bool SystemManager::disarmSystem() {
    if(_state == SystemState::DISARMED) return true;
    
    _changeState(SystemState::DISARMED, MsgID::SYS_DISARMED);
    _buzzer.shortBeep(2);
    return true;
}

void SystemManager::_changeState(SystemState newState, MsgID msgId, const char* extra) {
    if(_state == newState) return;
    
    // Turn off previous state indicators
    switch(_state) {
        case SystemState::FIRE_ALERT:
        case SystemState::INTRUSION_ALERT:
            _alarm.off();
            break;
        default: break;
    }

    _previousState = _state;
    _state = newState;
    _stateChangeTime = millis();
    
    // Handle new state indicators
    switch(_state) {
        case SystemState::DISARMED:
            _buzzer.shortBeep();
            _redLed.off();
            _yellowLed.off();
            _greenLed.on();
            break;
            
        case SystemState::ARMING:
            _buzzer.shortBeep(2);
            _redLed.off();
            _yellowLed.blink(1000);
            _greenLed.off();
            break;
            
        case SystemState::ARMED:
            _buzzer.shortBeep(3);
            _redLed.off();
            _yellowLed.off();
            _greenLed.blink(2000);
            break;
            
        case SystemState::FIRE_ALERT:
            _alarm.on();
            _buzzer.off();
            _redLed.blink(ALARM_BLINK_INTERVAL);
            _yellowLed.off();
            _greenLed.off();
            break;
            
        case SystemState::INTRUSION_ALERT:
            _alarm.on();
            _buzzer.off();
            _redLed.blink(ALARM_BLINK_INTERVAL);
            _yellowLed.off();
            _greenLed.off();
            break;
            
        case SystemState::MAINTENANCE:
            _buzzer.longBeep();
            _redLed.off();
            _yellowLed.on();
            _greenLed.off();
            break;
    }
    
	char logMsg[32];
    if(extra) {
        snprintf_P(logMsg, sizeof(logMsg), PSTR("%s:%s"), _getMessage(msgId), extra);
    } else {
        strncpy_P(logMsg, _getMessage(msgId), sizeof(logMsg));
    }

    EventLogger::EventType type = _determineEventType(msgId);
    _logger.logEvent(type, logMsg); 
    
    if(_stateCallback) {
        _stateCallback(_state, logMsg);
    }
    
    if((_state == SystemState::FIRE_ALERT || _state == SystemState::INTRUSION_ALERT) && _alertCallback) {
        _alertCallback(_state, logMsg);
    }
}

void SystemManager::_handleSecurityBreach(MsgID msgId) {
    if (_state != SystemState::ARMED) return;
    _changeState(SystemState::INTRUSION_ALERT, msgId);
    _sendAlertNotification(msgId);
}

bool SystemManager::_checkSmokeConsistency(float ppm1, float ppm2) const {
    return fabs(ppm1 - ppm2) <= _smokeDifferential;
}

SystemManager::SystemState SystemManager::getState() const {
    return _state;
}

unsigned long SystemManager::getStateDuration() const {
    return millis() - _stateChangeTime;
}

unsigned long SystemManager::getArmingRemaining() const {
    if(_state != SystemState::ARMING) return 0;
    unsigned long elapsed = millis() - _armingStartTime;
    return (_armingDelay * 1000) - min(elapsed, _armingDelay * 1000);
}

void SystemManager::setSmokeDifferential(float differential) {
    _smokeDifferential = differential;
}

void SystemManager::_handleSensorEvents() {
    if(_state == SystemState::MAINTENANCE) {
        char status[32];
        char tempBuf[8];
        getTemperatureReadings(tempBuf);
        
        snprintf_P(status, sizeof(status), 
            PSTR("S:D1:%d,D2:%d,M:%d,%s"),
            _door.isOpen() ? 1 : 0,
            _gate.isOpen() ? 1 : 0,
            _motion.isActive() ? 1 : 0,
            tempBuf);
        _logger.logEvent(status);
    }
}

void SystemManager::_sendAlertNotification(MsgID msgId, const char* extra) {
    char smsBuf[32];
    if(extra) {
        snprintf_P(smsBuf, sizeof(smsBuf), PSTR("%s:%s"), _getMessage(msgId), extra);
    } else {
        strncpy_P(smsBuf, _getMessage(msgId), sizeof(smsBuf));
    }
    
    if(_adminPhone1[0]) _gsm.sendSMS(_adminPhone1, smsBuf);
    if(_adminPhone2[0]) _gsm.sendSMS(_adminPhone2, smsBuf);
    
    if(msgId == MsgID::ALRM_FIRE || msgId == MsgID::ALRM_INTRUSION) {
        if(_userPhone1[0]) _gsm.sendSMS(_userPhone1, smsBuf);
        if(_userPhone2[0]) _gsm.sendSMS(_userPhone2, smsBuf);
    }
    
    _logger.logEvent(smsBuf);
}

void SystemManager::_logEvent(MsgID msgId, const char* extra) {
    char logBuf[32];
    if(extra) {
        snprintf_P(logBuf, sizeof(logBuf), PSTR("%s:%s"), _getMessage(msgId), extra);
    } else {
        strncpy_P(logBuf, _getMessage(msgId), sizeof(logBuf));
    }
    
    EventLogger::EventType type = _determineEventType(msgId);
    _logger.logEvent(type, logBuf);
    Serial.println(logBuf);
}
    
EventLogger::EventType SystemManager::_determineEventType(MsgID msgId) const {
    switch(msgId) {
        case MsgID::ALRM_FIRE: return EventLogger::SMOKE_DETECTED;
        case MsgID::ALRM_INTRUSION: return EventLogger::ALARM_TRIGGERED;
        case MsgID::BAD_IBUTTON: return EventLogger::UNKNOWN_EVENT;
        case MsgID::HEALTH_FAIL: return EventLogger::UNKNOWN_EVENT;
        case MsgID::KEY_ADDED: return EventLogger::UNKNOWN_EVENT;
        case MsgID::KEY_REMOVED: return EventLogger::UNKNOWN_EVENT;
        case MsgID::SMOKE_MISMATCH: return EventLogger::SMOKE_DETECTED;
        case MsgID::SYS_ARMED: return EventLogger::UNKNOWN_EVENT;
        case MsgID::SYS_ARMING: return EventLogger::UNKNOWN_EVENT;
        case MsgID::SYS_DISARMED: return EventLogger::UNKNOWN_EVENT;
        case MsgID::SYS_READY: return EventLogger::UNKNOWN_EVENT;
        case MsgID::TEMP_READINGS: return EventLogger::UNKNOWN_EVENT;
        default: return EventLogger::UNKNOWN_EVENT;
    }
}

bool SystemManager::_checkSystemHealth() {
    static char lastHealthStatus[32] = "";
    strcpy(_healthStatus, "H:");
    bool healthy = true;
    
    if(!_smoke1.isOperational()) { strcat(_healthStatus, "S1,"); healthy = false; }
    if(!_smoke2.isOperational()) { strcat(_healthStatus, "S2,"); healthy = false; }
    if(_smokeRelay.isError())    { strcat(_healthStatus, "SR,"); healthy = false; }
    if(!_door.isOperational())   { strcat(_healthStatus, "D1,"); healthy = false; }
    if(!_gate.isOperational())   { strcat(_healthStatus, "D2,"); healthy = false; }
    if(!_gsm.isOperational())    { strcat(_healthStatus, "GSM,"); healthy = false; }
    if(!_temps.isOperational())  { strcat(_healthStatus, "TMP,"); healthy = false; }
    
    if(!healthy) {
        _healthStatus[strlen(_healthStatus)-1] = '\0';
        if(strcmp(lastHealthStatus, _healthStatus) != 0) {
            _logEvent(MsgID::HEALTH_FAIL, _healthStatus);
            strcpy(lastHealthStatus, _healthStatus);
        }
    } else if(lastHealthStatus[0] != '\0') {
        lastHealthStatus[0] = '\0';
    }
    return healthy;
}

bool SystemManager::checkSystemHealth() {
    return _checkSystemHealth(); // Просто вызываем внутренний метод
}


// Static callback handlers
void SystemManager::_handleSmoke1AlertStatic(float ppm, bool isCritical) {
    if(_smoke1Instance) _smoke1Instance->_handleSmoke1Alert(ppm, isCritical);
}

void SystemManager::_handleSmoke2AlertStatic(float ppm, bool isCritical) {
    if(_smoke2Instance) _smoke2Instance->_handleSmoke2Alert(ppm, isCritical);
}

void SystemManager::_handleMotionStatic() {
    if(_motionInstance) _motionInstance->_handleMotion();
}

void SystemManager::_handleIButtonAccessStatic(const uint8_t* keyId) {
    if(_instanceForIButton) _instanceForIButton->_handleIButtonAccess(keyId);
}

void SystemManager::_handleDoorEventStatic(DoorSensor::StateChange change) {
    if(_doorInstance) _doorInstance->_handleDoorEvent(change);
}

void SystemManager::_handleGateEventStatic(DoorSensor::StateChange change) {
    if(_gateInstance) _gateInstance->_handleGateEvent(change);
}

// Instance handlers
void SystemManager::_handleSmoke1Alert(float ppm, bool isCritical) {
    if(isCritical) _handleFireAlert(ppm, _smoke2.getPPM());
}

void SystemManager::_handleSmoke2Alert(float ppm, bool isCritical) {
    if(isCritical) _handleFireAlert(_smoke1.getPPM(), ppm);
}

void SystemManager::_handleMotion() {
    if(_state == SystemState::ARMED) {
        _handleSecurityBreach(MsgID::ALRM_INTRUSION);
    }
}

void SystemManager::_handleIButtonAccess(const uint8_t* keyId) {
    if(verifyIButtonKey(keyId)) {
        if(_state == SystemState::ARMED || _state == SystemState::FIRE_ALERT || 
           _state == SystemState::INTRUSION_ALERT) {
            disarmSystem();
        } else {
            armSystem();
        }
    } else {
        _logEvent(MsgID::BAD_IBUTTON);
    }
}

void SystemManager::_handleDoorEvent(DoorSensor::StateChange change) {
    if(change == DoorSensor::StateChange::OPENED && _state == SystemState::ARMED) {
        _handleSecurityBreach(MsgID::ALRM_INTRUSION);
    }
}

void SystemManager::_handleGateEvent(DoorSensor::StateChange change) {
    if(change == DoorSensor::StateChange::OPENED && _state == SystemState::ARMED) {
        _handleSecurityBreach(MsgID::ALRM_INTRUSION);
    }
}

void SystemManager::_handleFireAlert(float ppm1, float ppm2) {
    if(_state == SystemState::DISARMED || _state == SystemState::ARMING) return;
    
    bool smokeDetected = _smoke1.isSmokeDetected() || _smoke2.isSmokeDetected() || 
                        _smokeRelay.isSmokeDetected();
    if(!smokeDetected) return;
    
    if(!_checkSmokeConsistency(ppm1, ppm2)) {
        _logEvent(MsgID::SMOKE_MISMATCH);
        _buzzer.shortBeep(3);
        return;
    }
    
    float maxPPM = max(ppm1, ppm2);
    char ppmStr[8];
    dtostrf(maxPPM, 4, 1, ppmStr);
    
    if(_smokeRelay.isSmokeDetected() || maxPPM >= _smokeCriticalThreshold) {
        _changeState(SystemState::FIRE_ALERT, MsgID::ALRM_FIRE, ppmStr);
    } else if(maxPPM >= _smokeWarningThreshold) {
        _buzzer.longBeep(2);
        _redLed.longBlink();
        _sendAlertNotification(MsgID::ALRM_FIRE, ppmStr);
    }
}

bool SystemManager::verifyIButtonKey(const uint8_t* key) {
    for(uint8_t i = 0; i < _numKeys; i++) {
        if(memcmp(_authorizedKeys[i], key, 8) == 0) {
            return true;
        }
    }
    return false;
}

bool SystemManager::addAuthorizedKey(const uint8_t* key) {
    if(_numKeys >= 3 || verifyIButtonKey(key)) return false;
    memcpy(_authorizedKeys[_numKeys++], key, 8);
    _logEvent(MsgID::KEY_ADDED);
    return true;
}

bool SystemManager::removeAuthorizedKey(const uint8_t* key) {
    for(uint8_t i = 0; i < _numKeys; i++) {
        if(memcmp(_authorizedKeys[i], key, 8) == 0) {
            for(uint8_t j = i; j < _numKeys - 1; j++) {
                memcpy(_authorizedKeys[j], _authorizedKeys[j+1], 8);
            }
            _numKeys--;
            _logEvent(MsgID::KEY_REMOVED);
            return true;
        }
    }
    return false;
}

bool SystemManager::verifyPhoneNumber(const char* number) const {
    return (number && strcmp(number, _adminPhone1) == 0) || 
           (number && _adminPhone2[0] && strcmp(number, _adminPhone2) == 0) ||
           (number && _userPhone1[0] && strcmp(number, _userPhone1) == 0) ||
           (number && _userPhone2[0] && strcmp(number, _userPhone2) == 0);
}

void SystemManager::handleIncomingCall(const String& number) {
    char buffer[12];
    number.toCharArray(buffer, sizeof(buffer));
    
    if (!verifyPhoneNumber(buffer)) return;

    if (_state == SystemState::ARMED) {
        disarmSystem();
        _garageLight.toggleLight();  // Используем GarageLight!
        _logEvent(MsgID::SYS_DISARMED, "BY_CALL");
    } 
    else {
        _garageLight.toggleLight();  // Используем GarageLight!
    }
}

