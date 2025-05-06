#include "GSMController.h"

GSMController::GSMController(uint8_t rxPin, uint8_t txPin, uint8_t powerPin) 
    : _serial(rxPin, txPin), _powerPin(powerPin) {}

bool GSMController::begin(unsigned long timeout) {
    _serial.begin(9600);
    delay(1000); // Initial delay
    
    if(_powerPin != -1) {
        pinMode(_powerPin, OUTPUT);
        digitalWrite(_powerPin, LOW); // Ensure power is off first
        delay(1000);
        digitalWrite(_powerPin, HIGH);
        delay(3000); // Increased delay for power stabilization
    }

    // Check if module responds
    if (!_sendATCommand("AT", "OK", 3000)) {
        _changeStatus(NetworkStatus::ERROR);
        return false;
    }

    // Add delays between commands
    _sendATCommand("ATE0", "OK", 1000); // Disable echo
    delay(500);
    _sendATCommand("AT+CMEE=1", "OK", 1000); // Enable verbose errors
    delay(500);
    _sendATCommand("AT+CMGF=1", "OK", 1000); // Text mode
    delay(500);
    _sendATCommand("AT+CNMI=1,2,0,0,0", "OK", 1000); // SMS notifications
    delay(500);
    _sendATCommand("AT+CLIP=1", "OK", 1000); // Caller ID
    delay(500);
    
    // Check CREG with more retries
    for (int i = 0; i < 3; i++) {
        if (_waitForNetwork(5000)) {
            return true;
        }
        delay(1000);
    }
    
    _changeStatus(NetworkStatus::ERROR);
    return false;
}

void GSMController::update() {
    if(_serial.available()) {
        String data = _readSerial();
        if(data.length() > 0) {
            _processIncomingData(data);
            _lastResponseTime = millis();
        }
    }
}

bool GSMController::sendSMS(const String& number, const String& text) {
    if(_status < NetworkStatus::REGISTERED_HOME) return false;

    _serial.print("AT+CMGS=\"");
    _serial.print(number);
    _serial.println("\"");
    
    if(!_waitForResponse(">", 2000)) return false;
    
    _serial.print(text);
    _serial.write(26); // Ctrl+Z
    delay(100); // Short delay after Ctrl+Z
    
    return _waitForResponse("+CMGS:", 10000); // Longer timeout for SMS sending
}

bool GSMController::makeCall(const String& number) {
    if(_status < NetworkStatus::REGISTERED_HOME) return false;
    
    _serial.print("ATD");
    _serial.print(number);
    _serial.println(";");
    
    if(!_waitForResponse("OK", 3000)) {
        _callStatus = CallStatus::NO_CALL;
        return false;
    }
    
    _callStatus = CallStatus::ACTIVE_CALL;
    return true;
}

void GSMController::setLowPowerMode(bool enable) {
    if (enable) {
        _sendATCommand("AT+CFUN=0", "OK", 1000);
    } else {
        _sendATCommand("AT+CFUN=1", "OK", 1000);
    }
}

// Callbacks
void GSMController::onSmsReceived(SmsCallback callback) {
    _smsCallback = callback;
}

void GSMController::onCallEvent(CallCallback callback) {
    _callCallback = callback;
}

void GSMController::onNetworkChange(StatusCallback callback) {
    _statusCallback = callback;
}

// Private methods
String GSMController::_readSerial(unsigned long timeout) {
    String data;
    unsigned long start = millis();
    while(millis() - start < timeout) {
        while(_serial.available()) {
            char c = _serial.read();
            data += c;
            // Keep reading until timeout or we get a complete response
            if(data.endsWith("\r\nOK\r\n") || data.endsWith("\r\nERROR\r\n")) {
                return data;
            }
        }
    }
    return data;
}

String GSMController::_sendATCommand(const char* cmd, const char* expected, unsigned long timeout, bool storeResponse = false) {
    _serial.println(cmd);
    String response;
    unsigned long start = millis();
    
    while (millis() - start < timeout) {
        if (_serial.available()) {
            char c = _serial.read();
            response += c;
            
            if (response.endsWith("\r\n") || response.endsWith("OK\r\n") || 
                response.endsWith("ERROR\r\n") || response.indexOf(expected) != -1) {
                break;
            }
        }
    }
    
    if (storeResponse) {
        return response;
    }
    return response.indexOf(expected) != -1 ? "OK" : "";
}


