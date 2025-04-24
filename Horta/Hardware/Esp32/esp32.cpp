/*
    Arquivo base para implementação sem uso de IA
    apenas regras truncadas no esp32 para ocontrole 
    de irrigação de plantas com base nos sensores:
    
    DHT11 - Temperatura e Umidade do ar
    FC-28 -  Umidade do solo
    FC-37 - Chuva 
    XXXXX - Pressão do ar
*/
//#include <WiFi.h>
#include <DHT.h>

// Definições de pinos dos sensores
#define DHTPIN 4                // Pino do sensor DHT11
#define DHTTYPE DHT11           // Tipo de sensor DHT
#define SOIL_MOISTURE_PIN 5     // Pino do sensor de umidade do solo FC-28
#define RAIN_SENSOR_PIN 14      // Pino analógico do sensor de chuva FC-37

// Configuração do Wi-Fi
// Para redes básicas
// const char* ssid = "Seu_SSID";
// const char* password = "Sua_Senha";

// Inicializando o sensor DHT
DHT dht(DHTPIN, DHTTYPE);

// Variáveis para sensores
float temperature, humidity, soilMoisture, soilTemperature, rainStatus, airPressure;

void setup() {
  // Inicializa monitor serial
  Serial.begin(115200);

  // Configuração dos pinos
  pinMode(SOIL_MOISTURE_PIN, INPUT);
  pinMode(RAIN_SENSOR_PIN, INPUT);

  // Inicializa o sensor DHT
  dht.begin();
}

void loop() {
    // Leitura do sensor de temperatura e umidade do ar
    float temperatura = dht.readTemperature();
    float umidadeAr = dht.readHumidity();
    
    // Valida as leituras do DHT11
    if (isnan(temperatura) || isnan(umidadeAr)) {
        Serial.println("Erro ao ler o sensor DHT11!");
        delay(2000);
        return;
    }
    
    // Leitura do sensor de umidade do solo
    int umidadeSolo = analogRead(SOIL_MOISTURE_PIN);
    
    // Leitura analógica do sensor de chuva
    int valorChuva = analogRead(RAIN_SENSOR_PIN);
    
    // Exibe os valores lidos no monitor serial
    Serial.print("Temperatura: ");
    Serial.print(temperatura);
    Serial.print(" °C, Umidade do Ar: ");
    Serial.print(umidadeAr);
    Serial.print(" %, Umidade do Solo: ");
    Serial.print(umidadeSolo);
    Serial.print(", Nível de Chuva (Analógico): ");
    Serial.println(valorChuva);

    // Lógica de irrigação
    if (valorChuva > 2000) { // Está chovendo
        Serial.println("Chuva detectada. Irrigação desativada.");
    } else if (umidadeSolo < 500) { // Solo seco
        if (temperatura > 25.0 && umidadeAr < 50.0) { // Clima quente e seco
          Serial.println("Condições críticas detectadas. Acionando irrigação...");
        } else {
              Serial.println("Solo seco, mas condições climáticas não são críticas. Irrigação desativada.");
        }
    } else {
        Serial.println("Condições normais. Irrigação desativada.");
    }
    
    delay(2000); // Aguarda 2 segundos antes da próxima leitura
}
