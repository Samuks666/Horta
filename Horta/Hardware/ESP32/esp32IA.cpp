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

// ======= CONFIGURAÇÃO WIFI / THINGSBOARD =======
const char* ssid = "WIFI_NAME";
const char* password = "WIFI_PASSWORD";
const char* thingsboardServer = "demo.thingsboard.io";
const char* accessToken = "TOKEN";

// ======= DEFINIÇÕES DE PINOS  =======
#define DHTTYPE DHT11                // Tipo do sensor DHT
#define DHTPIN 4                    // GPIO 4 (Digital) - DHT11
#define SOIL_MOISTURE_PIN 35        // GPIO 35 (ADC1_CH7) - FC-28 
#define RAIN_ANALOG_PIN 34          // GPIO 34 (ADC1_CH6) - FC-37
#define LEVEL_SENSOR1_PIN 14        // GPIO 14 (Digital) - Nível baixo 
#define LEVEL_SENSOR2_PIN 27        // GPIO 27 (Digital) - Nível alto 
#define PUMP_PIN 26                 // GPIO 26 (Output) - Bomba
#define SOLENOIDE_PIN 25            // GPIO 25 (Output) - Válvula
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
    MODE_AUTO,         // Modo automático (IA + Umidade mínima)
    MODE_MANUAL        // Comando manual do ThingsBoard
};

// ======= VARIÁVEIS GLOBAIS =======
WaterSystemState tankState = TANK_OK;
IrrigationMode currentMode = MODE_AUTO;
unsigned long lastTankCheck = 0;
unsigned long lastTelemetry = 0;
unsigned long tankFillStartTime = 0;
unsigned long lastIrrigationCheck = 0;
unsigned long lastSensorRead = 0;
bool irrigationBlocked = false;
bool bmpAvailable = false;
bool manualIrrigation = false;
float minSoilHumidity = 30.0;
unsigned long irrigationStartTime = 0;
bool irrigationActive = false;
bool thingsboardConnected = false;
unsigned long lastConnectionAttempt = 0;
const unsigned long CONNECTION_RETRY_INTERVAL = 60000; // Tentar reconectar a cada 1 minuto

// ======= CONSTANTES DE TEMPO  =======
const unsigned long SENSOR_READ_INTERVAL = 2000;     // 2 segundos - Debug
const unsigned long TELEMETRY_INTERVAL = 5000;       // 5 segundos - Telemetria
const unsigned long TANK_CHECK_INTERVAL = 10000;     // 10 segundos - Tanque
const unsigned long IRRIGATION_CHECK_INTERVAL = 30000; // 30 segundos - Irrigação
const unsigned long MAX_FILL_TIME = 120000;           // 2 minutos - Timeout tanque
const unsigned long SENSOR_TIMEOUT = 5000;            // 5 segundos - Timeout sensores
const unsigned long MAX_IRRIGATION_TIME = 60000;      // 1 minuto máximo
const unsigned long MIN_IRRIGATION_TIME = 10000;      // 10 segundos mínimo
const float HUMIDITY_TOLERANCE = 2.0;                 // Tolerância de 2% para parar irrigação

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
        
        // Controlar bomba imediatamente no modo manual
        if (currentMode == MODE_MANUAL) {
            controlSmartPump(manualIrrigation);
        } else {
            controlSmartPump(false); // Parar se sair do modo manual
        }
        
        Serial.println("Modo manual: " + String(enable ? "ATIVADO" : "DESATIVADO"));
        response = "{\"success\":true,\"manualMode\":" + String(enable ? "true" : "false") + "}";
        
    } else if (method == "setMinHumidity") {
        // Define umidade mínima (integrada no modo automático)
        float newMinHumidity = doc["params"]["humidity"];
        if (newMinHumidity >= 0 && newMinHumidity <= 100) {
            minSoilHumidity = newMinHumidity;
            // Não muda o modo - continua no automático
            Serial.println("Nova umidade mínima: " + String(minSoilHumidity) + "% (Integrada no modo AUTO)");
            response = "{\"success\":true,\"minHumidity\":" + String(minSoilHumidity) + "}";
        } else {
            response = "{\"success\":false,\"error\":\"Invalid humidity range\"}";
        }
        
    } else if (method == "setAutoMode") {
        // Volta para modo automático (IA + Umidade)
        currentMode = MODE_AUTO;
        manualIrrigation = false;
        Serial.println("Modo automático (IA + Umidade) ativado");
        response = "{\"success\":true,\"mode\":\"auto\"}";
        
    } else if (method == "emergencyStop") {
        // Parada de emergência
        controlSmartPump(false);
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
        default: return "UNKNOWN";
    }
}