bool GSMController::_waitForResponse(const char* expected, unsigned long timeout) {
    unsigned long start = millis();
    String response;
    
    while(millis() - start < timeout) {
        if(_serial.available()) {
            char c = _serial.read();
            response += c;
            
            if(response.indexOf(expected) != -1) {
                return true;
            }
            if(response.indexOf("ERROR") != -1) {
                return false;
            }
        }
    }
    return false;
}

void GSMController::_processIncomingData(const String& data) {
    if(data.startsWith("+CMTI:")) {
        // Новое SMS
        if(_smsCallback) {
            int index = data.indexOf(',');
            String smsIndex = data.substring(index + 1);
            _serial.println("AT+CMGR=" + smsIndex);
            String smsData = _readSerial(1000);
            // Парсим номер и текст сообщения
            String number = _extractNumber(smsData);
            String text = smsData.substring(smsData.indexOf('\n') + 1);
            _smsCallback(number, text);
        }
    }
    else if(data.startsWith("+CLIP:")) {
        // Входящий звонок с определением номера
        if(_callCallback) {
            String number = _extractNumber(data);
            _callStatus = CallStatus::INCOMING_CALL;
            _callCallback(number, _callStatus);
        }
    }
    else if(data.startsWith("NO CARRIER") || data.startsWith("BUSY")) {
        // Звонок завершен
        _callStatus = CallStatus::CALL_ENDED;
        if(_callCallback) {
            _callCallback("", _callStatus);
        }
    }
	else if(data.startsWith("+CREG:")) {
		// Изменение статуса сети
		int status = data.charAt(data.length() - 1) - '0';
		NetworkStatus newStatus;
		switch(status) {
			case 1: newStatus = NetworkStatus::REGISTERED_HOME; break;
			case 0: newStatus = NetworkStatus::DISCONNECTED; break;
			default: newStatus = NetworkStatus::ERROR;
		}
		_changeStatus(newStatus);
	}	
}

void GSMController::_changeStatus(NetworkStatus newStatus) {
    if(_status != newStatus) {
        _status = newStatus;
        if(_statusCallback) {
            _statusCallback(_status);
        }
    }
}

String GSMController::_extractNumber(const String& data) const {
    int start = data.indexOf('\"') + 1;
    int end = data.indexOf('\"', start);
    return data.substring(start, end);
}

bool GSMController::_waitForNetwork(unsigned long timeout) {
    unsigned long start = millis();
    while(millis() - start < timeout) {
        // Более гибкая проверка ответа CREG
        String response = _sendATCommand("AT+CREG?", "+CREG:", 2000, true);
        
        if (response.indexOf("+CREG: 0,1") != -1 || response.indexOf("+CREG: 1,1") != -1) {
            _changeStatus(NetworkStatus::REGISTERED_HOME);
            return true;
        }
        delay(1000);
    }
    
    // Если не удалось зарегистрироваться, проверяем базовую работоспособность
    if (_sendATCommand("AT", "OK", 1000)) {
        _changeStatus(NetworkStatus::DISCONNECTED);
    } else {
        _changeStatus(NetworkStatus::ERROR);
    }
    return false;
}

GSMController::NetworkStatus GSMController::getNetworkStatus() const {
    return _status;
}

bool GSMController::isOperational() const {
    // Быстрая проверка питания
    if(_powerPin != -1 && digitalRead(_powerPin) != HIGH) {
        return false;
    }

    // Проверяем, отвечает ли модуль
    if (!_sendATCommand("AT", "OK", 1000)) {
        return false;
    }

    // Проверяем регистрацию в сети
    String cregResponse = _sendATCommand("AT+CREG?", "+CREG:", 2000, true);
    return (cregResponse.indexOf(",1") != -1 || cregResponse.indexOf(",5") != -1);
}