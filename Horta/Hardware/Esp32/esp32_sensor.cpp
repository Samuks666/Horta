/*
    Arquivo principal para controle de irrigação automatizada utilizando ESP32.
    Este código implementa a lógica de controle baseada em regras truncadas 
    sem o uso de inteligência artificial.

    Sensores utilizados:
    - DHT11: Medição de temperatura e umidade do ar.
    - FC-28: Monitoramento da umidade do solo.
    - FC-37: Detecção de chuva.
    - BMP-280: Medição de pressão atmosférica (não me parece necessário).
*/

#include <DHT.h>

// Pinos dos sensores
#define DHTPIN 4
#define DHTTYPE DHT11
#define SOIL_MOISTURE_PIN 32
#define RAIN_SENSOR_PIN 14
#define RELE_PIN 27

DHT dht(DHTPIN, DHTTYPE);

// Número de plantas
#define N_PLANTAS 6

struct Planta {
  const char* nome;
  float tempMin, tempMax;
  float umidArMin, umidArMax;
  float umidSoloMin, umidSoloMax;
};

Planta plantas[N_PLANTAS] = {
  {"Manjericão", 20, 30, 50, 80, 40, 80},
  {"Guaco", 18, 30, 60, 90, 50, 80},
  {"Hortelã", 18, 28, 60, 90, 40, 80},
  {"Ginseng", 15, 25, 60, 90, 50, 80},
  {"Cânfora", 20, 30, 40, 70, 40, 75},
  {"Terramicina", 20, 28, 50, 80, 40, 75}
};

void setup() {
  Serial.begin(115200);
  dht.begin();

  pinMode(SOIL_MOISTURE_PIN, INPUT);
  pinMode(RAIN_SENSOR_PIN, INPUT);
  pinMode(RELE_PIN, OUTPUT);
  //digitalWrite(RELE_PIN, LOW);

  Serial.println("Sistema de irrigação iniciado.");
}

void loop() {
  float temperatura = dht.readTemperature();
  float umidadeAr = dht.readHumidity();

  if (isnan(temperatura) || isnan(umidadeAr)) {
    Serial.println("Erro ao ler DHT11.");
    delay(2000);
    return;
  }

  int leituraSolo = analogRead(SOIL_MOISTURE_PIN);
  float umidadeSolo = 100.0 - ((float)leituraSolo / 4095.0 * 100.0);
  if (umidadeSolo < 0) umidadeSolo = 0;
  if (umidadeSolo > 100) umidadeSolo = 100;

  int leituraChuva = analogRead(RAIN_SENSOR_PIN);
  float chuva = 1.0 - ((float)leituraChuva / 4095.0);

  Serial.println("---- Leituras ----");
  Serial.print("Temperatura: "); Serial.print(temperatura); Serial.println(" °C");
  Serial.print("Umidade do ar: "); Serial.print(umidadeAr); Serial.println(" %");
  Serial.print("Umidade do solo: "); Serial.print(umidadeSolo); Serial.println(" %");
  Serial.print("Chuva (0-1): "); Serial.println(chuva);
  Serial.println("------------------");

  bool precisaIrrigar = false;
  Serial.print("Plantas que precisam de irrigação: ");

  for (int i = 0; i < N_PLANTAS; i++) {
    bool soloSeco = umidadeSolo < plantas[i].umidSoloMin;

    if (soloSeco && chuva < 0.7) {
      precisaIrrigar = true;
      Serial.print(plantas[i].nome);
      Serial.print(" | ");
    }
  }

  Serial.println();

  if (precisaIrrigar) {
    Serial.println("Acionando irrigação...");
    //digitalWrite(RELE_PIN, HIGH);  // Liga relé
  } else {
    Serial.println("Irrigação desativada.");
    //digitalWrite(RELE_PIN, LOW);   // Desliga relé
  }

  delay(2000);
}
