#include <SPIFFS.h>
#include <Arduino.h>
#include <math.h>
#include <string.h>

#define NUM_SENSORS 7  // Número de características
#define NUM_TRAINING 100  // Número de amostras no dataset

// Estrutura para armazenar dados
float X[NUM_TRAINING][NUM_SENSORS];
int y[NUM_TRAINING];

// Função para carregar os dados do arquivo .npy
void loadData() {
    File X_file = SPIFFS.open("/X_train.npy", "r");
    File y_file = SPIFFS.open("/y_train.npy", "r");

    if (X_file && y_file) {
        for (int i = 0; i < NUM_TRAINING; i++) {
            for (int j = 0; j < NUM_SENSORS; j++) {
                X_file.readBytes((char*)&X[i][j], sizeof(float));
            }
            y_file.readBytes((char*)&y[i], sizeof(int));
        }
        X_file.close();
        y_file.close();
    } else {
        Serial.println("Falha ao abrir arquivos");
    }
}

// Função de cálculo da distância Euclidiana
float euclideanDistance(float a[], float b[]) {
    float sum = 0;
    for (int i = 0; i < NUM_SENSORS; i++) {
        sum += pow(a[i] - b[i], 2);
    }
    return sqrt(sum);
}

// Função para predizer se deve regar ou não
int predict(float input[]) {
    float distances[NUM_TRAINING];
    int sortedIndices[NUM_TRAINING];

    // Calcular as distâncias de Euclides
    for (int i = 0; i < NUM_TRAINING; i++) {
        distances[i] = euclideanDistance(input, X[i]);
        sortedIndices[i] = i;
    }

    // Ordenar as distâncias
    for (int i = 0; i < NUM_TRAINING - 1; i++) {
        for (int j = i + 1; j < NUM_TRAINING; j++) {
            if (distances[sortedIndices[i]] > distances[sortedIndices[j]]) {
                int temp = sortedIndices[i];
                sortedIndices[i] = sortedIndices[j];
                sortedIndices[j] = temp;
            }
        }
    }

    // Contar os votos dos k vizinhos mais próximos (k = 3)
    int votes[2] = {0, 0};
    for (int i = 0; i < 3; i++) {
        votes[y[sortedIndices[i]]]++;
    }

    // Retornar a classe com mais votos (1 ou 0)
    return (votes[1] > votes[0]) ? 1 : 0;
}

void setup() {
    Serial.begin(115200);

    // Iniciar o SPIFFS
    if (!SPIFFS.begin()) {
        Serial.println("Falha ao iniciar o SPIFFS");
        return;
    }

    // Carregar dados de treinamento
    loadData();

    // Exemplo de entrada para previsão
    float newData[NUM_SENSORS] = {30.0, 25.0, 5.0, 20.0, 65.0, 1010.0, 0.0};
    int decision = predict(newData);

    Serial.print("Decisão para regar (1 = sim, 0 = não): ");
    Serial.println(decision);
}

void loop() {
    // Lógica adicional para o loop, se necessário
}
