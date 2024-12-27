#include <Arduino.h>
#include <LiquidCrystal_I2C.h>
#include <I2CKeyPad.h>
#include <Wire.h>

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLE2902.h>

// Definitions
#define RED_LIGHT_PIN 5
#define GREEN_LIGHT_PIN 18
#define BUZZER_PIN 4
#define BUZZER_CHANNEL 0
#define SERVICE_UUID "19B10010-E8F2-537E-4F6C-D104768A1214"
#define STATE_CHARACTERISTIC_UUID "19B10011-E8F2-537E-4F6C-D104768A1214"
#define USERNAME_CHARACTERISTIC_UUID "19B10012-E8F2-537E-4F6C-D104768A1214"
#define PASSWORD_CHARACTERISTIC_UUID "19B10013-E8F2-537E-4F6C-D104768A1214"

// I2C Addresses
#define KEYPAD_ADDRESS 0x20
#define LCD_ADDRESS 0x27

// This represent the different states of the device
enum class State {
  ACTIVE,
  WRONG_PASSWORD,
  DEFUSED,
};

// The state is stored in a volatile variable
volatile State state = State::ACTIVE;
volatile bool doStateTransition = true;

// Globals
LiquidCrystal_I2C lcd(LCD_ADDRESS, 16, 2);
I2CKeyPad keypad(KEYPAD_ADDRESS);

String password = "1234";
String username = "";
String userInput = "";
char lastKey = 'N';

BLEServer *bleServer = NULL;
BLECharacteristic stateChar(STATE_CHARACTERISTIC_UUID, BLECharacteristic::PROPERTY_READ);
BLECharacteristic usernameChar(USERNAME_CHARACTERISTIC_UUID, BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE);
BLECharacteristic passwordChar(PASSWORD_CHARACTERISTIC_UUID, BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE);

// Handles
TaskHandle_t stateMachineTaskHandle;
TaskHandle_t activeBeepTaskHandle;

// Function prototypes
void changeStateTo(State newState);
void stateMachineTask(void * parameter);
void beep(int pin, int duration, note_t note, uint8_t octave);
void activeBeepTask(void * parameter);
void handleKeypadEvent();
void updateLCD();
void updateBLE();
void setupBLE();

// This represent what to do when transitioning
// Possible transitions are the following:
// - ACTIVE -> WRONG_PASSWORD
// - WRONG_PASSWORD -> ACTIVE
// - ACTIVE -> DEFUSED
// - DEFUSED -> ACTIVE
void changeStateTo(State newState) {
  state = newState;
  doStateTransition = true;
}

// State transitions are applied in this function,
// This is run on its thread
void stateMachineTask(void * parameter) {
  Serial.println("State Machine Task started");
  while (true) {
    if (doStateTransition) {
      doStateTransition = false;
      switch (state) {
        case State::ACTIVE:
          Serial.println("Transitioning to: ACTIVE");
          digitalWrite(RED_LIGHT_PIN, LOW);
          digitalWrite(GREEN_LIGHT_PIN, LOW);
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("Enter password:");
          userInput = ""; // Reset user input
          break;
        case State::WRONG_PASSWORD:
          Serial.println("Transitioning to: WRONG_PASSWORD");
          digitalWrite(GREEN_LIGHT_PIN, LOW);
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("Wrong pass !");
          for (int i = 0; i < 4; ++i) {
              digitalWrite(RED_LIGHT_PIN, HIGH);
              beep(BUZZER_CHANNEL, 50, NOTE_F, 4);
              vTaskDelay(50 / portTICK_PERIOD_MS);
              digitalWrite(RED_LIGHT_PIN, LOW);
              vTaskDelay(50 / portTICK_PERIOD_MS);
          }
          vTaskDelay(1000 / portTICK_PERIOD_MS);
          changeStateTo(State::ACTIVE);
          userInput = "";
          break;
        case State::DEFUSED:
          Serial.println("Transitioning to: DEFUSED");
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("Congratulations");
          lcd.setCursor(0, 1);
          lcd.print(username);
          digitalWrite(GREEN_LIGHT_PIN, HIGH);
          vTaskDelay(60000 / portTICK_PERIOD_MS);
          changeStateTo(State::ACTIVE);
          break;
      }
    }
    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
}

// This function is an utility function to play a beep and beep the light
// Using ledcWriteTone and ledcWrite
void beep(int channel, int duration, note_t note, uint8_t octave = 4) {
  digitalWrite(RED_LIGHT_PIN, LOW);
  // ledcWriteNote(channel, note, octave);
  vTaskDelay(duration / portTICK_PERIOD_MS);
  ledcWrite(channel, 0);
  digitalWrite(RED_LIGHT_PIN, HIGH);
}

// Beep only when in active state
void activeBeepTask(void * parameter) {
  Serial.println("Active Beep Task started");
  while (true) {
    if (state == State::ACTIVE) {
      beep(BUZZER_CHANNEL, 50, NOTE_Eb, 7);
      vTaskDelay(1000 / portTICK_PERIOD_MS);
    } else {
      vTaskDelay(100 / portTICK_PERIOD_MS);
    }
  }
}

// Keep track of user input
// The keypad will always return a key, without blocking, 
// so we need to implement our way to avoid multiple key presses
// We can do this by checking the last key pressed, since no key presses correspond to "N"
void handleKeypadEvent() {
  char key = keypad.getChar();
  if (key == 'F' || key == 'N') {
    lastKey = 'F';
    return;
  }

  if (key == lastKey) {
    return;
  }

  Serial.print("Key pressed: ");
  Serial.println(key);

  userInput += key;
  updateLCD();
  if (userInput.length() == password.length()) {
    if (userInput == password) {
      changeStateTo(State::DEFUSED);
    } else {
      changeStateTo(State::WRONG_PASSWORD);
    }
  }

  lastKey = key;
}

// We also need to keep the LCD updated,
// In ACTIVE state, the second line correspond to the centered user input
void updateLCD() {
  if (state == State::ACTIVE) {
    lcd.setCursor(0, 1);

    int num_spaces = (16 - userInput.length()) / 2;
    lcd.print(String("                ").substring(0, num_spaces) + userInput + String("                ").substring(0, num_spaces));
  }
}

// We also need to keep the BLE characteristic updated
// In ACTIVE state, the characteristic will be the user input
void updateBLE() {
  String stateString;
  switch (state) {
    case State::ACTIVE:
      stateString = "ACTIVE : '" + userInput + "'";
      stateChar.setValue(stateString.c_str());
      break;
    case State::WRONG_PASSWORD:
      stateChar.setValue("WRONG_PASSWORD");
      break;
    case State::DEFUSED:
      stateChar.setValue("DEFUSED");
      break;
  }
}

class BLECallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      pServer->startAdvertising(); // restart advertising
    };

    void onDisconnect(BLEServer* pServer) {
      pServer->startAdvertising(); // restart advertising
    }
};

