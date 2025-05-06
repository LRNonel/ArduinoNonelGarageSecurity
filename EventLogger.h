#ifndef EVENT_LOGGER_H
#define EVENT_LOGGER_H

#include <EEPROM.h>
#include <Arduino.h>

class EventLogger {
public:
    enum EventType {
        ALARM_TRIGGERED = 1,
        DOOR_OPENED,
        MOTION_DETECTED,
        SMOKE_DETECTED,
        UNKNOWN_EVENT = 255
    };

    struct LogEntry {
        uint32_t timestamp;
        EventType type;
    };

    EventLogger(uint16_t startAddress = 0, uint16_t maxEntries = 100);
    
    bool logEvent(EventType type);  // Type only
    bool logEvent(EventType type, const char* message);  // Type + message
    bool logEvent(const char* message);  // Raw string message
    void clearLog();
    bool clearEEPROM(); // Полная очистка выделенной области
    void printLogs() const;
    uint16_t getEventCount(EventType type = UNKNOWN_EVENT) const;
    bool getLastEvents(LogEntry* buffer, uint16_t count) const;
    String eventTypeToString(EventType type) const;

private:
    uint16_t _startAddr;
    uint16_t _maxEntries;
    uint16_t _currentIndex = 0;
    bool _wrappedAround = false;
    
    bool _isValidType(EventType type) const;
    bool _writeEntry(const LogEntry& entry);
    bool _readEntry(uint16_t index, LogEntry& entry) const;
    uint16_t _getActualEntryCount() const;
};

#endif