#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

// WiFi sarearen datuak hemen idatzi.
const char *WIFI_SSID = "WIFIa";
const char *WIFI_PASSWORD = "pasahitza";

// ThingsBoard zerbitzariaren datuak hemen idatzi.
const char *TB_SERVER = "eu.thingsboard.cloud";
const uint16_t TB_PORT = 1883;
const char *TB_TOKEN = "Access Token-a";

WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

unsigned long lastSendMs = 0;
const unsigned long SEND_INTERVAL_MS = 1000;

float kalkulatuZerraUhina(unsigned long unekoMs) {
  // 20 segundoko periodoan, lehen 10 segunduetan 0->100 eta hurrengo 10etan 100->0.
  const float periodoMs = 20000.0f;
  const float igoeraTarteaMs = 10000.0f;

  float t = fmodf((float)unekoMs, periodoMs);

  if (t < igoeraTarteaMs) {
    return (t / igoeraTarteaMs) * 100.0f;
  }

  return 100.0f - ((t - igoeraTarteaMs) / igoeraTarteaMs) * 100.0f;
}

void konektatuWiFira() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  Serial.print("WiFira konektatzen");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.print("WiFi konektatuta. IP helbidea: ");
  Serial.println(WiFi.localIP());
}

void konektatuThingsBoardera() {
  while (!mqttClient.connected()) {
    Serial.print("ThingsBoard-era konektatzen...");

    // ThingsBoard-en tokena erabiltzaile gisa bidaltzen da (pasahitzik gabe).
    if (mqttClient.connect("ESP8266_Client", TB_TOKEN, nullptr)) {
      Serial.println("konektatuta");
    } else {
      Serial.print("huts egin du, rc=");
      Serial.print(mqttClient.state());
      Serial.println(". 5 segundo barru berriro saiatuko naiz.");
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(9600);
  delay(100);

  konektatuWiFira();

  mqttClient.setServer(TB_SERVER, TB_PORT);
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    konektatuWiFira();
  }

  if (!mqttClient.connected()) {
    konektatuThingsBoardera();
  }

  mqttClient.loop();

  unsigned long unekoMs = millis();
  if (unekoMs - lastSendMs >= SEND_INTERVAL_MS) {
    lastSendMs = unekoMs;

    float balioa = kalkulatuZerraUhina(unekoMs);
    // Egoera TRUE bada seinalea goraka doa; FALSE bada beheraka.
    bool egoera = fmodf((float)unekoMs, 20000.0f) < 10000.0f;

    char payload[96];
    snprintf(payload, sizeof(payload),
             "{\"zerra_uhina\":%.2f,\"egoera\":%s}",
             balioa,
             egoera ? "true" : "false");

    mqttClient.publish("v1/devices/me/telemetry", payload);

    Serial.print("Bidalia -> ");
    Serial.print(payload);
    Serial.print(" | Egoera: ");
    Serial.println(egoera ? "TRUE (GORAKORRA)" : "FALSE (BEHERAKORRA)");
  }
}
