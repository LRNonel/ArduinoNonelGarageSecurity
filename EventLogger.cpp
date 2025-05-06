#include "EventLogger.h"

EventLogger::EventLogger(uint16_t startAddress, uint16_t maxEntries) 
    : _startAddr(startAddress), _maxEntries(maxEntries) {
		 if (startAddress + (maxEntries * sizeof(LogEntry)) > 1024) {
        Serial.println("EEPROM o/f!");
        while(1); // Halt if out of space
    }
}

bool EventLogger::_isValidType(EventType type) const {
    return type >= ALARM_TRIGGERED && type <= SMOKE_DETECTED;
}

bool EventLogger::logEvent(EventType type, const char* message) {
    if (!_isValidType(type)) {
        return false;
    }

    // You might want to store the message somewhere or print it
    Serial.print("Ev: ");
    Serial.print(eventTypeToString(type));
    Serial.print(" - Mes: ");
    Serial.println(message);

    LogEntry entry = {millis() / 1000, type};
    return _writeEntry(entry);
}

bool EventLogger::logEvent(const char* message) {
    
    Serial.print("Mes: ");
    Serial.println(message);

    LogEntry entry = {millis() / 1000, UNKNOWN_EVENT};
    return _writeEntry(entry);
}

bool EventLogger::logEvent(EventType type) {
    if (!_isValidType(type)) {
        return false;
    }

    LogEntry entry = {millis() / 1000, type}; // Сохраняем в секундах
    
    if (_writeEntry(entry)) {
        if (_currentIndex == 0 && !_wrappedAround) {
            _wrappedAround = true;
        }
        return true;
    }
    return false;
}

bool EventLogger::_writeEntry(const LogEntry& entry) {
    if (_currentIndex >= _maxEntries) {
        return false;
    }

    uint16_t address = _startAddr + (_currentIndex * sizeof(LogEntry));
    
    // Write each byte of the struct manually
    const uint8_t* p = (const uint8_t*)(&entry);
    for (size_t i = 0; i < sizeof(LogEntry); i++) {
        EEPROM.update(address + i, p[i]); // Update only if changed
    }

    _currentIndex = (_currentIndex + 1) % _maxEntries;
    return true;
}

bool EventLogger::_readEntry(uint16_t index, LogEntry& entry) const {
    if (index >= _maxEntries) {
        return false;
    }

    uint16_t address = _startAddr + (index * sizeof(LogEntry));
    EEPROM.get(address, entry);
    return entry.timestamp != 0xFFFFFFFF && _isValidType(entry.type);
}

void EventLogger::printLogs() const {
    LogEntry entry;
    uint16_t start = _wrappedAround ? _currentIndex : 0;
    uint16_t count = _getActualEntryCount();

    Serial.println("= Ev Log =");
    for(uint16_t i = 0; i < count; i++) {
        uint16_t idx = (start + i) % _maxEntries;
        if(_readEntry(idx, entry)) {
            Serial.print("[");
            Serial.print(entry.timestamp);
            Serial.print("] Ev: ");
            Serial.println(eventTypeToString(entry.type));
        }
    }
}

uint16_t EventLogger::getEventCount(EventType type) const {
    uint16_t count = 0;
    LogEntry entry;
    uint16_t totalEntries = _getActualEntryCount();
    uint16_t start = _wrappedAround ? _currentIndex : 0;

    for(uint16_t i = 0; i < totalEntries; i++) {
        uint16_t idx = (start + i) % _maxEntries;
        if(_readEntry(idx, entry) && (type == UNKNOWN_EVENT || entry.type == type)) {
            count++;
        }
    }
    return count;
}

bool EventLogger::getLastEvents(LogEntry* buffer, uint16_t count) const {
    if (!buffer || count == 0 || count > _maxEntries) {
        return false;
    }

    uint16_t totalEntries = _getActualEntryCount();
    count = min(count, totalEntries);
    uint16_t start = (_currentIndex + _maxEntries - count) % _maxEntries;

    for(uint16_t i = 0; i < count; i++) {
        uint16_t idx = (start + i) % _maxEntries;
        if(!_readEntry(idx, buffer[i])) {
            return false;
        }
    }
    return true;
}

void EventLogger::clearLog() {
    _currentIndex = 0;
    _wrappedAround = false;
}

bool EventLogger::clearEEPROM() {
    for(uint16_t i = 0; i < _maxEntries; i++) {
        uint16_t address = _startAddr + (i * sizeof(LogEntry));
        LogEntry emptyEntry{0, UNKNOWN_EVENT};
        const uint8_t* p = (const uint8_t*)(&emptyEntry);
        for (size_t j = 0; j < sizeof(LogEntry); j++) {
            EEPROM.update(address + j, p[j]);
        }
    }
    return true;
}

String EventLogger::eventTypeToString(EventType type) const {
    switch(type) {
        case ALARM_TRIGGERED: return "A";
        case DOOR_OPENED: return "B";
        case MOTION_DETECTED: return "C";
        case SMOKE_DETECTED: return "D";
        default: return "Z";
    }
}

uint16_t EventLogger::_getActualEntryCount() const {
    return _wrappedAround ? _maxEntries : _currentIndex;
}