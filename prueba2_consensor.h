// ==== ESP32 + Firebase + VL53L0X (1 sensor) ====
// Librerías
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <Wire.h>
#include <VL53L0X.h>   // Instalar: "VL53L0X by Pololu"

// --- WiFi ---
const char* SSID = "FIUNA";
const char* PASS = "fiuna#2024";

// --- URLs Firebase (terminan en .json) ---
const char* URL_FOOD     = "https://dispensadoraut-default-rtdb.firebaseio.com/commands/Food.json";
const char* URL_WATER    = "https://dispensadoraut-default-rtdb.firebaseio.com/commands/Water.json";
const char* URL_SENSOR1  = "https://dispensadoraut-default-rtdb.firebaseio.com/sensors/sensor1.json";

// --- VL53L0X ---
VL53L0X tof;
// Ajustá estos límites según tu montaje:
const int NEAR_MM = 30;    // depósito LLENO (sensor ve cerca)
const int FAR_MM  = 200;   // depósito VACÍO  (sensor ve lejos)

// ==== Helpers HTTP ====
String getFirebaseString(const char* url) {
  WiFiClientSecure c; c.setInsecure();
  HTTPClient http;
  if (!http.begin(c, url)) return "";
  int code = http.GET();
  String payload = (code > 0) ? http.getString() : "";
  http.end();

  payload.trim();
  if (payload == "null") return "";
  if (payload.length() >= 2 && payload[0]=='\"' && payload[payload.length()-1]=='\"')
    payload = payload.substring(1, payload.length()-1);
  return payload; // ej: 80%
}

int putNumber(const char* url, int value) {
  WiFiClientSecure c; c.setInsecure();
  HTTPClient http; http.begin(c, url);
  http.addHeader("Content-Type", "application/json");
  int code = http.PUT(String(value));   // número puro (sin comillas)
  http.end();
  return code;
}

// ==== Tiempos ====
unsigned long tCmd = 0;   // comandos (Food/Water)
unsigned long tS1  = 0;   // sensor1

void setup() {
  Serial.begin(115200);

  // I2C ESP32 por defecto: SDA=21, SCL=22
  Wire.begin(21, 22);

  // Iniciar VL53L0X
  if (!tof.init()) {
    Serial.println("❌ VL53L0X no detectado. Revisa SDA/SCL/VIN/GND.");
    while (1) delay(10);
  }
  tof.setTimeout(500);
  Serial.println("✅ VL53L0X OK");

  // WiFi
  WiFi.begin(SSID, PASS);
  Serial.print("Conectando WiFi");
  while (WiFi.status()!=WL_CONNECTED){ delay(300); Serial.print("."); }
  Serial.println("\n✅ WiFi conectado");
}

void loop() {
  unsigned long now = millis();

  // 1) Leer comandos cada 2 s
  if (now - tCmd >= 2000) {
    tCmd = now;
    String food  = getFirebaseString(URL_FOOD);
    String water = getFirebaseString(URL_WATER);
    Serial.print("🔥 Food:  ");  Serial.println(food);   // ej: 30%
    Serial.print("💧 Water: ");  Serial.println(water);  // ej: 40%
  }

  // 2) Leer VL53L0X y subir a sensors/sensor1 cada 700 ms
  if (now - tS1 >= 700) {
    tS1 = now;

    int mm = tof.readRangeSingleMillimeters();
    if (tof.timeoutOccurred()) {
      Serial.println("⚠ VL53 timeout");
    } else {
      // Mapear mm -> 0..100 (NEAR=lleno, FAR=vacío)
      int mmClamped = constrain(mm, NEAR_MM, FAR_MM);
      float pct = 100.0f * (float)(FAR_MM - mmClamped) / (float)(FAR_MM - NEAR_MM);
      int sensor1 = (int)round(constrain(pct, 0.0f, 100.0f));

      int code = putNumber(URL_SENSOR1, sensor1);
      Serial.printf("📤 sensor1=%d%% (mm=%d) -> HTTP %d\n", sensor1, mm, code);
    }
  }
}
