"""
Script para treinar um modelo KNN usando dados de sensores
(Umidade e Temperatura do Ar, Umidade do Solo e Precipitação).
Inclui funcionalidades para carregar, preprocessar,
treinar e salvar o modelo para rodar em um esp32.
"""

# Importações necessárias
import os
import pickle
import pandas as pd
import numpy as np
from sklearn.model_selection import train_test_split
from sklearn.preprocessing import StandardScaler
from sklearn.neighbors import KNeighborsClassifier

# Constantes
DATASET_PATH = "TARP.csv"
N_NEIGHBORS = 3
TEST_SIZE = 0.2
RANDOM_STATE = 42
N_ROWS = 2205  # Número de linhas a serem carregadas do dataset

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

def save_to_header(filename, var_name, array):
    """Salva um array em formato de arquivo de cabeçalho C++."""
    print(f"Salvando {var_name} em {filename}...")
    with open(filename, 'w') as f:
        f.write(f'#ifndef {var_name.upper()}_H\n')
        f.write(f'#define {var_name.upper()}_H\n\n')
        f.write(f'static const float {var_name}[] = {{\n')
        for row in array:
            f.write('    ' + ', '.join(f'{x:.6f}' for x in row) + ',\n')
        f.write('};\n\n')
        f.write(f'#endif // {var_name.upper()}_H\n')

def save_labels(filename, var_name, array):
    """Salva os rótulos em formato de arquivo de cabeçalho C++."""
    print(f"Salvando {var_name} em {filename}...")
    with open(filename, 'w') as f:
        f.write(f'#ifndef {var_name.upper()}_H\n')
        f.write(f'#define {var_name.upper()}_H\n\n')
        f.write(f'static const int {var_name}[] = {{\n')
        f.write(', '.join(map(str, array)) + '\n')
        f.write('};\n\n')
        f.write(f'#endif // {var_name.upper()}_H\n')

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

    # Treinar o modelo KNN
    print(f"Treinando o modelo KNN com {N_NEIGHBORS} vizinhos...")
    knn = KNeighborsClassifier(n_neighbors=N_NEIGHBORS)
    knn.fit(X_train, y_train)

    # Avaliar o modelo
    accuracy = knn.score(X_test, y_test)
    print(f"Acurácia do modelo: {accuracy * 100:.2f}%")

    # Salvar os dados de treino e teste
    save_to_header('X_train.h', 'X_train', X_train)
    save_to_header('X_test.h', 'X_test', X_test)
    save_labels('y_train.h', 'y_train', y_train)
    save_labels('y_test.h', 'y_test', y_test)

    # Salvar o modelo e o scaler
    print("Salvando o modelo e o scaler...")
    with open('knn_model.pkl', 'wb') as model_file:
        pickle.dump(knn, model_file)
    with open('scaler.pkl', 'wb') as scaler_file:
        pickle.dump(scaler, scaler_file)

    # Salvar as previsões
    print("Salvando as previsões...")
    predictions = knn.predict(X_test)
    np.save('predictions.npy', predictions)

    print("Processo concluído com sucesso!")

if __name__ == "__main__":
    main()