bool isTimeElapsed(unsigned long &lastTime, unsigned long interval) {
    unsigned long currentTime = millis();
    
    // Proteção contra overflow do millis()
    if (currentTime < lastTime) {
        lastTime = currentTime;
        return false;
    }
    
    if (currentTime - lastTime >= interval) {
        lastTime = currentTime;
        return true;
    }
    
    return false;
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
        Serial.println("\nFalha ao conectar ao Wi-Fi.");
        Serial.println("⚠️ MODO OFFLINE ATIVADO - Sistema funcionará autonomamente");
    }
}

void connectThingsBoard() {
    // Só tenta conectar se Wi-Fi estiver disponível
    if (WiFi.status() != WL_CONNECTED) {
        thingsboardConnected = false;
        return;
    }

    unsigned int retryCount = 0;
    while (!client.connected() && retryCount < 3) { // Máximo de 3 tentativas (reduzido)
        Serial.print("Conectando ao ThingsBoard...");
        if (client.connect("ESP32_IrrigationSystem", accessToken, NULL)) {
            Serial.println("Conectado!");
            client.subscribe("v1/devices/me/rpc/request/+");
            Serial.println("Subscrito aos comandos RPC");
            thingsboardConnected = true;
            return;
        } else {
            Serial.print(" Falhou, código: ");
            Serial.println(client.state());
            retryCount++;
            delay(2000); // Delay reduzido
        }
    }
    
    if (!client.connected()) {
        thingsboardConnected = false;
        Serial.println("Falha ao conectar ao ThingsBoard.");
        Serial.println("⚠️ MODO OFFLINE ATIVADO - Sistema funcionará autonomamente");
    }
}

