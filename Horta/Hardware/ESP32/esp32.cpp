/*
    Sistema de Irrigação para Manjericão com ESP32 e ThingsBoard
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

// ======= CONFIGURAÇÃO WiFi e ThingsBoard =======
const char* ssid = "SUA_REDE_WIFI";
const char* password = "SUA_SENHA_WIFI";
const char* thingsboardServer = "demo.thingsboard.io";
const char* accessToken = "SEU_TOKEN_THINGSBOARD";

// ======= DEFINIÇÕES DE PINOS =======
#define DHTPIN 4                    // D4 - GPIO 4 (Sensor DHT11)
#define DHTTYPE DHT11               // Tipo de sensor DHT
#define SOIL_MOISTURE_PIN 36        // A0 - GPIO 36 (ADC1_CH0 - FC-28)
#define RAIN_PRECIPITATION_PIN 2    // D2 - GPIO 2 (FC-37 Digital)
#define RAIN_ANALOG_PIN 35          // A3 - GPIO 35 (FC-37 Analógico)
#define LEVEL_SENSOR1_PIN 14       // D14 - GPIO 14 (Sensor nível baixo)
#define LEVEL_SENSOR2_PIN 27       // D27 - GPIO 27 (Sensor nível alto)
#define PUMP_PIN 12                // D12 - GPIO 12 (Bomba irrigação)
#define SOLENOIDE_PIN 13           // D13 - GPIO 13 (Válvula solenoide)
#define WATER_PUMP_PIN 32          // D32 - GPIO 32 (Bomba abastecimento)
#define BMP_SDA 21                 // D21 - GPIO 21 (I2C SDA)
#define BMP_SCL 22                 // D22 - GPIO 22 (I2C SCL)

// ======= INICIALIZAÇÃO DOS SENSORES =======
DHT dht(DHTPIN, DHTTYPE);
Adafruit_BMP280 bmp;
WiFiClient espClient;
PubSubClient client(espClient);

// ======= PARÂMETROS PARA MANJERICÃO =======
const float BASIL_MIN_SOIL_MOISTURE = 60.0;      // Manjericão precisa de solo úmido (60-70%)
const float BASIL_MAX_SOIL_MOISTURE = 85.0;      // Evitar encharcamento
const float BASIL_MIN_TEMPERATURE = 18.0;        // Temperatura mínima ideal
const float BASIL_MAX_TEMPERATURE = 30.0;        // Temperatura máxima ideal
const float BASIL_MIN_AIR_HUMIDITY = 40.0;       // Umidade do ar mínima
const float BASIL_MAX_AIR_HUMIDITY = 80.0;       // Umidade do ar máxima

// ======= ESTADOS DO SISTEMA =======
enum WaterSystemState {
    TANK_OK,           // Tanque com água suficiente
    TANK_LOW,          // Nível baixo - precisa reabastecer
    TANK_EMPTY,        // Tanque vazio - parar irrigação
    TANK_FILLING,      // Abastecendo o tanque
    TANK_FULL          // Tanque cheio
};

enum IrrigationMode {
    MODE_AUTO,         // Modo automático (regras manjericão)
    MODE_MANUAL,       // Comando manual do ThingsBoard
    MODE_CUSTOM        // Parâmetros customizados
};

// ======= VARIÁVEIS GLOBAIS =======
WaterSystemState tankState = TANK_OK;
IrrigationMode currentMode = MODE_AUTO;
unsigned long lastTankCheck = 0;
unsigned long lastTelemetry = 0;
unsigned long tankFillStartTime = 0;
bool irrigationBlocked = false;
bool bmpAvailable = false;
bool manualIrrigation = false;
float customMinSoilHumidity = BASIL_MIN_SOIL_MOISTURE;

// ======= CONSTANTES DE TEMPO =======
const unsigned long TANK_CHECK_INTERVAL = 2000;
const unsigned long TELEMETRY_INTERVAL = 30000;  // 30 segundos
const unsigned long MAX_FILL_TIME = 300000;      // 5 minutos

// ======= ESTRUTURA DOS DADOS DOS SENSORES =======
struct SensorData {
    float temperatura;
    float umidadeAr;
    float umidadeSolo;
    float pressao;
    float altitude;
    bool chuvaDigital;
    int chuvaAnalogica;
    bool nivelBaixo;
    bool nivelAlto;
    bool bmpOk;
    bool irrigando;
    String tankStatus;
    String weatherCondition;
    String plantCondition;
};

// ======= CALLBACK RPC DO THINGSBOARD =======
void callback(char* topic, byte* payload, unsigned int length) {
    String msg = "";
    for (int i = 0; i < length; i++) {
        msg += (char)payload[i];
    }
    
    Serial.println("Comando RPC recebido: " + msg);
    
    String topicStr = String(topic);
    String requestId = topicStr.substring(topicStr.lastIndexOf("/") + 1);
    String responseTopic = "v1/devices/me/rpc/response/" + requestId;
    
    DynamicJsonDocument doc(1024);
    deserializeJson(doc, msg);
    
    String method = doc["method"];
    String response = "{}";
    
    if (method == "getSystemStatus") {
        response = "{\"tankState\":\"" + getTankStateText() + "\",";
        response += "\"irrigating\":" + String(digitalRead(PUMP_PIN) ? "true" : "false") + ",";
        response += "\"mode\":\"" + getModeText() + "\",";
        response += "\"minHumidity\":" + String(customMinSoilHumidity) + ",";
        response += "\"plant\":\"Manjericao\"}";
        
    } else if (method == "setManualIrrigation") {
        bool enable = doc["params"]["enable"];
        manualIrrigation = enable;
        currentMode = enable ? MODE_MANUAL : MODE_AUTO;
        
        Serial.println("Modo manual: " + String(enable ? "ATIVADO" : "DESATIVADO"));
        response = "{\"success\":true,\"manualMode\":" + String(enable ? "true" : "false") + "}";
        
    } else if (method == "setCustomHumidity") {
        float newMinHumidity = doc["params"]["humidity"];
        if (newMinHumidity >= 30 && newMinHumidity <= 90) {
            customMinSoilHumidity = newMinHumidity;
            currentMode = MODE_CUSTOM;
            Serial.println("Nova umidade customizada: " + String(customMinSoilHumidity) + "%");
            response = "{\"success\":true,\"customHumidity\":" + String(customMinSoilHumidity) + "}";
        } else {
            response = "{\"success\":false,\"error\":\"Umidade deve estar entre 30-90%\"}";
        }
        
    } else if (method == "setBasilMode") {
        currentMode = MODE_AUTO;
        manualIrrigation = false;
        Serial.println("Modo manjericao ativado");
        response = "{\"success\":true,\"mode\":\"basil\"}";
        
    } else if (method == "emergencyStop") {
        digitalWrite(PUMP_PIN, LOW);
        digitalWrite(SOLENOIDE_PIN, LOW);
        manualIrrigation = false;
        Serial.println("PARADA DE EMERGENCIA ATIVADA");
        response = "{\"success\":true,\"stopped\":true}";
    }
    
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
        case MODE_AUTO: return "MANJERICAO";
        case MODE_MANUAL: return "MANUAL";
        case MODE_CUSTOM: return "PERSONALIZADO";
        default: return "UNKNOWN";
    }
}

// ======= ANÁLISE DAS CONDIÇÕES DA PLANTA =======
String analyzeBasilConditions(const SensorData& data) {
    // Verificar temperatura
    if (data.temperatura < BASIL_MIN_TEMPERATURE) {
        return "FRIO_DEMAIS";
    } else if (data.temperatura > BASIL_MAX_TEMPERATURE) {
        return "QUENTE_DEMAIS";
    }
    
    // Verificar umidade do ar
    if (data.umidadeAr < BASIL_MIN_AIR_HUMIDITY) {
        return "AR_SECO";
    } else if (data.umidadeAr > BASIL_MAX_AIR_HUMIDITY) {
        return "AR_UMIDO_DEMAIS";
    }
    
    // Verificar umidade do solo
    if (data.umidadeSolo < BASIL_MIN_SOIL_MOISTURE) {
        return "SOLO_SECO";
    } else if (data.umidadeSolo > BASIL_MAX_SOIL_MOISTURE) {
        return "SOLO_ENCHARCADO";
    }
    
    return "CONDICOES_IDEAIS";
}

// ======= CONEXÕES =======
void connectWiFi() {
    WiFi.begin(ssid, password);
    Serial.print("Conectando ao Wi-Fi");
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.print(".");
    }
    Serial.println("\nWi-Fi conectado!");
    Serial.println("IP: " + WiFi.localIP().toString());
}

void connectThingsBoard() {
    while (!client.connected()) {
        Serial.print("Conectando ao ThingsBoard...");
        if (client.connect("ESP32_BasilIrrigation", accessToken, NULL)) {
            Serial.println("Connected!");
            client.subscribe("v1/devices/me/rpc/request/+");
            Serial.println("Subscrito aos comandos RPC");
        } else {
            Serial.print(" Falhou, codigo: ");
            Serial.println(client.state());
            delay(3000);
        }
    }
}

// ======= LEITURA DOS SENSORES =======
SensorData readAllSensors() {
    SensorData data;
    
    // DHT11
    data.temperatura = dht.readTemperature();
    data.umidadeAr = dht.readHumidity();
    
    // FC-28 (Umidade do Solo)
    int soilReading = analogRead(SOIL_MOISTURE_PIN);
    data.umidadeSolo = map(soilReading, 0, 4095, 100, 0);
    
    // FC-37 (Sensor de Chuva)
    data.chuvaDigital = !digitalRead(RAIN_PRECIPITATION_PIN);
    data.chuvaAnalogica = analogRead(RAIN_ANALOG_PIN);
    
    // BMP280
    data.bmpOk = false;
    if (bmpAvailable) {
        data.pressao = bmp.readPressure() / 100.0F;
        data.altitude = bmp.readAltitude(1013.25);
        data.bmpOk = true;
        
        if (data.pressao < 1000) {
            data.weatherCondition = "TEMPESTADE";
        } else if (data.pressao > 1020) {
            data.weatherCondition = "ESTAVEL";
        } else {
            data.weatherCondition = "VARIAVEL";
        }
    }
    
    // Sensores de nível
    data.nivelBaixo = digitalRead(LEVEL_SENSOR1_PIN);
    data.nivelAlto = digitalRead(LEVEL_SENSOR2_PIN);
    
    // Status da irrigação
    data.irrigando = digitalRead(PUMP_PIN);
    data.tankStatus = getTankStateText();
    
    // Análise das condições da planta
    data.plantCondition = analyzeBasilConditions(data);
    
    return data;
}

// ======= SISTEMA DE GERENCIAMENTO DO TANQUE =======
WaterSystemState readTankLevel() {
    bool level1 = digitalRead(LEVEL_SENSOR1_PIN);
    bool level2 = digitalRead(LEVEL_SENSOR2_PIN);
    
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
    digitalWrite(WATER_PUMP_PIN, turnOn ? HIGH : LOW);
    
    if (turnOn) {
        Serial.println("BOMBA DE ABASTECIMENTO LIGADA");
        tankFillStartTime = millis();
    } else {
        Serial.println("BOMBA DE ABASTECIMENTO DESLIGADA");
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
                    Serial.println("NIVEL BAIXO - Iniciando abastecimento automatico");
                    tankState = TANK_FILLING;
                    controlWaterSupply(true);
                    irrigationBlocked = false;
                } else if (currentLevel == TANK_EMPTY) {
                    Serial.println("TANQUE VAZIO - Bloqueando irrigacao");
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
                    Serial.println("TANQUE CHEIO - Parando abastecimento automatico");
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
                    Serial.println("ABASTECIMENTO AUTOMATICO CONCLUIDO");
                    tankState = TANK_FULL;
                    controlWaterSupply(false);
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

// ======= CONTROLE DE IRRIGAÇÃO =======
void controlPump(bool shouldIrrigate) {
    if (shouldIrrigate && irrigationBlocked) {
        Serial.println("IRRIGACAO BLOQUEADA - Tanque vazio");
        digitalWrite(PUMP_PIN, LOW);
        digitalWrite(SOLENOIDE_PIN, LOW);
        return;
    }
    
    digitalWrite(PUMP_PIN, shouldIrrigate ? HIGH : LOW);
    digitalWrite(SOLENOIDE_PIN, shouldIrrigate ? HIGH : LOW);
    
    if (shouldIrrigate) {
        Serial.println("IRRIGACAO ATIVA - Sistema ligado");
    } else {
        Serial.println("IRRIGACAO DESATIVADA - Sistema desligado");
    }
}

// ======= LÓGICA DE DECISÃO PARA MANJERICÃO =======
bool shouldIrrigateBasil(const SensorData& data) {
    // PRIORIDADE 1: Comando manual
    if (currentMode == MODE_MANUAL) {
        Serial.println("MODO MANUAL ATIVO");
        return manualIrrigation;
    }
    
    // Não irrigar se estiver chovendo
    bool rainDetected = data.chuvaDigital || (data.chuvaAnalogica < 3000);
    if (rainDetected) {
        Serial.println("CHUVA DETECTADA - Irrigacao cancelada");
        return false;
    }
    
    // Não irrigar com pressão muito baixa (tempestade)
    if (data.bmpOk && data.pressao < 995) {
        Serial.println("PRESSAO BAIXA - Possivel tempestade");
        return false;
    }
    
    // Não irrigar se solo já estiver muito úmido (evitar encharcamento)
    if (data.umidadeSolo > BASIL_MAX_SOIL_MOISTURE) {
        Serial.println("SOLO MUITO UMIDO - Evitando encharcamento (" + String(data.umidadeSolo) + "%)");
        return false;
    }
    
    float targetHumidity = (currentMode == MODE_CUSTOM) ? customMinSoilHumidity : BASIL_MIN_SOIL_MOISTURE;
    
    // REGRA PRINCIPAL: Umidade do solo baixa
    if (data.umidadeSolo < targetHumidity) {
        Serial.println("MANJERICAO PRECISA DE AGUA - Solo seco (" + String(data.umidadeSolo) + "% < " + String(targetHumidity) + "%)");
        
        // Condições especiais para manjericão
        if (data.temperatura > BASIL_MAX_TEMPERATURE) {
            Serial.println("TEMPERATURA ALTA - Irrigacao prioritaria");
            return true;
        }
        
        if (data.umidadeAr < BASIL_MIN_AIR_HUMIDITY) {
            Serial.println("AR SECO - Irrigacao necessaria");
            return true;
        }
        
        return true;
    }
    
    return false;
}

// ======= ENVIO DE TELEMETRIA =======
void sendTelemetry(const SensorData& data, bool irrigationDecision) {
    String payload = "{";
    payload += "\"temperature\":" + String(data.temperatura, 1) + ",";
    payload += "\"humidity\":" + String(data.umidadeAr, 1) + ",";
    payload += "\"soilMoisture\":" + String(data.umidadeSolo, 1) + ",";
    payload += "\"rainDetected\":" + String(data.chuvaDigital ? "true" : "false") + ",";
    payload += "\"rainIntensity\":" + String(data.chuvaAnalogica) + ",";
    payload += "\"irrigating\":" + String(data.irrigando ? "true" : "false") + ",";
    payload += "\"tankState\":\"" + data.tankStatus + "\",";
    payload += "\"irrigationBlocked\":" + String(irrigationBlocked ? "true" : "false") + ",";
    payload += "\"currentMode\":\"" + getModeText() + "\",";
    payload += "\"plantType\":\"Manjericao\",";
    payload += "\"plantCondition\":\"" + data.plantCondition + "\",";
    payload += "\"customMinHumidity\":" + String(customMinSoilHumidity) + ",";
    payload += "\"irrigationDecision\":" + String(irrigationDecision ? "true" : "false");
    
    if (data.bmpOk) {
        payload += ",\"pressure\":" + String(data.pressao, 1);
        payload += ",\"altitude\":" + String(data.altitude, 1);
        payload += ",\"weather\":\"" + data.weatherCondition + "\"";
    }
    
    payload += "}";
    
    client.publish("v1/devices/me/telemetry", payload.c_str());
    Serial.println("Telemetria enviada ao ThingsBoard");
}

// ======= SETUP =======
void setup() {
    Serial.begin(115200);
    Serial.println("SISTEMA DE IRRIGACAO PARA MANJERICAO v1.0");
    Serial.println("Com ThingsBoard e Controle Automatico de Tanque");
    Serial.println("=======================================");
    
    connectWiFi();
    client.setServer(thingsboardServer, 1883);
    client.setCallback(callback);
    
    Wire.begin(BMP_SDA, BMP_SCL);
    if (!bmp.begin(0x76) && !bmp.begin(0x77)) {
        Serial.println("BMP280 nao encontrado");
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
    pinMode(RAIN_PRECIPITATION_PIN, INPUT);
    pinMode(RAIN_ANALOG_PIN, INPUT);
    pinMode(LEVEL_SENSOR1_PIN, INPUT);
    pinMode(LEVEL_SENSOR2_PIN, INPUT);
    
    pinMode(PUMP_PIN, OUTPUT);
    pinMode(SOLENOIDE_PIN, OUTPUT);
    pinMode(WATER_PUMP_PIN, OUTPUT);
    
    digitalWrite(PUMP_PIN, LOW);
    digitalWrite(SOLENOIDE_PIN, LOW);
    digitalWrite(WATER_PUMP_PIN, LOW);
    
    dht.begin();
    tankState = readTankLevel();
    
    Serial.println("PARAMETROS PARA MANJERICAO:");
    Serial.println("   Umidade do solo: 60-85%");
    Serial.println("   Temperatura: 18-30C");
    Serial.println("   Umidade do ar: 40-80%");
    Serial.println("=======================================");
    Serial.println("Comandos ThingsBoard:");
    Serial.println("   - setManualIrrigation: Controle manual");
    Serial.println("   - setCustomHumidity: Define umidade customizada");
    Serial.println("   - setBasilMode: Volta para modo manjericao");
    Serial.println("   - getSystemStatus: Status do sistema");
    Serial.println("   - emergencyStop: Parada de emergencia");
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
    
    // Gerenciar sistema de tanque (AUTOMÁTICO)
    manageTankSystem();
    
    // Ler sensores
    SensorData sensorData = readAllSensors();
    
    // Validar leituras DHT11
    if (isnan(sensorData.temperatura) || isnan(sensorData.umidadeAr)) {
        Serial.println("Erro DHT11");
        delay(5000);
        return;
    }
    
    // Validar ranges
    if (sensorData.temperatura < -40 || sensorData.temperatura > 80 || 
        sensorData.umidadeAr < 0 || sensorData.umidadeAr > 100) {
        Serial.println("Dados fora do range");
        delay(5000);
        return;
    }
    
    // DECISÃO DE IRRIGAÇÃO PARA MANJERICÃO
    bool irrigationDecision = shouldIrrigateBasil(sensorData);
    
    // Controlar bomba
    controlPump(irrigationDecision);
    
    // Enviar telemetria (a cada 30 segundos)
    unsigned long currentTime = millis();
    if (currentTime - lastTelemetry >= TELEMETRY_INTERVAL) {
        sendTelemetry(sensorData, irrigationDecision);
        lastTelemetry = currentTime;
    }
    
    // Log local
    Serial.println("STATUS:");
    Serial.printf("Temp: %.1fC | Umid.Ar: %.1f%% | Umid.Solo: %.1f%%\n", 
                  sensorData.temperatura, sensorData.umidadeAr, sensorData.umidadeSolo);
    Serial.printf("Tanque: %s | Modo: %s | Irrigando: %s\n", 
                  sensorData.tankStatus.c_str(), getModeText().c_str(), 
                  sensorData.irrigando ? "SIM" : "NAO");
    Serial.printf("Condicao Planta: %s\n", sensorData.plantCondition.c_str());
    Serial.println("=======================================");
    
    delay(10000);  // 10 segundos
}