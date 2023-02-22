#include <LiquidCrystal_I2C.h>
#include "CustomKeypad.h"
#define deltaTime(val) (millis()-val)

// c'est le keypad si t'avais pas capté
char layout[19] = "123A456B789C*0#DNF";  // N = NoKey, F = Fail
char valid[13] = "0123456789*#";
unsigned long lastKeyPress = 0;
unsigned long keyDelay = 250;
// Et la c'est le keypad avec la lib (0x20 c'est l'adresse I2C)
I2CKeyPad keyPad(0x20);

// Compteurs pour la LED et pour le BEEP (pour faire des trucs async avec une boucle sync)
unsigned long currentMillis = 0;
unsigned long prevMillisLed = 0;
unsigned long startTime = 0;
unsigned long prevMillisBeep = 0;
// Gére le temps avant le prochain beep
double timingUpBomb = 0;

// J'ai besoin de commenté ???
const String password = "1075043";
// Là aussi ?????!
String input_password;

// Pin des deux LEDs
const int ledRouge = 4;
const int ledVerte = 2;

// Si le code est valide
bool valide = false;
// Si on a déjà fait le premier beep accéléré
bool startedBeep = false;

// Alors 0x27 c'est l'adresse I2C, 16*2 c'est la résolution
LiquidCrystal_I2C lcd(0x27, 16, 2);

void setup() {
  
  // Setup du LCD et la premiere ligne qui change jamais
  lcd.init();
  lcd.clear();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Entrez le code :");

  // Pour le debug
  Serial.begin(9600);
  keyPad.begin();
  keyPad.loadKeyMap(layout);
  
  // On allume tout
  pinMode(ledVerte, OUTPUT);
  pinMode(ledRouge, OUTPUT);
  ledcAttachPin(32, 0);
  ledcSetup(0, 2500, 12);

  // Préalloue la mémoire (alloc)
  input_password.reserve(password.length());

  startTime = millis();
}

void(* resetFunc) (void) = 0; 

void loop() {
  // on prend le temps en court (c'est pour le async)
  currentMillis = millis();
  
  // On gere le clavier ici
  if (keyPad.isPressed() && deltaTime(lastKeyPress) > keyDelay) {
    char key = keyPad.getKey();
    bool isValid = false;
    for (int i = 0; i < 12; i++) {
      if (valid[i] == key) {
        isValid = true;
      }
    }
    Serial.println("entry : '"+String(key)+"'");
    if (isValid) {
      lastKeyPress = millis();
      // On commence a aller plus vite aussi
      if (!startedBeep) {
        startedBeep = true;
        startTime = millis();
        currentMillis = millis();
        timingUpBomb = beep_time();
      }
      // By Oyzed (the condition)
      if (input_password.length() == password.length()) {
        input_password.remove(0, 1);
      }
      input_password += key;
      if (password == input_password) {
        Serial.println("correct");
        valide = true;
        digitalWrite(ledRouge, LOW);
        digitalWrite(ledVerte, HIGH);

        int pos = 8-(input_password.length())/2;
        lcd.setCursor(pos, 1);
        lcd.print(input_password);
        delay(60000);
        resetFunc();
      } else if (input_password == "*******") {
        resetFunc();
      } else {
        Serial.println("incorrect");
        valide = false;
      }
      Serial.println(input_password);
      // pos = tailleécran/2 - mdp/2
      int pos = 8-(input_password.length())/2;
      lcd.setCursor(pos, 1);
      lcd.print(input_password);
    }
  }

  led_blink();
  beep_beep();
}

void led_blink() {
  // si c'est valide c'est vert, sinon on blink 1s
  if (!valide) {
    digitalWrite(ledVerte, LOW);
    if (currentMillis - prevMillisLed > 1000 && currentMillis - prevMillisLed < 2000 ) {
      digitalWrite(ledRouge, HIGH);
    } else if (currentMillis - prevMillisLed > 2000) {
      digitalWrite(ledRouge, LOW);
      prevMillisLed = currentMillis;
    }
  } else {
    digitalWrite(ledRouge, LOW);
    digitalWrite(ledVerte, HIGH);
  }
}

double beep_time() {
  unsigned long t = (currentMillis - startTime) / 1000;
  // formule de csgo (a peu pres 45s)
  double bps = 1.05 * pow(2.71, 0.0052 * t + 0.000871 * t * t) - 0.2;
  Serial.println("Bps : " + String(bps));
  Serial.println("------------");
  return 1000 / bps;
}

void beep_beep() {
  if (!valide) {
    // beeps de 125 ms
    if (currentMillis - prevMillisBeep > 100 && currentMillis - prevMillisBeep < 125 ) {
      // magie : 12 c'est le pin, 3500 c'est la freq (proche de résonnance mais pas trop sinon c'est trop fort), 125 la durée en ms
      ledcWrite(0, 0);
    } else if (currentMillis - prevMillisBeep > timingUpBomb) {
      // arrête le beep
      ledcWrite(0, 2048);  // rapport cyclique 50%
      prevMillisBeep = currentMillis;
      timingUpBomb = beep_time();
    }

    if (timingUpBomb < 125) {
      ledcWrite(0, 0);
      if (startTime != 0) {
        delay(60000); // 60s
      }
      resetFunc();
    }

    if (!startedBeep) {
      startTime = millis();
      startedBeep = false;
    }
  }
}
