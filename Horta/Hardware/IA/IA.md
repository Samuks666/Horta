# Documentação da IA

## Descrição
O script `knn.py` foi desenvolvido para treinar um modelo KNN (K-Nearest Neighbors) utilizando dados de sensores ambientais como umidade, temperatura do ar, umidade do solo e precipitação. 
Ele é otimizado para preprocessar os dados, reduzir o conjunto de treinamento (K-Means) e gerar um modelo compacto que pode ser implementado em um dispositivo ESP32.

## Funcionalidades
- **Carregamento e validação do Dataset**: O script utiliza o arquivo `TARP.csv`, que contém os dados de sensores.
- **Pré-processamento**:
  - Tratamento de valores ausentes.
  - Conversão de rótulos em valores binários (`ON` -> 1 e `OFF` -> 0).
  - Normalização dos dados com `StandardScaler`.
- **Redução dos Dados de Treinamento**:
  - Utiliza o algoritmo `KMeans` para reduzir o número de amostras de treinamento, agrupando os dados em clusters.
  - Determina o rótulo mais frequente em cada cluster.
- **Geração de Arquivo de Cabeçalho (`model_data.h`)**:
  - Armazena os dados reduzidos (`X_train_reduced` e `y_train_reduced`).
  - Inclui as médias e escalas utilizadas para normalização dos dados.

## Dataset: `TARP.csv`
O arquivo `TARP.csv` é o dataset principal utilizado para o treinamento. 

### Link para o Dataset Original
Os dados utilizados estão disponíveis publicamente no Kaggle e podem ser encontrados no seguinte link:  
[Dataset for Predicting Watering the Plants](https://www.kaggle.com/datasets/nelakurthisudheer/dataset-for-predicting-watering-the-plants)

### Colunas Necessárias:
- **Temperature**: Temperatura do ar.
- **Air humidity (%)**: Umidade relativa do ar.
- **Soil Humidity**: Umidade do solo.
- **rainfall**: Precipitação.
- **Status**: Irrigação (`ON` ou `OFF`).

## Dados do Arquivo `model_data.h`
O arquivo gerado `model_data.h` é um cabeçalho C++ que contém:
1. **`X_train_reduced`**: Dados de treinamento reduzidos, otimizados para execução em dispositivos de baixa capacidade computacional.
2. **`y_train_reduced`**: Rótulos correspondentes aos clusters reduzidos.
3. **`scaler_mean`** e **`scaler_scale`**: Parâmetros utilizados para normalização dos dados.
