/*
    Sistema de Irrigação Inteligente com ESP32, KNN e ThingsBoard
    Features: Temperatura, Umidade do Ar, Umidade do Solo + IoT
    
    Sensores:
    - DHT11: Temperatura e Umidade do ar
    - FC-28: Umidade do solo
    - FC-37: Sensor precipitação
    - BMP280: Sensor de pressão atmosférica
    - Sensores de nível: Controle automático do tanque
    
    IoT: ThingsBoard para monitoramento e controle remoto
*/

#include <DHT.h>
#include <Wire.h>
#include <Adafruit_BMP280.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "model_data.h"  // Header com os dados do modelo KNN

// ======= CONFIGURAÇÃO WiFi e ThingsBoard =======
const char* ssid = "REDE_WIFI";
const char* password = "SENHA_WIFI";
const char* thingsboardServer = "demo.thingsboard.io";
const char* accessToken = "TOKEN_THINGSBOARD";

// ======= DEFINIÇÕES DE PINOS CORRIGIDAS =======
#define DHTTYPE DHT11                // Tipo do sensor DHT
#define DHTPIN 4                    // GPIO 4 (Digital) - DHT11
#define SOIL_MOISTURE_PIN 35        // GPIO 35 (ADC1_CH7) - FC-28 
#define RAIN_ANALOG_PIN 34          // GPIO 34 (ADC1_CH6) - FC-37
#define LEVEL_SENSOR1_PIN 14        // GPIO 14 (Digital) - Nível baixo 
#define LEVEL_SENSOR2_PIN 27        // GPIO 27 (Digital) - Nível alto 
#define PUMP_PIN 12                 // GPIO 12 (Output) - Bomba
#define SOLENOIDE_PIN 13            // GPIO 13 (Output) - Válvula
#define BMP_SDA 21                  // GPIO 21 (I2C SDA) - BMP280
#define BMP_SCL 22                  // GPIO 22 (I2C SCL) - BMP280

// ======= INICIALIZAÇÃO DOS SENSORES =======
DHT dht(DHTPIN, DHTTYPE);
Adafruit_BMP280 bmp;
WiFiClient espClient;
PubSubClient client(espClient);

// ======= PARÂMETROS DO MODELO KNN =======
#define N_FEATURES 3
#define N_TRAIN_REDUCED 100
#define N_NEIGHBORS 3

// ======= ESTADOS DO SISTEMA =======
enum WaterSystemState {
    TANK_OK,           // Tanque com água suficiente
    TANK_LOW,          // Nível baixo - precisa reabastecer
    TANK_EMPTY,        // Tanque vazio - parar irrigação
    TANK_FILLING,      // Abastecendo o tanque
    TANK_FULL          // Tanque cheio
};

enum IrrigationMode {
    MODE_AUTO,         // Modo automático (IA)
    MODE_MANUAL,       // Comando manual do ThingsBoard
    MODE_HUMIDITY      // Baseado na umidade mínima definida
};

// ======= VARIÁVEIS GLOBAIS =======
WaterSystemState tankState = TANK_OK;
IrrigationMode currentMode = MODE_AUTO;
unsigned long lastTankCheck = 0;
unsigned long lastTelemetry = 0;
unsigned long tankFillStartTime = 0;
unsigned long lastIrrigationCheck = 0;
bool irrigationBlocked = false;
bool bmpAvailable = false;
bool manualIrrigation = false;
float minSoilHumidity = 30.0;  // Umidade mínima padrão (30%)

// ======= CONSTANTES DE TEMPO =======
const unsigned long TANK_CHECK_INTERVAL = 10000; // 10 segundos
const unsigned long TELEMETRY_INTERVAL = 5000;  // 5 segundos
const unsigned long IRRIGATION_CHECK_INTERVAL = 30000; // 30 segundos
const unsigned long MAX_FILL_TIME = 300000;      // 5 minutos