// ======= FUNÇÃO PARA TENTAR RECONECTAR PERIODICAMENTE =======
void tryReconnect() {
    unsigned long currentTime = millis();
    
    // Só tenta reconectar a cada minuto
    if (currentTime - lastConnectionAttempt >= CONNECTION_RETRY_INTERVAL) {
        lastConnectionAttempt = currentTime;
        
        Serial.println("🔄 Tentando reconectar...");
        
        // Tentar reconectar Wi-Fi se desconectado
        if (WiFi.status() != WL_CONNECTED) {
            connectWiFi();
        }
        
        // Tentar reconectar ThingsBoard se Wi-Fi estiver OK
        if (WiFi.status() == WL_CONNECTED && !client.connected()) {
            connectThingsBoard();
        }
        
        // Atualizar status de conexão
        thingsboardConnected = (WiFi.status() == WL_CONNECTED && client.connected());
        
        if (thingsboardConnected) {
            Serial.println("✅ Reconectado com sucesso!");
        } else {
            Serial.println("❌ Ainda sem conexão - Continuando em modo offline");
        }
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

void printSensorData(SensorData data) {
  // Indicar status de conexão
  String connectionStatus = thingsboardConnected ? "🌐 ONLINE" : "📡 OFFLINE";
  
  Serial.println("\n==================== DADOS DOS SENSORES ====================");
  Serial.println("Status: " + connectionStatus + " | Modo: " + getModeText());
  
  // DHT11 - Temperatura e Umidade do Ar
  Serial.print("Temperatura (DHT11): ");
  if (data.temperatura == -999) {
    Serial.println("ERRO");
  } else {
    Serial.print(data.temperatura);
    Serial.println(" °C");
  }

  Serial.print("Umidade do Ar (DHT11): ");
  if (data.umidadeAr == -999) {
    Serial.println("ERRO");
  } else {
    Serial.print(data.umidadeAr);
    Serial.println(" %");
  }

  // FC-28 - Umidade do Solo
  Serial.print("Umidade do Solo (FC-28): ");
  if (data.umidadeSolo < 0 || data.umidadeSolo > 100) {
    Serial.println("ERRO");
  } else {
    Serial.print(data.umidadeSolo);
    Serial.print(" % (Mín: ");
    Serial.print(minSoilHumidity);
    Serial.println("%)");
  }

  // FC-37 - Chuva (Analog)
  Serial.print("Chuva (FC-37 - Valor Analógico): ");
  Serial.println(data.chuvaAnalogica);

  // BMP280 - Pressão e Altitude
  if (data.bmpOk) {
    Serial.print("Pressão Atmosférica (BMP280): ");
    Serial.print(data.pressao);
    Serial.println(" hPa");

    Serial.print("Altitude Estimada (BMP280): ");
    Serial.print(data.altitude);
    Serial.println(" m");
  } else {
    Serial.println("BMP280: Leitura inválida ou não disponível.");
  }

  // Sensores de Nível
  Serial.print("Nível Baixo Detectado: ");
  Serial.println(data.nivelBaixo ? "Sim" : "Não");

  Serial.print("Nível Alto Detectado: ");
  Serial.println(data.nivelAlto ? "Sim" : "Não");

  // Status da Irrigação
  Serial.print("Bomba Ligada: ");
  Serial.println(data.irrigando ? "Sim" : "Não");
  
  if (irrigationActive) {
    unsigned long duration = (millis() - irrigationStartTime) / 1000;
    Serial.print("Tempo de Irrigação: ");
    Serial.print(duration);
    Serial.println(" segundos");
  }

  Serial.print("Estado do Tanque: ");
  Serial.println(data.tankStatus);

  Serial.println("============================================================\n");
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

// ======= CONTROLE INTELIGENTE DE IRRIGAÇÃO =======
void controlSmartPump(bool shouldStart) {
    unsigned long currentTime = millis();
    
    // Verificar se irrigação está bloqueada por falta de água
    if (shouldStart && irrigationBlocked) {
        Serial.println("IRRIGAÇÃO BLOQUEADA - Tanque vazio");
        digitalWrite(PUMP_PIN, LOW);
        irrigationActive = false;
        return;
    }
    
    // INICIAR IRRIGAÇÃO
    if (shouldStart && !irrigationActive) {
        digitalWrite(PUMP_PIN, HIGH);
        irrigationActive = true;
        irrigationStartTime = currentTime;
        Serial.println("🚿 IRRIGAÇÃO INICIADA - Monitorando umidade...");
        return;
    }
    
    // PARAR IRRIGAÇÃO (comando externo)
    if (!shouldStart && irrigationActive) {
        digitalWrite(PUMP_PIN, LOW);
        irrigationActive = false;
        Serial.println("🛑 IRRIGAÇÃO INTERROMPIDA - Comando externo");
        return;
    }
    
    // VERIFICAR SE DEVE PARAR (apenas se estiver irrigando)
    if (irrigationActive) {
        unsigned long irrigationDuration = currentTime - irrigationStartTime;
        SensorData currentData = readAllSensors(); // Ler dados atuais
        
        bool shouldStop = false;
        String stopReason = "";
        
        // CONDIÇÃO 1: Tempo máximo atingido
        if (irrigationDuration >= MAX_IRRIGATION_TIME) {
            shouldStop = true;
            stopReason = "Tempo máximo atingido (" + String(MAX_IRRIGATION_TIME/1000) + "s)";
        }
        
        // CONDIÇÃO 2: Umidade desejada atingida (após tempo mínimo)
        else if (irrigationDuration >= MIN_IRRIGATION_TIME) {
            // Verificar se umidade mínima foi atingida
            if (currentData.umidadeSolo >= (minSoilHumidity + HUMIDITY_TOLERANCE)) {
                shouldStop = true;
                stopReason = "Umidade desejada atingida (" + String(currentData.umidadeSolo) + "% >= " + String(minSoilHumidity + HUMIDITY_TOLERANCE) + "%)";
            }
            // Verificar se umidade está muito alta (segurança)
            else if (currentData.umidadeSolo >= 80.0) {
                shouldStop = true;
                stopReason = "Umidade muito alta detectada (" + String(currentData.umidadeSolo) + "%)";
            }
        }
        
        // PARAR IRRIGAÇÃO
        if (shouldStop) {
            digitalWrite(PUMP_PIN, LOW);
            irrigationActive = false;
            Serial.println("✅ IRRIGAÇÃO FINALIZADA - " + stopReason);
            Serial.println("   Duração: " + String(irrigationDuration/1000) + " segundos");
        }
    }
}

// ======= LÓGICA DE DECISÃO COM PRIORIDADES E MODO OFFLINE =======
bool shouldIrrigate(const SensorData& data) {
    // Verificar se os dados são válidos
    if (data.temperatura == -999 || data.umidadeAr == -999) {
        Serial.println("ERRO: Dados inválidos dos sensores - Irrigação bloqueada");
        return false;
    }
    
    // PRIORIDADE 1: Comando manual do ThingsBoard (apenas se conectado)
    if (currentMode == MODE_MANUAL && thingsboardConnected) {
        Serial.println("🎮 MODO MANUAL ATIVO - Comando ThingsBoard");
        return manualIrrigation;
    }
    
    // Se não conectado ao ThingsBoard, força modo automático
    if (!thingsboardConnected && currentMode == MODE_MANUAL) {
        Serial.println("📡 Sem conexão - Forçando modo AUTOMÁTICO");
        currentMode = MODE_AUTO;
    }
    
    // MODO AUTOMÁTICO: Combina IA + Umidade mínima
    
    // PRIORIDADE 2: Umidade crítica (sempre irriga se muito baixa)
    if (data.umidadeSolo < minSoilHumidity) {
        String modeText = thingsboardConnected ? "ONLINE" : "OFFLINE";
        Serial.println("🌱 UMIDADE CRÍTICA (" + modeText + ") - Irrigação prioritária (" + 
                      String(data.umidadeSolo) + "% < " + String(minSoilHumidity) + "%)");
        return true;
    }
    
    // PRIORIDADE 3: Decisão da IA (se umidade não está crítica)
    float input[N_FEATURES] = {data.temperatura, data.umidadeAr, data.umidadeSolo};
    float input_scaled[N_FEATURES];
    for (int i = 0; i < N_FEATURES; i++) {
        input_scaled[i] = input[i];
    }
    standardize(input_scaled, N_FEATURES);
    
    int prediction = knn_predict(input_scaled);
    if (prediction == 1) {
        String modeText = thingsboardConnected ? "ONLINE" : "OFFLINE";
        Serial.println("🤖 IA DECIDIU (" + modeText + ") - Irrigação recomendada (Temp:" + String(data.temperatura) + 
                      "°C, Umid.Ar:" + String(data.umidadeAr) + "%, Umid.Solo:" + String(data.umidadeSolo) + "%)");
        return true;
    }
    
    String modeText = thingsboardConnected ? "ONLINE" : "OFFLINE";
    Serial.println("✅ CONDIÇÕES OK (" + modeText + ") - Irrigação não necessária");
    return false;
}

// ======= ENVIO DE TELEMETRIA COM VERIFICAÇÃO DE CONEXÃO =======
void sendTelemetry(const SensorData& data, bool irrigationDecision) {
    // Só envia telemetria se conectado ao ThingsBoard
    if (!thingsboardConnected || !client.connected()) {
        Serial.println("📡 Telemetria não enviada - Sem conexão com ThingsBoard");
        return;
    }

    DynamicJsonDocument doc(1024);

    doc["temperature"] = data.temperatura;
    doc["humidity"] = data.umidadeAr;
    doc["soilMoisture"] = data.umidadeSolo;
    doc["rainIntensity"] = data.chuvaAnalogica;
    doc["irrigating"] = irrigationActive; // Usar estado real da irrigação
    doc["tankState"] = data.tankStatus;
    doc["irrigationBlocked"] = irrigationBlocked;
    doc["currentMode"] = getModeText();
    doc["minSoilHumidity"] = minSoilHumidity;
    doc["aiDecision"] = irrigationDecision;
    doc["offlineMode"] = false; // Indicar que está online
    
    // Adicionar informações de tempo se irrigando
    if (irrigationActive) {
        unsigned long duration = (millis() - irrigationStartTime) / 1000;
        doc["irrigationDuration"] = duration;
        doc["irrigationTimeRemaining"] = (MAX_IRRIGATION_TIME - (millis() - irrigationStartTime)) / 1000;
    }

    if (data.bmpOk) {
        doc["pressure"] = data.pressao;
        doc["altitude"] = data.altitude;
        doc["weather"] = data.weatherCondition;
    }

    String payload;
    serializeJson(doc, payload);
    
    if (client.publish("v1/devices/me/telemetry", payload.c_str())) {
        Serial.println("📡 Telemetria enviada ao ThingsBoard");
    } else {
        Serial.println("❌ Falha ao enviar telemetria");
        thingsboardConnected = false; // Marcar como desconectado
    }
}

// ======= SETUP DO SISTEMA =======
void setup() {
    Serial.begin(115200);
    Serial.println("SISTEMA DE IRRIGAÇÃO INTELIGENTE v2.0");
    Serial.println("Com ThingsBoard e Controle Automático de Tanque");
    Serial.println("=======================================");
    
    // Conectar Wi-Fi e ThingsBoard
    connectWiFi();
    client.setServer(thingsboardServer, 1883);
    client.setCallback(callback);
    connectThingsBoard();
    
    // Atualizar status inicial de conexão
    thingsboardConnected = (WiFi.status() == WL_CONNECTED && client.connected());
    
    if (thingsboardConnected) {
        Serial.println("✅ Sistema ONLINE - ThingsBoard conectado");
    } else {
        Serial.println("⚠️ Sistema OFFLINE - Funcionando autonomamente");
    }
    
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
    
    if (thingsboardConnected) {
        Serial.println("📡 MODO ONLINE ATIVO");
        Serial.println("Comandos disponíveis via ThingsBoard:");
        Serial.println("   - setManualIrrigation: Controle manual");
        Serial.println("   - setMinHumidity: Define umidade mínima (integrada no modo AUTO)");
        Serial.println("   - setAutoMode: Volta para modo IA + Umidade");
        Serial.println("   - getSystemStatus: Status do sistema");
        Serial.println("   - emergencyStop: Parada de emergência");
    } else {
        Serial.println("🔋 MODO OFFLINE ATIVO");
        Serial.println("Sistema funcionará autonomamente:");
        Serial.println("   - Modo automático (IA + Umidade mínima)");
        Serial.println("   - Umidade mínima atual: " + String(minSoilHumidity) + "%");
        Serial.println("   - Tentará reconectar automaticamente");
    }
    Serial.println("=======================================");
    
    delay(2000);
}

// ======= LOOP PRINCIPAL =======
void loop() {
    // === GERENCIAR CONEXÕES ===
    // Verificar conexão ThingsBoard apenas se conectado
    if (thingsboardConnected) {
        if (!client.connected()) {
            thingsboardConnected = false;
            Serial.println("❌ Conexão ThingsBoard perdida - Mudando para modo OFFLINE");
        } else {
            client.loop(); // Processar mensagens apenas se conectado
        }
    } else {
        // Tentar reconectar periodicamente
        tryReconnect();
    }

    unsigned long currentTime = millis();
    
    // === SEMPRE ler sensores (dados frescos) ===
    SensorData sensorData = readAllSensors();
    
    // === Imprimir dados dos sensores a cada 2 segundos ===
    if (isTimeElapsed(lastSensorRead, SENSOR_READ_INTERVAL)) {
        printSensorData(sensorData);
        lastSensorRead = currentTime;
    }

    // === CONTROLE CONTÍNUO DA IRRIGAÇÃO ===
    if (irrigationActive) {
        // Se irrigação estiver ativa, verificar continuamente se deve parar
        controlSmartPump(true); // Verifica condições de parada
    } else {
        // === Verificar se deve INICIAR irrigação a cada 30 segundos ===
        if (isTimeElapsed(lastIrrigationCheck, IRRIGATION_CHECK_INTERVAL)) {
            // Validar dados críticos antes de tomar decisão
            if (sensorData.temperatura == -999 || sensorData.umidadeAr == -999) {
                Serial.println("ERRO CRÍTICO: DHT11 com falha - Pausando irrigação");
                controlSmartPump(false); // Garantir que está desligada
                lastIrrigationCheck = currentTime;
                return;
            }

            bool shouldStart = shouldIrrigate(sensorData);
            if (shouldStart) {
                controlSmartPump(true); // Iniciar irrigação inteligente
            }
            lastIrrigationCheck = currentTime;
            
            String modeText = thingsboardConnected ? "ONLINE" : "OFFLINE";
            Serial.println("=== VERIFICAÇÃO DE IRRIGAÇÃO (" + modeText + ") EXECUTADA ===");
        }
    }

    // === Enviar telemetria a cada 5 segundos (apenas se conectado) ===
    static bool lastIrrigationDecision = irrigationActive; // Usar estado atual
    if (isTimeElapsed(lastTelemetry, TELEMETRY_INTERVAL)) {
        lastIrrigationDecision = irrigationActive;
        sendTelemetry(sensorData, lastIrrigationDecision);
        lastTelemetry = currentTime;
    }

    // === Gerenciar sistema de tanque a cada 10 segundos ===
    if (isTimeElapsed(lastTankCheck, TANK_CHECK_INTERVAL)) {
        manageTankSystem();
        lastTankCheck = currentTime;
    }

    delay(100); // Pequeno delay para estabilidade
}
