#define BLYNK_TEMPLATE_ID "TMPL39-hN8kuM"
#define BLYNK_TEMPLATE_NAME "Smart Farming"
#define BLYNK_AUTH_TOKEN "dZ6YSQhq_5VbvKghbBtITPXdAzaGgdGV"

#define BLYNK_PRINT Serial

#include <WiFi.h>
#include <BlynkSimpleEsp32.h>
#include <DHT.h>

// WiFi
char ssid[] = "abc";
char pass[] = "123456789";

// Pins
#define SOIL_PIN 34
#define WATER_PIN 27
#define DHTPIN 4
#define RELAY_PIN 5

#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

BlynkTimer timer;

// 🔧 CALIBRATION (adjust these for your sensor)
#define DRY_SOIL 3200
#define WET_SOIL 1200

// Hysteresis thresholds
int ON_THRESHOLD  = 50;  // below → ON
int OFF_THRESHOLD = 60;  // above → OFF

bool autoMode = true;    // default AUTO
bool pumpState = false;
bool lastPumpState = false;

// Convert soil value → %
int getMoisturePercent(int soilValue)
{
  int percent = map(soilValue, DRY_SOIL, WET_SOIL, 0, 100);
  return constrain(percent, 0, 100);
}

// 🔁 Auto/Manual switch
BLYNK_WRITE(V5)
{
  autoMode = (param.asInt() == 0); // 0=AUTO, 1=MANUAL
}

// 🔘 Pump button (works only in MANUAL)
BLYNK_WRITE(V4)
{
  if (!autoMode)
  {
    pumpState = param.asInt();
  }
}

void sendSensorData()
{
  int soilValue = analogRead(SOIL_PIN);
  int moisturePercent = getMoisturePercent(soilValue);

  int waterStatus = digitalRead(WATER_PIN); // ONLY for display

  float temp = dht.readTemperature();
  float hum  = dht.readHumidity();

  // Water display text
  String waterText = (waterStatus == LOW) ? "FULL" : "EMPTY";

  // Send to Blynk
  Blynk.virtualWrite(V0, moisturePercent);
  Blynk.virtualWrite(V1, waterText);
  Blynk.virtualWrite(V2, temp);
  Blynk.virtualWrite(V3, hum);

  // 🤖 AUTO MODE (based ONLY on soil moisture)
  if (autoMode)
  {
    if (moisturePercent < ON_THRESHOLD)
    {
      pumpState = true;   // ON
    }
    else if (moisturePercent > OFF_THRESHOLD)
    {
      pumpState = false;  // OFF
    }
  }

  // Apply relay (most relays are active LOW)
  digitalWrite(RELAY_PIN, pumpState ? HIGH : LOW );

  // 🔁 Sync button ONLY when state changes
  if (pumpState != lastPumpState)
  {
    Blynk.virtualWrite(V4, pumpState);
    lastPumpState = pumpState;
  }

  // Debug
  Serial.print("Mode: "); Serial.print(autoMode ? "AUTO" : "MANUAL");
  Serial.print(" | Moisture: "); Serial.print(moisturePercent);
  Serial.print("% | Water: "); Serial.print(waterText);
  Serial.print(" | Pump: "); Serial.println(pumpState);
}

void setup()
{
  Serial.begin(115200);

  pinMode(SOIL_PIN, INPUT);
  pinMode(WATER_PIN, INPUT_PULLUP); // stable digital read
  pinMode(RELAY_PIN, OUTPUT);

  digitalWrite(RELAY_PIN, HIGH); // pump OFF initially

  dht.begin();

  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);

  // Sync UI on reconnect
  Blynk.syncVirtual(V4, V5);

  timer.setInterval(1000L, sendSensorData);
}

void loop()
{
  Blynk.run();
  timer.run();
}