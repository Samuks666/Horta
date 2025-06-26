# Sistema de Inteligência Artificial para Irrigação

O sistema de IA utiliza algoritmo KNN (K-Nearest Neighbors) para tomar decisões inteligentes de irrigação baseadas em dados ambientais coletados pelos sensores.

<img src="IA_diagram.png" alt="Diagrama do Sistema de IA" width="300">

# Dados Técnicos do Sistema de IA

| Característica        | Descrição                              |
|-----------------------|----------------------------------------|
| Algoritmo             | K-Nearest Neighbors (KNN)             |
| Features              | 3 variáveis (Temperatura, Umidade Ar, Umidade Solo) |
| Dataset               | TARP.csv (1000+ amostras)             |
| Clusters Reduzidos    | 100 pontos para otimização ESP32      |
| Vizinhos (K)          | 3 vizinhos mais próximos              |
| Acurácia              | ~50-60% em testes                     |

## Arquivos do Sistema

| Arquivo               | Função                                 |
|-----------------------|----------------------------------------|
| `IA_simple.py`        | Script de treinamento do modelo       |
| `model_data.h`        | Dados do modelo em formato C++        |
| `TARP.csv`           | Dataset para treinamento              |
| `esp32IA.cpp`        | Implementação no ESP32                |

## Features de Entrada

| Feature               | Descrição                              | Faixa Típica |
|-----------------------|----------------------------------------|--------------|
| Temperatura do Ar     | Temperatura ambiente em °C            | 15-40°C      |
| Umidade do Ar         | Umidade relativa do ar em %            | 20-90%       |
| Umidade do Solo       | Umidade do solo em %                   | 0-100%       |

## Componentes
- Dataset TARP.csv com dados históricos de irrigação
- Algoritmo KNN otimizado para ESP32
- Preprocessamento com StandardScaler
- Redução de dados usando K-Means clustering

## Funcionamento

### 1. Coleta de Dados
O sistema coleta continuamente dados dos sensores:
- DHT11: Temperatura e umidade do ar
- FC-28: Umidade do solo
- BMP280: Pressão atmosférica (opcional)

### 2. Preprocessamento
Os dados são normalizados usando os parâmetros do StandardScaler:
- **Média**: [24.28, 58.45, 45.33]
- **Escala**: [6.75, 30.04, 26.03]

### 3. Decisão KNN
O algoritmo encontra os 3 vizinhos mais próximos e decide por votação majoritária.

## Código de Treinamento

Segue o script principal para treinar o modelo:

[Código em Python da IA](./IA_simple.py)

## Implementação no ESP32

O modelo treinado é convertido para arrays C++ no arquivo `model_data.h`:

```cpp
#include "model_data.h"

// Função de normalização
void standardize(float *input, int n_features) {
    for (int i = 0; i < n_features; i++) {
        input[i] = (input[i] - scaler_mean[i]) / scaler_scale[i];
    }
}

// Função de distância euclidiana
float euclidean_distance(const float *a, const float *b, int n_features) {
    float distance = 0.0;
    for (int i = 0; i < n_features; i++) {
        float diff = a[i] - b[i];
        distance += diff * diff;
    }
    return sqrt(distance);
}

// Função de predição KNN
int knn_predict(const float *input) {
    float min_distances[3];
    int indices[3];
    
    // Inicializa distâncias
    for (int i = 0; i < 3; i++) {
        min_distances[i] = INFINITY;
        indices[i] = -1;
    }
    
    // Encontra 3 vizinhos mais próximos
    for (int i = 0; i < 100; i++) {
        float distance = euclidean_distance(input, &X_train_reduced[i * 3], 3);
        
        for (int j = 0; j < 3; j++) {
            if (distance < min_distances[j]) {
                // Desloca elementos
                for (int k = 2; k > j; k--) {
                    min_distances[k] = min_distances[k - 1];
                    indices[k] = indices[k - 1];
                }
                min_distances[j] = distance;
                indices[j] = i;
                break;
            }
        }
    }
    
    // Votação majoritária
    int votes[2] = {0, 0};
    for (int i = 0; i < 3; i++) {
        if (indices[i] >= 0) {
            votes[y_train_reduced[indices[i]]]++;
        }
    }
    
    return (votes[1] > votes[0]) ? 1 : 0;
}
```

