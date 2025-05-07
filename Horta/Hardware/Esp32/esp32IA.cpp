/*
    Integração do modelo KNN treinado com o ESP32
    para controle de irrigação baseado em sensores:
    - DHT11: Temperatura e Umidade do ar
    - FC-28: Umidade do solo
    - FC-37: Sensor de chuva
*/

#include <DHT.h>
#include "model_data.h"  // Header único com os dados do modelo KNN

// Definições de pinos dos sensores
#define DHTPIN 4                // Pino do sensor DHT11
#define DHTTYPE DHT11           // Tipo de sensor DHT
#define SOIL_MOISTURE_PIN 5     // Pino do sensor de umidade do solo FC-28
#define RAIN_SENSOR_PIN 14      // Pino analógico do sensor de chuva FC-37

// Inicializando o sensor DHT
DHT dht(DHTPIN, DHTTYPE);

// Parâmetros do modelo KNN
#define N_FEATURES 4            // Número de características (sensores)
#define N_TRAIN_REDUCED 50      // Número de clusters reduzidos
#define N_NEIGHBORS 3           // Número de vizinhos mais próximos

// Função para padronizar os dados de entrada
void standardize(float *input, int n_features) {
    for (int i = 0; i < n_features; i++) {
        input[i] = (input[i] - scaler_mean[i]) / scaler_scale[i];
    }
}

// Função para calcular a distância euclidiana
float euclidean_distance(const float *a, const float *b, int n_features) {
    float distance = 0.0;
    for (int i = 0; i < n_features; i++) {
        distance += pow(a[i] - b[i], 2);
    }
    return sqrt(distance);
}

// Função para realizar a predição usando KNN
int knn_predict(const float *input) {
    float distances[N_TRAIN_REDUCED];
    int indices[N_NEIGHBORS];
    float min_distances[N_NEIGHBORS];

    // Inicializar os arrays de vizinhos mais próximos
    for (int i = 0; i < N_NEIGHBORS; i++) {
        min_distances[i] = INFINITY;
        indices[i] = -1;
    }

    // Calcular distâncias para todos os clusters reduzidos
    for (int i = 0; i < N_TRAIN_REDUCED; i++) {
        float distance = euclidean_distance(input, &X_train_reduced[i * N_FEATURES], N_FEATURES);

        // Atualizar os vizinhos mais próximos
        for (int j = 0; j < N_NEIGHBORS; j++) {
            if (distance < min_distances[j]) {
                // Deslocar os valores para abrir espaço para o novo vizinho
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

    // Determinar a classe majoritária
    int votes[2] = {0, 0};  // Supondo classes 0 e 1
    for (int i = 0; i < N_NEIGHBORS; i++) {
        votes[y_train_reduced[indices[i]]]++;
    }

    return (votes[1] > votes[0]) ? 1 : 0;
}

void setup() {
    // Inicializa monitor serial
    Serial.begin(115200);

    // Configuração dos pinos
    pinMode(SOIL_MOISTURE_PIN, INPUT);
    pinMode(RAIN_SENSOR_PIN, INPUT);

    // Inicializa o sensor DHT
    dht.begin();

    Serial.println("Sistema de irrigação iniciado.");
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

    // Validar leituras dos sensores analógicos
    if (umidadeSolo < 0 || valorChuva < 0) {
        Serial.println("Erro ao ler os sensores analógicos!");
        delay(2000);
        return;
    }

    // Criar o vetor de entrada com os valores lidos
    float input[N_FEATURES] = {
        temperatura,
        umidadeAr,
        (float)umidadeSolo / 4095.0 * 100.0,  // Converter para porcentagem
        (float)valorChuva                    // Usar o valor de rainfall diretamente
    };

    // Padronizar os dados de entrada
    standardize(input, N_FEATURES);

    // Fazer a predição usando o modelo KNN
    int prediction = knn_predict(input);

    // Exibir os valores lidos e a predição no monitor serial
    Serial.print("Temperatura: ");
    Serial.print(temperatura);
    Serial.print(" °C, Umidade do Ar: ");
    Serial.print(umidadeAr);
    Serial.print(" %, Umidade do Solo: ");
    Serial.print((float)umidadeSolo / 4095.0 * 100.0);
    Serial.print(" %, Nível de Chuva: ");
    Serial.print(valorChuva);
    Serial.print(", Predição: ");
    Serial.println(prediction == 1 ? "Irrigar" : "Não Irrigar");

    // Lógica de controle de irrigação com base na predição
    if (prediction == 1) {
        Serial.println("Acionando irrigação...");
    } else {
        Serial.println("Irrigação desativada.");
    }

    delay(2000); // Aguarda 2 segundos antes da próxima leitura
}
