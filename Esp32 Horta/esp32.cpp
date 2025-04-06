/*
    Arquivo base para implementação sem uso de IA
    apenas regras truncadas no esp32 para ocontrole 
    de irrigação de plantas com base nos sensores:
    
    DHT11 - Temperatura e Umidade do ar 
    XXXXX - Temperatura e Umidade do solo
    XXXXX - Pressão do ar
    XXXXX - Chuva 
    
*/
#include <WiFi.h>
#include <DHT.h>

// Definindo os pinos
#define DHTPIN 4       // Pino do DHT11
#define DHTTYPE DHT11  // Tipo de sensor DHT
#define SOIL_MOISTURE_PIN 34 // Pino do sensor de umidade do solo
#define RAIN_SENSOR_PIN 35   // Pino do sensor de chuva
#define RELAY_PIN 12        // Pino para controle do relé da bomba

// Configuração do Wi-Fi
// Para redes básicas
const char* ssid = "Seu_SSID";
const char* password = "Sua_Senha";

// Inicializando o sensor DHT
DHT dht(DHTPIN, DHTTYPE);

// Variáveis para sensores
float temperature, humidity, soilMoisture, soilTemperature, rainStatus, airPressure;

void setup() {
    // Iniciando o serial e o Wi-Fi
    Serial.begin(115200);
    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.println("Conectando ao Wi-Fi...");
    }

    Serial.println("Conectado ao Wi-Fi");

    dht.begin();

    // Inicializando o pino do relé
    pinMode(RELAY_PIN, OUTPUT);
}

void loop() {
    // Leitura dos sensores
    temperature = dht.readTemperature();    // Temperatura (C)
    humidity = dht.readHumidity();          // Umidade do ar (%)
    soilMoisture = analogRead(SOIL_MOISTURE_PIN);  // Umidade do solo (valor analógico)
    rainStatus = digitalRead(RAIN_SENSOR_PIN);     // Sensor de chuva (HIGH ou LOW)

    // Verificar se as leituras são válidas
    if (isnan(temperature) || isnan(humidity)) {
        Serial.println("Falha na leitura do sensor DHT!");
        return;
    }

    // Exibir os valores lidos no Serial Monitor
    Serial.print("Temperatura: ");
    Serial.print(temperature);
    Serial.print(" C\tUmidade do Ar: ");
    Serial.print(humidity);
    Serial.print(" %\tUmidade do Solo: ");
    Serial.print(soilMoisture);
    Serial.print("\tChuva: ");
    Serial.println(rainStatus == HIGH ? "Chovendo" : "Não chovendo");

    // Lógica para controle de irrigação bem básica só template
    if (rainStatus == LOW && soilMoisture < 500 && temperature > 25) {
        Serial.println("Regando a horta...");
        digitalWrite(RELAY_PIN, HIGH); // Acionar a bomba
        delay(10000);  // Bombear por 10 segundos (ajuste conforme necessário)
        digitalWrite(RELAY_PIN, LOW);  // Desligar a bomba
    } else {
        Serial.println("Não é necessário regar.");
    }

    delay(5000);  // Espera 5 segundos para a próxima leitura
}
