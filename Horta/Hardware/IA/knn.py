"""
    Script (Otimizado) para treinar um modelo KNN usando dados de sensores
    (Umidade e Temperatura do Ar, Umidade do Solo e Precipitação).
    Inclui funcionalidades para carregar, preprocessar,
    treinar e salvar o modelo para rodar em um esp32.
    Inclui redução de dados de treinamento com KMeans.
"""

# Importações necessárias
import os
import pandas as pd
import numpy as np
from sklearn.model_selection import train_test_split
from sklearn.preprocessing import StandardScaler
from sklearn.cluster import KMeans

# Constantes
DATASET_PATH = "TARP.csv"
N_NEIGHBORS = 3
TEST_SIZE = 0.2
RANDOM_STATE = 42
N_ROWS = 2205  # Número de linhas a serem carregadas do dataset
N_CLUSTERS = 50  # Número de clusters para reduzir os dados de treinamento

def load_dataset(filepath, nrows):
    """Carrega as primeiras `nrows` linhas do dataset e realiza validações."""
    if not os.path.exists(filepath):
        raise FileNotFoundError(f"O arquivo {filepath} não foi encontrado.")
    print(f"Carregando as primeiras {nrows} linhas do dataset...")
    df = pd.read_csv(filepath, nrows=nrows)  # Carregar apenas as primeiras `nrows` linhas
    
    # Remover espaços extras nos nomes das colunas
    df.columns = df.columns.str.strip()
    return df

def preprocess_data(df):
    """Preprocessa os dados: converte rótulos e separa características."""
    print("Preprocessando os dados...")
    
    # Colunas necessárias
    required_columns = ['Temperature', 'Air humidity (%)', 'Soil Humidity', 'rainfall', 'Status']
    
    # Verificar se todas as colunas estão presentes
    missing_columns = [col for col in required_columns if col not in df.columns]
    if missing_columns:
        raise KeyError(f"As seguintes colunas estão faltando no dataset: {missing_columns}")
    
    # Selecionar apenas as colunas necessárias
    df = df[required_columns]
    
    # Tratar valores ausentes apenas nas colunas numéricas
    print("Tratando valores ausentes...")
    numeric_columns = ['Temperature', 'Air humidity (%)', 'Soil Humidity', 'rainfall']
    df[numeric_columns] = df[numeric_columns].fillna(df[numeric_columns].mean())
    
    # Converter rótulos da coluna 'Status' para valores binários
    df['Status'] = df['Status'].apply(lambda x: 1 if x == 'ON' else 0)
    
    # Separar características (X) e rótulos (y)
    X = df[['Temperature', 'Air humidity (%)', 'Soil Humidity', 'rainfall']].values
    y = df['Status'].values
    return X, y

def reduce_training_data(X_train, y_train, n_clusters):
    """Reduz os dados de treinamento usando KMeans."""
    print(f"Reduzindo os dados de treinamento para {n_clusters} clusters...")
    kmeans = KMeans(n_clusters=n_clusters, random_state=RANDOM_STATE)
    kmeans.fit(X_train)
    X_reduced = kmeans.cluster_centers_
    
    # Determinar o rótulo mais comum em cada cluster
    y_reduced = []
    for center in X_reduced:
        distances = np.linalg.norm(X_train - center, axis=1)
        closest_points = np.argsort(distances)[:5]  # Pega os 5 pontos mais próximos
        labels = y_train[closest_points]
        y_reduced.append(np.bincount(labels).argmax())  # Classe majoritária
    return X_reduced, np.array(y_reduced)

def save_to_single_header(filename, X_reduced, y_reduced, mean, scale):
    """Salva todos os dados necessários em um único arquivo de cabeçalho C++."""
    print(f"Salvando todos os dados em {filename}...")
    with open(filename, 'w') as f:
        f.write(f'#ifndef MODEL_DATA_H\n')
        f.write(f'#define MODEL_DATA_H\n\n')

        # Salvar os clusters reduzidos
        f.write(f'static const float X_train_reduced[] = {{\n')
        for row in X_reduced:
            f.write('    ' + ', '.join(f'{x:.6f}' for x in row) + ',\n')
        f.write('};\n\n')

        # Salvar os rótulos reduzidos
        f.write(f'static const int y_train_reduced[] = {{\n')
        f.write('    ' + ', '.join(map(str, y_reduced)) + '\n')
        f.write('};\n\n')

        # Salvar a média para padronização
        f.write(f'static const float scaler_mean[] = {{\n')
        f.write('    ' + ', '.join(f'{x:.6f}' for x in mean) + '\n')
        f.write('};\n\n')

        # Salvar o desvio padrão para padronização
        f.write(f'static const float scaler_scale[] = {{\n')
        f.write('    ' + ', '.join(f'{x:.6f}' for x in scale) + '\n')
        f.write('};\n\n')

        f.write(f'#endif // MODEL_DATA_H\n')

def main():
    """Função principal para executar o pipeline de treinamento do modelo."""
    # Carregar e preprocessar os dados
    df = load_dataset(DATASET_PATH, N_ROWS)
    X, y = preprocess_data(df)

    # Dividir os dados em treino e teste
    print("Dividindo os dados em treino e teste...")
    X_train, X_test, y_train, y_test = train_test_split(X, y, test_size=TEST_SIZE, random_state=RANDOM_STATE)

    # Padronizar os dados
    print("Padronizando os dados...")
    scaler = StandardScaler()
    X_train = scaler.fit_transform(X_train)
    X_test = scaler.transform(X_test)

    # Reduzir os dados de treinamento
    X_train_reduced, y_train_reduced = reduce_training_data(X_train, y_train, N_CLUSTERS)

    # Salvar todos os dados em um único arquivo de cabeçalho
    save_to_single_header(
        'model_data.h',
        X_train_reduced,
        y_train_reduced,
        scaler.mean_,
        scaler.scale_
    )

    print("Processo concluído com sucesso!")

if __name__ == "__main__":
    main()
