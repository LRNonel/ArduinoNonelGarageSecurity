//Setup libraries
#include <Alarm.h>
#include <Arduino.h>
#include <Buzzer.h>
#include <DallasTemperature.h>
#include <DoorSensor.h>
#include <EEPROM.h>
#include <EventLogger.h>
#include <GarageLight.h>
#include <GSMController.h>
#include <iButtonAccess.h>
#include <Led.h>
#include <MovingSensor.h>
#include <MultiDS18B20.h>
#include <OneWire.h>
#include <SmokeRelay.h>
#include <SmokeSensor.h>
#include <SystemManager.h>
#include <Wire.h>

#define DEBUG_MODE 0
#if DEBUG_MODE
  #define DEBUG_PRINT(x) Serial.print(x)
  #define DEBUG_PRINTLN(x) Serial.println(x)
#else
  #define DEBUG_PRINT(x)
  #define DEBUG_PRINTLN(x)
#endif

//PINs allocation
const uint8_t ALARM_PIN = A0;
const uint8_t DOOR_PIN = A1;
const uint8_t GATE_PIN = A2;
const uint8_t LIGHT_RELAY_PIN = A3;
const uint8_t MOTION_PIN = A4;
const uint8_t SMOKE_RELAY_PIN = A5;
const uint8_t MQ7_1_PIN = A6;
const uint8_t MQ7_2_PIN = A7;
const uint8_t GSM_PWR = 2;
const uint8_t GSM_RX = 4;
const uint8_t GSM_TX = 3;
const uint8_t TEMP_PIN = 5;
const uint8_t BUZZER_PIN = 6;
const uint8_t RED_LED = 7;
const uint8_t YELLOW_LED = 9;
const uint8_t MQ7_HEAT_PIN = 10;
const uint8_t IBUTTON_PIN = 11;
const uint8_t LIGHT_FEEDBACK_PIN = 12;
const uint8_t GREEN_LED = 13;
const uint16_t EEPROM_START_ADDR = 0;


// Module instances
GSMController gsm(GSM_RX, GSM_TX, GSM_PWR);
iButtonAccess ibutton(IBUTTON_PIN);
Alarm alarm(ALARM_PIN);
Buzzer buzzer(BUZZER_PIN);
Led redLed(RED_LED);
Led yellowLed(YELLOW_LED);
Led greenLed(GREEN_LED);
DoorSensor doorSensor(DOOR_PIN, DoorSensor::SensorType::NORMALLY_OPEN);
DoorSensor gateSensor(GATE_PIN, DoorSensor::SensorType::NORMALLY_OPEN);
GarageLight garageLight(LIGHT_RELAY_PIN, LIGHT_FEEDBACK_PIN);
SmokeSensor smokeSensor1(MQ7_1_PIN, MQ7_HEAT_PIN);
SmokeSensor smokeSensor2(MQ7_2_PIN, MQ7_HEAT_PIN);
SmokeRelay smokeRelay(SMOKE_RELAY_PIN);
MovingSensor motionSensor(MOTION_PIN, true);
MultiDS18B20 temps(TEMP_PIN);
EventLogger logger(EEPROM_START_ADDR);
SystemManager systemManager(gsm, alarm, smokeSensor1, smokeSensor2, doorSensor, gateSensor, ibutton, logger, buzzer, temps, smokeRelay, redLed, yellowLed, greenLed, motionSensor, garageLight);
static void callEventHandler(const String& number, GSMController::CallStatus status) {
  if (status == GSMController::CallStatus::INCOMING_CALL) {
    systemManager.handleIncomingCall(number);
  }
}



void setup() {
  /*playMelody();
  */
  Serial.begin(9600);
  gsm.begin(10000);
  DEBUG_PRINTLN(F("GSM init"));
  ibutton.begin();
  alarm.init();
  redLed.begin();
  yellowLed.begin();
  greenLed.begin();
  garageLight.begin();
  motionSensor.begin();
  temps.begin();
  systemManager.begin();
  systemManager.setSmokeDifferential(20.0);
  ibutton.setAccessGrantedCallback([](const uint8_t* keyId) {
    DEBUG_PRINT("Acc all! Key ID: ");
    iButtonAccess::printKey(keyId);
  });

  ibutton.setAccessDeniedCallback([](const uint8_t* keyId) {
    DEBUG_PRINT("Acc den! Key ID: ");
    iButtonAccess::printKey(keyId);
  });
  gsm.onCallEvent([](const String& num, GSMController::CallStatus status) {
    if (status == GSMController::CallStatus::INCOMING_CALL) {
      systemManager.handleIncomingCall(num);
    }
  });
}

void printGSMStatus() {
  static unsigned long lastPrint = 0;
  if (millis() - lastPrint > 5000) {
    lastPrint = millis();
    DEBUG_PRINT("GSM St: ");
    switch (gsm.getNetworkStatus()) {
      case GSMController::NetworkStatus::DISCONNECTED:
        DEBUG_PRINTLN("N reg");
        break;
      case GSMController::NetworkStatus::REGISTERED_HOME:
        DEBUG_PRINTLN("Reg (h)");
        break;
      case GSMController::NetworkStatus::ERROR:
        DEBUG_PRINTLN("Er");
        break;
    }
  }
}

