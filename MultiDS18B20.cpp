#include "MultiDS18B20.h"
#include <Arduino.h>

MultiDS18B20::MultiDS18B20(uint8_t pin) 
    : _oneWire(pin), _sensors(&_oneWire) {}

void MultiDS18B20::begin() {
    _sensors.begin();
    _sensors.setWaitForConversion(false); // Enable async mode
    discoverSensors();
}

void MultiDS18B20::update() {
    unsigned long currentMillis = millis();
    
    // Time for new reading request
    if(currentMillis - _lastRequestTime >= _requestInterval) {
        requestTemperatures();
        _lastRequestTime = currentMillis;
    }
    
    // Process completed conversion
    if(_sensors.isConversionComplete()) {
        if(_garageFound) {
            _garageTemp = _sensors.getTempC(_garageAddr);
            if(_tempCallback) _tempCallback("Gar", _garageTemp);
        }
        
        if(_outdoorFound) {
            _outdoorTemp = _sensors.getTempC(_outdoorAddr);
            if(_tempCallback) _tempCallback("Out", _outdoorTemp);
        }
        
        // Restart conversion
        requestTemperatures();
    }
}

void MultiDS18B20::requestTemperatures() {
    _sensors.requestTemperatures();
}

float MultiDS18B20::getGarageTemp() const {
    return _garageTemp;
}

float MultiDS18B20::getOutdoorTemp() const {
    return _outdoorTemp;
}

bool MultiDS18B20::setSensorAddress(const char* location, const uint8_t* addr) {
    if(strcmp(location, "Gar") == 0) {
        memcpy(_garageAddr, addr, 8);
        _garageFound = true;
        return true;
    }
    else if(strcmp(location, "Out") == 0) {
        memcpy(_outdoorAddr, addr, 8);
        _outdoorFound = true;
        return true;
    }
    return false;
}

void MultiDS18B20::setTemperatureCallback(TemperatureCallback callback) {
    _tempCallback = callback;
}

void MultiDS18B20::discoverSensors() {
    Serial.println("Scan DS18B20");
    int deviceCount = _sensors.getDeviceCount();
    
    if(deviceCount == 0) {
        Serial.println("No found!");
        return;
    }
    
    Serial.print("Found ");
    Serial.print(deviceCount);
    Serial.println(" sen");
    
    uint8_t addr[8];
    for(int i = 0; i < deviceCount; i++) {
        if(_sensors.getAddress(addr, i)) {
            Serial.print("Sensor ");
            Serial.print(i + 1);
            Serial.print(": ");
            printAddress(addr);
            
            // Auto-assign first sensor as Garage
            if(!_garageFound) {
                setSensorAddress("Garage", addr);
                Serial.println(" (As as Gar)");
            } 
            // Second as Outdoor
            else if(!_outdoorFound) {
                setSensorAddress("Outdoor", addr);
                Serial.println(" (As as Out)");
            }
        }
    }
}

void MultiDS18B20::printAddress(const uint8_t* addr) {
    for(uint8_t i = 0; i < 8; i++) {
        if(addr[i] < 16) Serial.print("0");
        Serial.print(addr[i], HEX);
        if(i < 7) Serial.print(" ");
    }
}

bool MultiDS18B20::isOperational() const {
    // Check if we have any sensors connected
    if(_sensors.getDeviceCount() == 0) {
        return false;
    }
    
    // Check if we found our expected sensors
    if(!_garageFound && !_outdoorFound) {
        return false;
    }
    
    // Check if temperatures are valid (not disconnected)
    if(_garageFound && _garageTemp == DEVICE_DISCONNECTED_C) {
        return false;
    }
    
    if(_outdoorFound && _outdoorTemp == DEVICE_DISCONNECTED_C) {
        return false;
    }
    
    return true;
}