// ======= ESTRUTURA DOS DADOS DOS SENSORES =======
struct SensorData {
    float temperatura;
    float umidadeAr;
    float umidadeSolo;
    float pressao;
    float altitude;
    int chuvaAnalogica;
    bool nivelBaixo;
    bool nivelAlto;
    bool bmpOk;
    bool irrigando;
    String tankStatus;
    String weatherCondition;
};

// ======= CALLBACK RPC DO THINGSBOARD =======
void callback(char* topic, byte* payload, unsigned int length) {
    // Converter payload para string
    String msg = "";
    for (int i = 0; i < length; i++) {
        msg += (char)payload[i];
    }
    
    Serial.println("Comando RPC recebido: " + msg);
    
    // Extrair ID da requisição
    String topicStr = String(topic);
    String requestId = topicStr.substring(topicStr.lastIndexOf("/") + 1);
    String responseTopic = "v1/devices/me/rpc/response/" + requestId;
    
    // Parse JSON
    DynamicJsonDocument doc(1024);
    deserializeJson(doc, msg);
    
    String method = doc["method"];
    String response = "{}";
    
    // Comandos disponíveis
    if (method == "getSystemStatus") {
        // Retorna status completo do sistema
        response = "{\"tankState\":\"" + getTankStateText() + "\",";
        response += "\"irrigating\":" + String(digitalRead(PUMP_PIN) ? "true" : "false") + ",";
        response += "\"mode\":\"" + getModeText() + "\",";
        response += "\"minHumidity\":" + String(minSoilHumidity) + "}";
        
    } else if (method == "setManualIrrigation") {
        // Comando manual de irrigação
        bool enable = doc["params"]["enable"];
        manualIrrigation = enable;
        currentMode = enable ? MODE_MANUAL : MODE_AUTO;
        
        Serial.println("Modo manual: " + String(enable ? "ATIVADO" : "DESATIVADO"));
        response = "{\"success\":true,\"manualMode\":" + String(enable ? "true" : "false") + "}";
        
    } else if (method == "setMinHumidity") {
        // Define umidade mínima
        float newMinHumidity = doc["params"]["humidity"];
        if (newMinHumidity >= 0 && newMinHumidity <= 100) {
            minSoilHumidity = newMinHumidity;
            currentMode = MODE_HUMIDITY;
            Serial.println("Nova umidade mínima: " + String(minSoilHumidity) + "%");
            response = "{\"success\":true,\"minHumidity\":" + String(minSoilHumidity) + "}";
        } else {
            response = "{\"success\":false,\"error\":\"Invalid humidity range\"}";
        }
        
    } else if (method == "setAutoMode") {
        // Volta para modo automático (IA)
        currentMode = MODE_AUTO;
        manualIrrigation = false;
        Serial.println("Modo automático (IA) ativado");
        response = "{\"success\":true,\"mode\":\"auto\"}";
        
    } else if (method == "emergencyStop") {
        // Parada de emergência
        digitalWrite(PUMP_PIN, LOW);
        digitalWrite(SOLENOIDE_PIN, LOW);
        manualIrrigation = false;
        Serial.println("PARADA DE EMERGÊNCIA ATIVADA");
        response = "{\"success\":true,\"stopped\":true}";
    }
    
    // Enviar resposta
    client.publish(responseTopic.c_str(), response.c_str());
}

// ======= FUNÇÕES AUXILIARES =======
String getTankStateText() {
    switch (tankState) {
        case TANK_OK: return "OK";
        case TANK_LOW: return "BAIXO";
        case TANK_EMPTY: return "VAZIO";
        case TANK_FILLING: return "ENCHENDO";
        case TANK_FULL: return "CHEIO";
        default: return "DESCONHECIDO";
    }
}

String getModeText() {
    switch (currentMode) {
        case MODE_AUTO: return "AUTO";
        case MODE_MANUAL: return "MANUAL";
        case MODE_HUMIDITY: return "HUMIDITY";
        default: return "UNKNOWN";
    }
}

