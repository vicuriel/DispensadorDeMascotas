#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>

const char* ssid     = "FIUNA";
const char* password = "fiuna#2024";

// 🔗 URLs correctas (terminan en .json)
const char* URL_FOOD  = "https://dispensadoraut-default-rtdb.firebaseio.com/commands/Food.json";
const char* URL_WATER = "https://dispensadoraut-default-rtdb.firebaseio.com/commands/Water.json";

// ---- Helper: GET de Firebase que devuelve el string sin comillas ----
String getFirebaseString(const char* url) {
  WiFiClientSecure client;
  client.setInsecure();                      // (para pruebas) ignora certificado HTTPS
  HTTPClient http;

  if (!http.begin(client, url)) return "";

  int code = http.GET();
  String payload = (code > 0) ? http.getString() : "";
  http.end();

  payload.trim();
  if (payload == "null") return "";

  // Si viene "80%" con comillas, las quitamos
  if (payload.length() >= 2 && payload[0] == '\"' && payload[payload.length()-1] == '\"') {
    payload = payload.substring(1, payload.length()-1);
  }
  return payload; // ej: 80%  (sin comillas)
}

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  Serial.print("Conectando a WiFi");
  while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }
  Serial.println("\n✅ WiFi conectado");
  Serial.print("IP: "); Serial.println(WiFi.localIP());
}

void loop() {
  if (WiFi.status() == WL_CONNECTED) {
    String food  = getFirebaseString(URL_FOOD);
    String water = getFirebaseString(URL_WATER);

    Serial.print("🔥 Food:  "); Serial.println(food);   // ej: 80%
    Serial.print("💧 Water: "); Serial.println(water);  // ej: 60%

    // (Opcional) Convertir a número quitando el %
    // String f = food;  f.replace("%","");  int foodNum  = f.toInt();
    // String w = water; w.replace("%","");  int waterNum = w.toInt();
  } else {
    Serial.println("⚠️ WiFi desconectado");
  }

  delay(2000);  // lee cada 2 s
}
