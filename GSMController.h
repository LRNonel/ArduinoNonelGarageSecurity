#ifndef GSM_CONTROLLER_H
#define GSM_CONTROLLER_H
#define SMS_BUFFER_SIZE 32    
#define CMD_BUFFER_SIZE 32   
#include <SoftwareSerial.h>
#include <Arduino.h>

class GSMController {
public:

	bool isOperational() const;
    
enum class NetworkStatus {
    DISCONNECTED,      // 0 - не зарегистрирован
    REGISTERED_HOME,   // 2 - зарегистрирован в домашней сети
    ERROR              // 4 - ошибка
};

    enum class CallStatus {
        NO_CALL,
        INCOMING_CALL,
        ACTIVE_CALL,
        CALL_ENDED
    };

    typedef void (*SmsCallback)(const String& number, const String& text);
    typedef void (*CallCallback)(const String& number, CallStatus status);
    typedef void (*StatusCallback)(NetworkStatus status);

    GSMController(uint8_t rxPin, uint8_t txPin, uint8_t powerPin = -1);
    
    bool begin(unsigned long timeout = 10000);
    void update();
    bool sendSMS(const String& number, const String& text);
    bool makeCall(const String& number);
    void endCall();
    void setLowPowerMode(bool enable);
    NetworkStatus getNetworkStatus() const;
    
    // Callbacks
    void onSmsReceived(SmsCallback callback);
    void onCallEvent(CallCallback callback);
    void onNetworkChange(StatusCallback callback);

private:
    SoftwareSerial _serial;
    uint8_t _powerPin;
    NetworkStatus _status = NetworkStatus::DISCONNECTED;
    CallStatus _callStatus = CallStatus::NO_CALL;
    unsigned long _lastResponseTime = 0;
    
    SmsCallback _smsCallback = nullptr;
    CallCallback _callCallback = nullptr;
    StatusCallback _statusCallback = nullptr;
    
    String _readSerial(unsigned long timeout = 100);
    String _sendATCommand(const char* cmd, const char* expected, unsigned long timeout, bool storeResponse = false);
    void _processIncomingData(const String& data);
    void _changeStatus(NetworkStatus newStatus);
    String _extractNumber(const String& data) const;
    bool _waitForNetwork(unsigned long timeout);
	bool _waitForResponse(const char* expected, unsigned long timeout);
};

#endif