bool isTimeElapsed(unsigned long lastTime, unsigned long interval) {
    return (millis() - lastTime >= interval);
}

// ======= CONEXÕES =======
void connectWiFi() {
    WiFi.begin(ssid, password);
    Serial.print("Conectando ao Wi-Fi");
    unsigned long startAttemptTime = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 30000) { // Timeout de 30 segundos
        delay(1000);
        Serial.print(".");
    }
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\nWi-Fi conectado!");
        Serial.println("IP: " + WiFi.localIP().toString());
    } else {
        Serial.println("\nFalha ao conectar ao Wi-Fi. Reiniciando...");
        ESP.restart();
    }
}

void connectThingsBoard() {
    unsigned int retryCount = 0;
    while (!client.connected() && retryCount < 5) { // Máximo de 5 tentativas
        Serial.print("Conectando ao ThingsBoard...");
        if (client.connect("ESP32_IrrigationSystem", accessToken, NULL)) {
            Serial.println("Conectado!");
            client.subscribe("v1/devices/me/rpc/request/+");
            Serial.println("Subscrito aos comandos RPC");
            return;
        } else {
            Serial.print(" Falhou, código: ");
            Serial.println(client.state());
            retryCount++;
            delay(3000);
        }
    }
    if (!client.connected()) {
        Serial.println("Falha ao conectar ao ThingsBoard. Reiniciando...");
        ESP.restart();
    }
}

// ======= FUNÇÕES DO MODELO KNN =======
void standardize(float *input, int n_features) {
    for (int i = 0; i < n_features; i++) {
        input[i] = (input[i] - scaler_mean[i]) / scaler_scale[i];
    }
}

float euclidean_distance(const float *a, const float *b, int n_features) {
    float distance = 0.0;
    for (int i = 0; i < n_features; i++) {
        float diff = a[i] - b[i];
        distance += diff * diff;
    }
    return sqrt(distance);
}

int knn_predict(const float *input) {
    float min_distances[N_NEIGHBORS];
    int indices[N_NEIGHBORS];

    for (int i = 0; i < N_NEIGHBORS; i++) {
        min_distances[i] = INFINITY;
        indices[i] = -1;
    }

    for (int i = 0; i < N_TRAIN_REDUCED; i++) {
        float distance = euclidean_distance(input, &X_train_reduced[i * N_FEATURES], N_FEATURES);

        for (int j = 0; j < N_NEIGHBORS; j++) {
            if (distance < min_distances[j]) {
                for (int k = N_NEIGHBORS - 1; k > j; k--) {
                    min_distances[k] = min_distances[k - 1];
                    indices[k] = indices[k - 1];
                }
                min_distances[j] = distance;
                indices[j] = i;
                break;
            }
        }
    }

    int votes[2] = {0, 0};
    for (int i = 0; i < N_NEIGHBORS; i++) {
        if (indices[i] >= 0) {
            votes[y_train_reduced[indices[i]]]++;
        }
    }

    return (votes[1] > votes[0]) ? 1 : 0;
}

