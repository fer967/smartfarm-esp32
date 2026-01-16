#include <WiFi.h>
#include <HTTPClient.h>
#include "DHT.h"

#define PIN_TRIG 33
#define PIN_ECHO 35
#define PIN_DHT  32
#define DHTTYPE  DHT11

const char* ssid = "CHANGE_ME";
const char* password = "CHANGE_ME";

const char* serverUrl ="https://smartfarm-cloud-vz6d.onrender.com/ingest/telemetry";

const char* apiKey = "CHANGE_ME";

DHT dht(PIN_DHT, DHTTYPE);

// Nivel mínimo en cm (ajustable)
#define WATER_LOW_CM 25.0

float readDistanceCM() {
  digitalWrite(PIN_TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(PIN_TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(PIN_TRIG, LOW);

  long duration = pulseIn(PIN_ECHO, HIGH, 30000); // timeout 30ms
  if (duration == 0) return -1;

  return duration * 0.0343 / 2;
}

void setup() {
  Serial.begin(115200);

  pinMode(PIN_TRIG, OUTPUT);
  pinMode(PIN_ECHO, INPUT);

  dht.begin();

  WiFi.begin(ssid, password);
  Serial.print("Conectando WiFi");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\n✅ WiFi conectado");
  Serial.println(WiFi.localIP());
}

void loop() {
  if (WiFi.status() == WL_CONNECTED) {

    float temp = dht.readTemperature();
    float hum  = dht.readHumidity();
    float dist = readDistanceCM();

    if (isnan(temp) || isnan(hum) || dist < 0) {
      Serial.println("❌ Error sensores");
      delay(5000);
      return;
    }

    bool waterLow = dist > WATER_LOW_CM;

    HTTPClient http;
    http.begin(serverUrl);
    http.addHeader("Content-Type", "application/json");
    http.addHeader("X-API-Key", apiKey);

    String payload = "{";
    payload += "\"device_id\":\"esp32_01\",";
    payload += "\"temperature\":" + String(temp, 1) + ",";
    payload += "\"humidity\":" + String(hum, 1) + ",";
    payload += "\"water_distance\":" + String(dist, 1) + ",";
    payload += "\"water_low\":" + String(waterLow ? "true" : "false");
    payload += "}";

    int code = http.POST(payload);

    Serial.println("📤 Payload:");
    Serial.println(payload);
    Serial.print("📥 HTTP code: ");
    Serial.println(code);

    http.end();
  }

  delay(10000);
}