void handleSystemState() {
  static uint16_t lastCheckSec = 0;      // 2 байта (вместо 4)
  static uint16_t lastGsmCheckSec = 0;   // 2 байта (вместо 4)
  const uint16_t nowSec = millis() / 1000; // Точность в секундах

  // Проверка GSM каждые 5 секунд (было 5000ms)
  if ((uint16_t)(nowSec - lastGsmCheckSec) >= 5) {
    lastGsmCheckSec = nowSec;
    
    if (gsm.getNetworkStatus() < GSMController::NetworkStatus::REGISTERED_HOME) {
      DEBUG_PRINTLN(F("GSM reconnect"));
      gsm.begin(10000);
    }
  }

  // Проверка здоровья системы каждые 5 минут (было 300000ms)
  if ((uint16_t)(nowSec - lastCheckSec) >= 300) { // 300 сек = 5 мин
    lastCheckSec = nowSec;
    systemManager.checkSystemHealth();
  }

  // Обработка состояний системы (без изменений)
  SystemManager::SystemState currentState = systemManager.getState();
  switch (currentState) {
    case SystemManager::SystemState::DISARMED:
      handleDisarmedState();
      break;

    case SystemManager::SystemState::ARMING:
      handleArmingState();
      break;

    case SystemManager::SystemState::ARMED:
      break;

    case SystemManager::SystemState::FIRE_ALERT:
    case SystemManager::SystemState::INTRUSION_ALERT:
      handleAlertState(currentState);
      break;

    case SystemManager::SystemState::MAINTENANCE:
      handleMaintenanceState();
      break;
  }
}

void loop() {
  printGSMStatus();
  systemManager.update();  // Основной цикл обработки
  gsm.update();
  handleSystemState();
  delay(10);
}

void handleDisarmedState() {
  // Nothing special needed here - all handled by callbacks
}

void handleArmingState() {
  unsigned long remaining = systemManager.getArmingRemaining();
  DEBUG_PRINT("Arm in: ");
  DEBUG_PRINTLN(remaining / 1000);
}


void handleAlertState(SystemManager::SystemState alertType) {
  // Periodic alert updates (every minute)
  static unsigned long lastAlertUpdate = 0;
  if (millis() - lastAlertUpdate > 60000) {
    lastAlertUpdate = millis();

    String alertMsg = (alertType == SystemManager::SystemState::FIRE_ALERT) ? "FIRE AL ACT" : "INTR AL ACT";
    alertMsg += " for " + String(systemManager.getStateDuration() / 60000) + " minutes";

    DEBUG_PRINTLN(alertMsg);

    // Send to all registered numbers
    gsm.sendSMS(systemManager.getAdminPhone1(), alertMsg);
    if (!systemManager.hasAdminPhone2()) {
      gsm.sendSMS(systemManager.getAdminPhone2(), alertMsg);
    }
    if (!systemManager.hasUserPhone1()) {
      gsm.sendSMS(systemManager.getUserPhone1(), alertMsg);
    }
    if (!systemManager.hasUserPhone2()) {
      gsm.sendSMS(systemManager.getUserPhone2(), alertMsg);
    }
  }
}

void handleMaintenanceState() {
  // Log sensor status periodically (every 10 seconds)
  static unsigned long lastLog = 0;
  if (millis() - lastLog > 10000) {
    lastLog = millis();
    DEBUG_PRINTLN("Maint act");
  }
}

void handleSms(const String& number, const String& text) {
  // Verify sender is authorized
  if (!systemManager.verifyPhoneNumber(number.c_str())) {
    DEBUG_PRINTLN("SMS: " + number);
    return;
  }

  // Process commands (case insensitive)
  String command = text;
  command.toUpperCase();
  command.trim();

  if (strcmp_P(command.c_str(), PSTR("STATUS")) == 0) {
    char status[30];  // Adjust size as needed
    snprintf_P(status, sizeof(status), PSTR("Sys st: %s"), systemManager.getStateString());
    gsm.sendSMS(number.c_str(), status);
  } else if (command == "ARM") {
    if (systemManager.armSystem()) {
      gsm.sendSMS(number, "Sys arm init");
    } else {
      gsm.sendSMS(number, "Cannot arm - inv state");
    }
  } else if (command == "DISARM") {
    if (systemManager.disarmSystem()) {
      gsm.sendSMS(number, "Sys disarm");
    } else {
      gsm.sendSMS(number, "Disarm fail");
    }
  } else if (strcmp_P(command.c_str(), PSTR("TEMP")) == 0) {
    char message[16];  // "TEMP:T:GG.OO" + null terminator = 12 bytes
    char tempCode[8];  // For optimized "T:GG.OO" format

    // Get compact temperature readings (values *10)
    systemManager.getTemperatureReadings(tempCode);

    // Format complete message
    snprintf_P(message, sizeof(message), PSTR("TEMP:%s"), tempCode);

    // Send SMS
    gsm.sendSMS(number.c_str(), message);
  } else {
    gsm.sendSMS(number, "Unk com. Val: STATUS, ARM, DISARM, TEMP");
  }
}

void handleStateChange(SystemManager::SystemState state, const String& message) {
  DEBUG_PRINTLN("St change: " + message);

  // Notify admin about important state changes
  if (state == SystemManager::SystemState::ARMED || state == SystemManager::SystemState::DISARMED || state == SystemManager::SystemState::MAINTENANCE) {
    gsm.sendSMS(systemManager.getAdminPhone1(), message);
    if (!systemManager.hasAdminPhone2()) {
      gsm.sendSMS(systemManager.getAdminPhone2(), message);
    }
  }
}

void handleAlert(SystemManager::SystemState state, const String& message) {
  DEBUG_PRINTLN("ALERT: " + message);

  // Call admin phones for critical alerts
  if (state == SystemManager::SystemState::FIRE_ALERT) {
    gsm.makeCall(systemManager.getAdminPhone1());
  } else if (state == SystemManager::SystemState::INTRUSION_ALERT) {
    gsm.makeCall(systemManager.getAdminPhone1());
    if (!systemManager.hasAdminPhone2()) {
      delay(5000);  // Wait before second call
      gsm.makeCall(systemManager.getAdminPhone2());
    }
  }
}