// ======= LEITURA DOS SENSORES =======
SensorData readAllSensors() {
    SensorData data;

    // DHT11
    data.temperatura = dht.readTemperature();
    data.umidadeAr = dht.readHumidity();

    // Validar leituras do DHT11
    if (isnan(data.temperatura) || isnan(data.umidadeAr)) {
        Serial.println("Erro: Leitura inválida do DHT11. Usando valores padrão.");
        data.temperatura = -999;  // Valor padrão
        data.umidadeAr = -999;    // Valor padrão
    }

    // FC-28 (Umidade do Solo)
    int soilReading = analogRead(SOIL_MOISTURE_PIN);
    data.umidadeSolo = map(soilReading, 0, 4095, 100, 0);

    // Validar leituras do FC-28
    if (data.umidadeSolo < 0 || data.umidadeSolo > 100) {
        Serial.println("Erro: Leitura inválida do sensor de umidade do solo. Usando valor padrão.");
        data.umidadeSolo = 50.0;  // Valor padrão
    }

    // FC-37 (Sensor de Chuva)
    data.chuvaAnalogica = analogRead(RAIN_ANALOG_PIN);

    // BMP280
    data.bmpOk = false;
    if (bmpAvailable) {
        data.pressao = bmp.readPressure() / 100.0F;
        data.altitude = bmp.readAltitude(1013.25);
        data.bmpOk = true;

        // Validar leituras do BMP280
        if (data.pressao < 300 || data.pressao > 1100) {
            Serial.println("Erro: Leitura inválida do BMP280. Ignorando dados.");
            data.pressao = -999;  // Valor de erro
            data.altitude = -999; // Valor de erro
            data.bmpOk = false;
        }
    }

    // Sensores de nível
    data.nivelBaixo = digitalRead(LEVEL_SENSOR1_PIN);
    data.nivelAlto = digitalRead(LEVEL_SENSOR2_PIN);

    // Status da irrigação
    data.irrigando = digitalRead(PUMP_PIN);
    data.tankStatus = getTankStateText();

    return data;
}

// ======= SISTEMA DE GERENCIAMENTO DO TANQUE AUTOMÁTICO =======
WaterSystemState readTankLevel() {
    bool level1 = digitalRead(LEVEL_SENSOR1_PIN);  // Nível baixo
    bool level2 = digitalRead(LEVEL_SENSOR2_PIN);  // Nível alto
    
    if (!level1 && !level2) {
        return TANK_EMPTY;
    } else if (level1 && !level2) {
        return TANK_LOW;
    } else if (level1 && level2) {
        return TANK_FULL;
    } else {
        return TANK_EMPTY;
    }
}

void controlWaterSupply(bool turnOn) {
    digitalWrite(SOLENOIDE_PIN, turnOn ? HIGH : LOW);
    
    if (turnOn) {
        Serial.println("ABASTECIMENTO LIGADA");
        tankFillStartTime = millis();
    } else {
        Serial.println("ABASTECIMENTO DESLIGADA");
    }
}

void manageTankSystem() {
    unsigned long currentTime = millis();
    
    if (currentTime - lastTankCheck >= TANK_CHECK_INTERVAL) {
        WaterSystemState currentLevel = readTankLevel();
        lastTankCheck = currentTime;
        
        switch (tankState) {
            case TANK_OK:
            case TANK_FULL:
                if (currentLevel == TANK_LOW) {
                    Serial.println("NÍVEL BAIXO - Iniciando abastecimento automático");
                    tankState = TANK_FILLING;
                    controlWaterSupply(true);
                    irrigationBlocked = false;
                } else if (currentLevel == TANK_EMPTY) {
                    Serial.println("TANQUE VAZIO - Bloqueando irrigação");
                    tankState = TANK_EMPTY;
                    controlWaterSupply(true);
                    irrigationBlocked = true;
                } else {
                    tankState = TANK_OK;
                    irrigationBlocked = false;
                }
                break;
                
            case TANK_LOW:
                if (currentLevel == TANK_FULL) {
                    Serial.println("TANQUE CHEIO - Parando abastecimento automático");
                    tankState = TANK_FULL;
                    controlWaterSupply(false);
                    irrigationBlocked = false;
                } else if (currentLevel == TANK_EMPTY) {
                    tankState = TANK_EMPTY;
                    irrigationBlocked = true;
                }
                break;
                
            case TANK_EMPTY:
                if (currentLevel == TANK_LOW || currentLevel == TANK_FULL) {
                    tankState = (currentLevel == TANK_FULL) ? TANK_FULL : TANK_LOW;
                    if (currentLevel == TANK_FULL) {
                        controlWaterSupply(false);
                    }
                    irrigationBlocked = false;
                }
                break;
                
            case TANK_FILLING:
                if (currentLevel == TANK_FULL) {
                    Serial.println("ABASTECIMENTO AUTOMÁTICO CONCLUÍDO - Sensor 2 atingido");
                    tankState = TANK_FULL;
                    controlWaterSupply(false);  // DESLIGA AUTOMATICAMENTE
                    irrigationBlocked = false;
                } else if (currentTime - tankFillStartTime > MAX_FILL_TIME) {
                    Serial.println("TIMEOUT - Sistema de abastecimento");
                    controlWaterSupply(false);
                    tankState = TANK_LOW;
                }
                break;
        }
    }
}