# Exemplo de Uso

```cpp
// Leitura dos sensores
float temperatura = dht.readTemperature();
float umidadeAr = dht.readHumidity();
float umidadeSolo = map(analogRead(SOIL_PIN), 0, 4095, 100, 0);

// Preparar dados para IA
float input[3] = {temperatura, umidadeAr, umidadeSolo};
float input_scaled[3];
for (int i = 0; i < 3; i++) {
    input_scaled[i] = input[i];
}

// Normalizar dados
standardize(input_scaled, 3);

// Fazer predição
int decision = knn_predict(input_scaled);

// Resultado: 1 = Irrigar, 0 = Não irrigar
if (decision == 1) {
    Serial.println("IA DECIDIU: Irrigar");
    // Ativar irrigação
} else {
    Serial.println("IA DECIDIU: Não irrigar");
    // Manter irrigação desligada
}
```

# Saída no Terminal

```
Carregando dataset TARP.csv...
Dataset carregado: 985 amostras válidas de 1000 totais
Preprocessando dados...
Features: ['Air temperature (C)', 'Air humidity (%)', 'Soil Moisture']
Formato dos dados: X=(985, 3), y=(985,)
Distribuição das classes: ON=492, OFF=493
Dividindo dados em treino e teste...
Padronizando dados...
Treinando modelo KNN com k=3...
Avaliando modelo...
Acurácia: 87.31%

Matriz de Confusão:
[[89 10]
 [15 83]]

Reduzindo dados de treinamento para 100 clusters...
Dados reduzidos: (100, 3)
Distribuição dos clusters: ON=48, OFF=52
Salvando modelo no arquivo model_data.h...
Modelo salvo com sucesso!

Processo concluído com sucesso!
Acurácia final: 87.31%
Modelo exportado para: model_data.h
Dados reduzidos para 100 clusters
Pronto para uso no ESP32!
```

## Dataset: TARP.csv

O dataset utilizado contém dados históricos de irrigação com as seguintes características:

### Fonte dos Dados
- **Origem**: [Dataset for Predicting Watering the Plants - Kaggle](https://www.kaggle.com/datasets/nelakurthisudheer/dataset-for-predicting-watering-the-plants)
- **Tamanho**: 1000+ amostras
- **Período**: Dados coletados ao longo de diferentes estações

### Colunas Principais
- **Air temperature (C)**: Temperatura ambiente
- **Air humidity (%)**: Umidade relativa do ar
- **Soil Moisture**: Umidade do solo
- **Status**: Decisão de irrigação (ON/OFF)

### Estatísticas do Dataset
- **Temperatura**: 15.5°C - 36.9°C (média: 24.3°C)
- **Umidade do Ar**: 0.6% - 58.5% (média: 22.4%)
- **Umidade do Solo**: 20% - 90% (média: 45.3%)

## Otimizações para ESP32

### Redução de Memória
- **Dataset Original**: 1000+ pontos → **Reduzido**: 100 clusters
- **Memória RAM**: ~3KB para armazenar modelo completo
- **Processamento**: Otimizado para microcontrolador

### Algoritmos Utilizados
1. **K-Means Clustering**: Reduz dados de treinamento
2. **Standard Scaler**: Normalização eficiente
3. **KNN Otimizado**: Implementação rápida em C++

## Aplicações no Sistema de Irrigação

- **Decisão Inteligente**: IA analisa condições e decide automaticamente
- **Aprendizado Contínuo**: Modelo baseado em dados históricos reais
- **Eficiência Energética**: Processamento otimizado para ESP32
- **Precisão**: Acurácia de ~87% nas decisões de irrigação
- **Adaptabilidade**: Funciona com diferentes tipos de plantas e condições
- **Integração**: Combina com sensores e controle manual via ThingsBoard