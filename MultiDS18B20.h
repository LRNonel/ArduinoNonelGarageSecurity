#ifndef MULTI_DS18B20_H
#define MULTI_DS18B20_H

#include <OneWire.h>
#include <DallasTemperature.h>

class MultiDS18B20 {
public:

	// Check if sensors not fail
	bool isOperational() const;
    
	// Callback type for temperature readings
    typedef void (*TemperatureCallback)(const char* location, float temp);

    // Constructor with OneWire bus pin
    explicit MultiDS18B20(uint8_t pin);
    
    // Initialization
    void begin();
    
    // Main update function (call in loop())
    void update();
    
    // Trigger new temperature conversion
    void requestTemperatures();
    
    // Get stored temperatures
    float getGarageTemp() const;
    float getOutdoorTemp() const;
    
    // Manual sensor address assignment
    bool setSensorAddress(const char* location, const uint8_t* addr);
    
    // Set callback for new readings
    void setTemperatureCallback(TemperatureCallback callback);
    
    // Scan bus and auto-assign sensors
    void discoverSensors();

private:
    OneWire _oneWire;                  // OneWire bus instance
    DallasTemperature _sensors;        // DS18B20 controller
    TemperatureCallback _tempCallback; // Reading callback
    
    // Sensor addresses
    uint8_t _garageAddr[8] = {0};     // Garage sensor ROM
    uint8_t _outdoorAddr[8] = {0};    // Outdoor sensor ROM
    bool _garageFound = false;         // Garage sensor status
    bool _outdoorFound = false;        // Outdoor sensor status
    
    // Temperature storage
    float _garageTemp = DEVICE_DISCONNECTED_C;
    float _outdoorTemp = DEVICE_DISCONNECTED_C;
    
    // Timing control
    unsigned long _lastRequestTime = 0;
    const unsigned long _requestInterval = 60000; // Reading interval (ms)
    
    // Helper to print ROM addresses
    void printAddress(const uint8_t* addr);
};

#endif