// ======= CONTROLE DE IRRIGAÇÃO COM PRIORIDADES =======
void controlPump(bool shouldIrrigate) {
    // Verificar se irrigação está bloqueada por falta de água
    if (shouldIrrigate && irrigationBlocked) {
        Serial.println("IRRIGAÇÃO BLOQUEADA - Tanque vazio");
        digitalWrite(PUMP_PIN, LOW);
        digitalWrite(SOLENOIDE_PIN, LOW);
        return;
    }
    
    digitalWrite(PUMP_PIN, shouldIrrigate ? HIGH : LOW);
    
    if (shouldIrrigate) {
        Serial.println("IRRIGAÇÃO ATIVA - Sistema ligado");
    } else {
        Serial.println("IRRIGAÇÃO DESATIVADA - Sistema desligado");
    }
}

// ======= LÓGICA DE DECISÃO COM PRIORIDADES =======
bool shouldIrrigate(const SensorData& data) {
    // PRIORIDADE 1: Comando manual do ThingsBoard
    if (currentMode == MODE_MANUAL) {
        Serial.println("MODO MANUAL ATIVO - Comando ThingsBoard");
        return manualIrrigation;
    }
    
    // PRIORIDADE 2: Umidade mínima definida pelo usuário
    if (currentMode == MODE_HUMIDITY) {
        bool needsWater = data.umidadeSolo < minSoilHumidity;
        if (needsWater) {
            Serial.println("UMIDADE BAIXA - Irrigação necessária (" + 
                          String(data.umidadeSolo) + "% < " + String(minSoilHumidity) + "%)");
        }
        return needsWater;
    }
    
    // PRIORIDADE 3: Decisão da IA (modo automático)
    float input[N_FEATURES] = {data.temperatura, data.umidadeAr, data.umidadeSolo};
    float input_scaled[N_FEATURES];
    for (int i = 0; i < N_FEATURES; i++) {
        input_scaled[i] = input[i];
    }
    standardize(input_scaled, N_FEATURES);
    
    int prediction = knn_predict(input_scaled);
    if (prediction == 1) {
        Serial.println("IA DECIDIU - Irrigação recomendada");
    }
    return prediction == 1;
}

// ======= ENVIO DE TELEMETRIA =======
void sendTelemetry(const SensorData& data, bool irrigationDecision) {
    DynamicJsonDocument doc(1024);

    doc["temperature"] = data.temperatura;
    doc["humidity"] = data.umidadeAr;
    doc["soilMoisture"] = data.umidadeSolo;
    doc["rainIntensity"] = data.chuvaAnalogica;
    doc["irrigating"] = data.irrigando;
    doc["tankState"] = data.tankStatus;
    doc["irrigationBlocked"] = irrigationBlocked;
    doc["currentMode"] = getModeText();
    doc["minSoilHumidity"] = minSoilHumidity;
    doc["aiDecision"] = irrigationDecision;

    if (data.bmpOk) {
        doc["pressure"] = data.pressao;
        doc["altitude"] = data.altitude;
        doc["weather"] = data.weatherCondition;
    }

    String payload;
    serializeJson(doc, payload);
    client.publish("v1/devices/me/telemetry", payload.c_str());
    Serial.println("Telemetria enviada ao ThingsBoard");
}

