#define BLYNK_TEMPLATE_ID "TMPL3hM8OLlrd"
#define BLYNK_TEMPLATE_NAME "Smart Energy Meter"
#define BLYNK_AUTH_TOKEN "E-ciQL-WOlRQZjvR6NbCAKMjyAb7KLJK"

#include <LiquidCrystal_I2C.h>
#include <WiFi.h>
#include <BlynkSimpleEsp32.h>
#include <Wire.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);

char auth[] = "E-ciQL-WOlRQZjvR6NbCAKMjyAb7KLJK";
char ssid[] = "Redmi 10A";
char pass[] = "prachi567";

BlynkTimer timer;

float totalEnergy_kWh = 0;
unsigned long lastUpdateTime = 0;

const int ledPins[3] = {4, 18, 19};  // GPIOs for 3 LEDs
bool ledStates[3] = {false, false, false};  // State tracking

float energyThreshold = 0.000010; // kWh
bool notified = false;

void setup() {
  Serial.begin(9600);
  delay(1000);

  for (int i = 0; i < 3; i++) {
    pinMode(ledPins[i], OUTPUT);
    digitalWrite(ledPins[i], LOW);
  }

  // Only this line is needed for both WiFi + Blynk
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);

  lastUpdateTime = millis();
  timer.setInterval(1000L, updateEnergy);

  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Smart Energy");
  lcd.setCursor(0, 1);
  lcd.print("Meter Ready");
  delay(2000);
  lcd.clear();
}


void loop() {
  Blynk.run();
  timer.run();
}

// Blynk Virtual Pin handlers for 3 LEDs
BLYNK_WRITE(V3) { controlLED(0, param.asInt()); }  // LED 1
BLYNK_WRITE(V4) { controlLED(1, param.asInt()); }  // LED 2
BLYNK_WRITE(V5) { controlLED(2, param.asInt()); }  // LED 3

void controlLED(int index, int state) {
  digitalWrite(ledPins[index], state);
  ledStates[index] = (state == 1);
  Serial.print("LED ");
  Serial.print(index + 1);
  Serial.println(state ? " ON" : " OFF");
}

void updateLCD(float energy, float cost) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Units: ");
  lcd.print(energy, 6);

  lcd.setCursor(0, 1);
  lcd.print("Cost: ₹");
  lcd.print(cost, 2);
}

void updateEnergy() {
  unsigned long currentTime = millis();
  float elapsedTimeHours = (currentTime - lastUpdateTime) / 3600000.0;
  lastUpdateTime = currentTime;

  float supplyVoltage = 3.3;
  float ledVoltageDrop = 2.0;
  float resistor = 1000.0;
  float totalPower_W = 0;

  bool anyLedOn = false;
  for (int i = 0; i < 3; i++) {
    int actualState = digitalRead(ledPins[i]);
    if (ledStates[i] && actualState == HIGH) {
      anyLedOn = true;
      float current = (supplyVoltage - ledVoltageDrop) / resistor;
      totalPower_W += supplyVoltage * current;
    }
  }

  if (anyLedOn) {
    totalEnergy_kWh += totalPower_W * elapsedTimeHours;

    float totalCost = calculateSlabCost(totalEnergy_kWh);

    Blynk.virtualWrite(V1, totalEnergy_kWh);
    Blynk.virtualWrite(V2, totalCost);

    if (totalEnergy_kWh >= energyThreshold && !notified) {
      Blynk.logEvent("high_energy_use", "Energy consumption exceeded threshold!");
      notified = true;
    }

    Serial.print("Energy: ");
    Serial.print(totalEnergy_kWh, 6);
    Serial.print(" kWh | Cost: ₹");
    Serial.println(totalCost, 2);

    updateLCD(totalEnergy_kWh, totalCost);
  } else {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("All LEDs OFF");
    lcd.setCursor(0, 1);
    lcd.print("No Power Draw");
    Serial.println("No LEDs ON - No energy consumption");
  }
}

float calculateSlabCost(float units) {
  float cost = 0;
  if (units <= 100) {
    cost = units * 2.90;
  } else if (units <= 300) {
    cost = (100 * 2.90) + ((units - 100) * 6.50);
  } else if (units <= 500) {
    cost = (100 * 2.90) + (200 * 6.50) + ((units - 300) * 8.00);
  } else {
    cost = (100 * 2.90) + (200 * 6.50) + (200 * 8.00) + ((units - 500) * 9.60);
  }
  return cost;
}