class PasswordCallbacks: public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) {
    std::string value = pCharacteristic->getValue();
    if (value.length() > 0) {
      password = value.c_str();
      Serial.print("New password: ");
      Serial.println(password);
    }
  }
};

class UsernameCallbacks: public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) {
    std::string value = pCharacteristic->getValue();
    if (value.length() > 0) {
      username = value.c_str();
      Serial.print("New username: ");
      Serial.println(username);
    }
  }
};

// Setup function for BLE
void setupBLE() {
  BLEDevice::init("4K LGTV");
  bleServer = BLEDevice::createServer();

  BLEService *service = bleServer->createService(SERVICE_UUID);
  service->addCharacteristic(&stateChar);
  service->addCharacteristic(&usernameChar);
  service->addCharacteristic(&passwordChar);

  bleServer->setCallbacks(new BLECallbacks());

  passwordChar.setValue(password.c_str());
  passwordChar.setCallbacks(new PasswordCallbacks());

  usernameChar.setValue(username.c_str());
  usernameChar.setCallbacks(new UsernameCallbacks());

  service->start();

  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);
  pAdvertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();
}

void setup() {
  Serial.begin(9600);

  Serial.println("Setup PINs");

  // Setup the lights
  pinMode(RED_LIGHT_PIN, OUTPUT);
  pinMode(GREEN_LIGHT_PIN, OUTPUT);

  pinMode(2, OUTPUT);
  digitalWrite(2, HIGH);

  // Setup the buzzer
  ledcSetup(BUZZER_CHANNEL, 2000, 8);
  ledcAttachPin(BUZZER_PIN, BUZZER_CHANNEL);

  ledcWrite(BUZZER_CHANNEL, 0);


  Serial.println("Setup keypad");

  // Setup the keypad
  keypad.begin();
  char * layout = (char *) "123A456B789C*0#DNF";
  keypad.loadKeyMap(layout);

  Serial.println("Setup LCD");

  // Setup the LCD
  lcd.init();
  lcd.backlight();

  Serial.println("Setup BLE");

  // Setup the BLE server, one characteristic for the password (read/write)
  // and one characteristic for the state (read only)
  setupBLE();

  Serial.println("Setup tasks");

  // Setup the state machine task
  xTaskCreate(stateMachineTask, "State Machine Task", 2048, NULL, 1, &stateMachineTaskHandle);

  // Setup the active beep task
  xTaskCreate(activeBeepTask, "Active Beep Task", 2048, NULL, 1, &activeBeepTaskHandle);
}

void loop() {
  handleKeypadEvent();
  updateBLE();
  delay(10);
}
