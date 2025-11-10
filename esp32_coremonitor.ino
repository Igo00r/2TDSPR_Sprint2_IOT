/*
  ESP32 MQTT -> VENDA_EVENTO (Oracle via Node-RED)
  Versão: BOTÃO ÚNICO (sem hardware extra)
  Bibliotecas: PubSubClient, ArduinoJson
*/

#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

const char* WIFI_SSID     = "AndroidADFTK7";
const char* WIFI_PASSWORD = "estrela7839";

const char* BROKER_MQTT = "broker.hivemq.com";
const uint16_t BROKER_PORT = 1883;
#define TOPICO_PUBLISH "2TDS/esp32/teste"
#define DEVICE_ID      "ESP32-01"

// --- Botão (BOOT) ---
const int BUTTON_PIN = 0;
const unsigned long DEBOUNCE_MS   = 40;
const unsigned long DOUBLE_MS     = 400;
const unsigned long LONGPRESS_MS  = 1000;

bool btnLast = true;
unsigned long lastChangeMs = 0, pressStartMs = 0, lastReleaseMs = 0;
int  clickCount = 0; bool longReported = false;

WiFiClient espClient;
PubSubClient MQTT(espClient);

void conectaWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }
  Serial.println("\n[WiFi] Conectado!");
}

void conectaMQTT() {
  MQTT.setServer(BROKER_MQTT, BROKER_PORT);
  while (!MQTT.connected()) {
    String clientId = "esp32-" + String(random(0xffff), HEX);
    if (MQTT.connect(clientId.c_str())) {
      Serial.println("[MQTT] Conectado!");
    } else {
      delay(2000);
    }
  }
}

void publishVendaEvento(const char* origem, const char* servicoCodigo,
                        int clienteId, float quantidade, float valorUnit) {
  StaticJsonDocument<320> doc;
  doc["dispositivoId"] = DEVICE_ID;
  doc["origem"] = origem;
  doc["servicoCodigo"] = servicoCodigo;
  doc["clienteId"] = clienteId;
  doc["quantidade"] = quantidade;
  doc["valorUnitario"] = valorUnit;
  doc["valorTotal"] = quantidade * valorUnit;
  char payload[320];
  size_t n = serializeJson(doc, payload);
  MQTT.publish(TOPICO_PUBLISH, payload, n);
  Serial.print("[PUB] "); Serial.println(payload);
}

void setupButton() { pinMode(BUTTON_PIN, INPUT_PULLUP); }

void processButton() {
  bool level = digitalRead(BUTTON_PIN);
  unsigned long now = millis();

  if (level != btnLast && (now - lastChangeMs) > DEBOUNCE_MS) {
    btnLast = level;
    lastChangeMs = now;
    if (level == LOW) pressStartMs = now;
    else {
      unsigned long dur = now - pressStartMs;
      if (dur >= LONGPRESS_MS) publishVendaEvento("BOTAO_LONGO", "AVALIACAO", 103, 1.0, 80.00);
      else {
        clickCount++;
        lastReleaseMs = now;
      }
    }
  }

  if (clickCount > 0 && (now - lastReleaseMs) > DOUBLE_MS) {
    if (clickCount == 1) publishVendaEvento("BOTAO_CURTO", "CONSULTA_PADRAO", 101, 1.0, 120.00);
    else publishVendaEvento("BOTAO_DUPLO", "MANUTENCAO", 102, 1.0, 200.00);
    clickCount = 0;
  }
}

void setup() {
  Serial.begin(115200);
  conectaWiFi();
  conectaMQTT();
  setupButton();
  Serial.println("[INFO] Use o botão BOOT: curto=CONSULTA, duplo=MANUTENCAO, longo=AVALIACAO");
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) conectaWiFi();
  if (!MQTT.connected()) conectaMQTT();
  MQTT.loop();
  processButton();
}