// ======= SETUP =======
void setup() {
    Serial.begin(115200);
    Serial.println("SISTEMA DE IRRIGAÇÃO INTELIGENTE v2.0");
    Serial.println("Com ThingsBoard e Controle Automático de Tanque");
    Serial.println("=======================================");
    
    // Conectar Wi-Fi e ThingsBoard
    connectWiFi();
    client.setServer(thingsboardServer, 1883);
    client.setCallback(callback);
    
    // Inicializar I2C e BMP280
    Wire.begin(BMP_SDA, BMP_SCL);
    if (!bmp.begin(0x76) && !bmp.begin(0x77)) {
        Serial.println("BMP280 não encontrado");
        bmpAvailable = false;
    } else {
        Serial.println("BMP280 inicializado");
        bmpAvailable = true;
        bmp.setSampling(Adafruit_BMP280::MODE_NORMAL,
                       Adafruit_BMP280::SAMPLING_X2,
                       Adafruit_BMP280::SAMPLING_X16,
                       Adafruit_BMP280::FILTER_X16,
                       Adafruit_BMP280::STANDBY_MS_500);
    }
    
    // Configurar pinos
    pinMode(SOIL_MOISTURE_PIN, INPUT);
    pinMode(RAIN_ANALOG_PIN, INPUT);
    pinMode(LEVEL_SENSOR1_PIN, INPUT);
    pinMode(LEVEL_SENSOR2_PIN, INPUT);
    
    pinMode(PUMP_PIN, OUTPUT);
    pinMode(SOLENOIDE_PIN, OUTPUT);
    
    // Estado inicial
    digitalWrite(PUMP_PIN, LOW);
    digitalWrite(SOLENOIDE_PIN, LOW);
    
    // Inicializar DHT
    dht.begin();
    
    // Estado inicial do tanque
    tankState = readTankLevel();
    
    Serial.println("Sistema inicializado com sucesso!");
    Serial.println("Comandos disponíveis via ThingsBoard:");
    Serial.println("   - setManualIrrigation: Controle manual");
    Serial.println("   - setMinHumidity: Define umidadade mínima");
    Serial.println("   - setAutoMode: Volta para modo IA");
    Serial.println("   - getSystemStatus: Status do sistema");
    Serial.println("   - emergencyStop: Parada de emergência");
    Serial.println("=======================================");
    
    delay(2000);
}

// ======= LOOP PRINCIPAL =======
void loop() {
    // Manter conexão ThingsBoard
    if (!client.connected()) {
        connectThingsBoard();
    }
    client.loop();

    unsigned long currentTime = millis();

    // Ler sensores uma vez por ciclo
    SensorData sensorData = readAllSensors();

    // Gerenciar sistema de tanque
    if (isTimeElapsed(lastTankCheck, TANK_CHECK_INTERVAL)) {
        manageTankSystem();
        lastTankCheck = currentTime;
    }

    // Verificar irrigação
    bool irrigationDecision = false; // Variável para armazenar a decisão
    if (isTimeElapsed(lastIrrigationCheck, IRRIGATION_CHECK_INTERVAL)) {
        if (sensorData.temperatura == -999 || sensorData.umidadeAr == -999) {
            Serial.println("Erro DHT11");
            delay(5000);
            return;
        }

        irrigationDecision = shouldIrrigate(sensorData);
        controlPump(irrigationDecision);
        lastIrrigationCheck = currentTime;
    }

    // Enviar telemetria
    if (isTimeElapsed(lastTelemetry, TELEMETRY_INTERVAL)) {
        sendTelemetry(sensorData, irrigationDecision);
        lastTelemetry = currentTime;
    }

    delay(100);  // Pequeno atraso para evitar sobrecarga
}

