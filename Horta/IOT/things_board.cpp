#include "things_board.h"
// no main.ino


// No setup(), 
// setupThingsBoard()

// E no  loop():
// maintainThingsBoard()
// sendTelemetry(temperatura, umidadeAR, umidadeSolo, valorChuva);
// void sendTestData();

const char* ssid = "ssid";
const char* password = "senha";

// 
const char* thingsboardServer = "demo.thingsboard.io";
const char* accessToken = "seuToken";

WiFiClient espClient;
PubSubClient client(espClient);

bool relayState = false;

// ======= CALLBACK RPC (botao relay) =======
void callback(char* topic, byte* payload, unsigned int length) {
  String msg;
  for (int i = 0; i < length; i++) msg += (char)payload[i];

  Serial.println("Recebido comando RPC:");
  Serial.println("Payload: " + msg);

  String topicStr = String(topic);
  String requestId = topicStr.substring(topicStr.lastIndexOf("/") + 1);

  if (msg.indexOf("getState") != -1) {
    String responseTopic = "v1/devices/me/rpc/response/" + requestId;
    String responsePayload = relayState ? "true" : "false";
    client.publish(responseTopic.c_str(), responsePayload.c_str());
    return;
  }

  if (msg.indexOf("setState") != -1) {
    relayState = msg.indexOf("true") != -1;
    digitalWrite(RELAY_PIN, relayState ? HIGH : LOW);
    Serial.println("Relé agora está: " + String(relayState ? "LIGADO" : "DESLIGADO"));
  }
}

// ======= conexoes =======
void connectWiFi() {
  WiFi.begin(ssid, password);
  Serial.print("Conectando ao Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("\nWi-Fi conectado!");
}

void connectThingsBoard() {
  while (!client.connected()) {
    Serial.print("Conectando ao ThingsBoard...");
    if (client.connect("ESP32Client", accessToken, NULL)) {
      Serial.println("Conectado!");
      client.subscribe("v1/devices/me/rpc/request/+");
    } else {
      Serial.print("Falhou. Código: ");
      Serial.println(client.state());
      delay(3000);
    }
  }
}

// ======= inicializacao =======
void setupThingsBoard() {
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);
  connectWiFi();
  client.setServer(thingsboardServer, 1883);
  client.setCallback(callback);
}

// ======= loop principal (MQTT) =======
void maintainThingsBoard() {
  if (!client.connected()) {
    connectThingsBoard();
  }
  client.loop();
}

// ======= envia dados para a plataforma =======
void sendTelemetry(float temp, float hum, float s_moist, float rain) {
  String payload = "{";
  payload += "\"temperature\":" + String(temp, 1) + ",";
  payload += "\"humidity\":" + String(hum, 1) + ",";
  payload += "\"relay\":" + String(relayState ? "true" : "false") + ",";
  payload += "\"rainStatus\":" + String(rain, 1) + ",";
  payload += "\"soilMoisture\":" + String(s_moist, 1);
  payload += "}";
  client.publish("v1/devices/me/telemetry", payload.c_str());
}

// ======= envia valores aleatorios para testar =======
void sendTestData() {
  float temp = random(1500, 3500) / 100.0;
  float hum = random(3000, 10000) / 100.0;
  float mo = random(1500, 3500) / 100.0;
  float ra = random(2, 0) / 100.0;

  Serial.print("Temp fake: ");
  Serial.print(temp);
  Serial.print(" °C | Umid fake: ");
  Serial.print(hum);
  Serial.print(" °C | rain fake: ");
  Serial.print(ra);
  Serial.print(" °C | moist fake: ");
  Serial.print(mo);
  Serial.println(" %");

  sendTelemetry(temp, hum, mo, ra);
}