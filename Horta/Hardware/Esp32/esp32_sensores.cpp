/*
    Arquivo base para implementação sem uso de IA
    apenas regras truncadas no esp32 para ocontrole 
    de irrigação de plantas com base nos sensores:
    
    DHT11 - Temperatura e Umidade do ar
    FC-28 -  Umidade do solo
    FC-37 - Chuva 
    XXXXX - Pressão do ar
*/

#include <esp_now.h>
#include <WiFi.h>
#include <DHT.h>

// Definições de pinos dos sensores
#define DHTPIN 4                // Pino do sensor de umidade e temp ar DHT11
#define DHTTYPE DHT11           // Tipo de sensor DHT
#define SOIL_MOISTURE_PIN 5     // Pino do sensor de umidade do solo FC-28
#define RAIN_SENSOR_PIN 14      // Pino analógico do sensor de chuva FC-37

// Inicializando o sensor DHT
DHT dht(DHTPIN, DHTTYPE);

// Estrutura para enviar dados via ESP-NOW
typedef struct {
    float temperatura;
    float umidadeAr;
    int umidadeSolo;
    int valorChuva;
    bool irrigar; // Comando de irrigação
} SensorData;

SensorData sensorData;

// Endereço MAC do ESP32 receptor
uint8_t broadcastAddress[] = {0x24, 0x6F, 0x28, 0xAB, 0xCD, 0xEF};

// Variáveis para controle de tempo
unsigned long lastSensorRead = 0;       // Última leitura dos sensores
unsigned long lastIrrigationCheck = 0; // Última verificação da lógica de irrigação
const unsigned long sensorInterval = 2000; // Intervalo de leitura dos sensores (2 segundos)
const unsigned long irrigationInterval = 12 * 60 * 60 * 1000; // Intervalo de verificação da irrigação (12 horas)

// Funções modularizadas

// Inicializa o ESP-NOW
void initEspNow() {
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();

    if (esp_now_init() != ESP_OK) {
        Serial.println("Erro ao inicializar ESP-NOW");
        return;
    }

    esp_now_register_send_cb([](const uint8_t *mac_addr, esp_now_send_status_t status) {
        Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Comando enviado com sucesso!" : "Falha no envio do comando!");
    });

    esp_now_peer_info_t peerInfo;
    memcpy(peerInfo.peer_addr, broadcastAddress, 6);
    peerInfo.channel = 0;
    peerInfo.encrypt = false;

    if (esp_now_add_peer(&peerInfo) != ESP_OK) {
        Serial.println("Erro ao adicionar peer");
        return;
    }

    Serial.println("ESP-NOW inicializado.");
}

// Lê os sensores e atualiza a estrutura `sensorData`
void readSensors() {
    sensorData.temperatura = dht.readTemperature();
    sensorData.umidadeAr = dht.readHumidity();

    if (isnan(sensorData.temperatura) || isnan(sensorData.umidadeAr)) {
        Serial.println("Erro ao ler o sensor DHT11!");
        return;
    }

    sensorData.umidadeSolo = analogRead(SOIL_MOISTURE_PIN);
    sensorData.valorChuva = analogRead(RAIN_SENSOR_PIN);

    Serial.print("Temperatura: ");
    Serial.print(sensorData.temperatura);
    Serial.print(" °C, Umidade do Ar: ");
    Serial.print(sensorData.umidadeAr);
    Serial.print(" %, Umidade do Solo: ");
    Serial.print(sensorData.umidadeSolo);
    Serial.print(", Nível de Chuva: ");
    Serial.println(sensorData.valorChuva);
}

// Executa a lógica de irrigação
void checkIrrigationLogic() {
    if (sensorData.valorChuva > 2000) { // Está chovendo
        sensorData.irrigar = false;
        Serial.println("Chuva detectada. Irrigação desativada.");
    } else if (sensorData.umidadeSolo < 400) { // Solo muito seco
        if (sensorData.temperatura > 22.0 && sensorData.umidadeAr < 60.0) { // Clima quente e seco
            sensorData.irrigar = true;
            Serial.println("Condições críticas detectadas. Acionando irrigação...");
        } else if (sensorData.temperatura > 18.0 && sensorData.umidadeAr < 70.0) { // Clima moderado
            sensorData.irrigar = true;
            Serial.println("Condições moderadas detectadas. Acionando irrigação...");
        } else {
            sensorData.irrigar = false;
            Serial.println("Solo seco, mas condições climáticas não justificam irrigação. Irrigação desativada.");
        }
    } else if (sensorData.umidadeSolo >= 400 && sensorData.umidadeSolo <= 800) { // Solo úmido
        sensorData.irrigar = false;
        Serial.println("Solo com umidade adequada. Irrigação desativada.");
    } else { // Solo muito úmido
        sensorData.irrigar = false;
        Serial.println("Solo muito úmido. Irrigação desativada.");
    }
}

// Envia os dados via ESP-NOW
void sendData() {
    esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *)&sensorData, sizeof(sensorData));
    if (result == ESP_OK) {
        Serial.println("Comando enviado para o receptor.");
    } else {
        Serial.println("Erro ao enviar comando.");
    }
}

void setup() {
    Serial.begin(115200);

    pinMode(SOIL_MOISTURE_PIN, INPUT);
    pinMode(RAIN_SENSOR_PIN, INPUT);

    dht.begin();
    initEspNow();
}

void loop() {
    unsigned long currentMillis = millis();

    // Leitura dos sensores a cada 2 segundos
    if (currentMillis - lastSensorRead >= sensorInterval) {
        lastSensorRead = currentMillis;
        readSensors();
    }

    // Verificar a lógica de irrigação apenas uma ou duas vezes ao dia
    if (currentMillis - lastIrrigationCheck >= irrigationInterval) {
        lastIrrigationCheck = currentMillis;
        checkIrrigationLogic();
        sendData();
    }
}
