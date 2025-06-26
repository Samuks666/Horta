"""
Script simplificado para treinar um modelo KNN para controle de irrigação
Otimizado para uso em ESP32 com apenas as features essenciais
"""

import pandas as pd
import numpy as np
from sklearn.model_selection import train_test_split
from sklearn.preprocessing import StandardScaler
from sklearn.neighbors import KNeighborsClassifier
from sklearn.metrics import accuracy_score, classification_report, confusion_matrix
from sklearn.cluster import KMeans

# Configurações
DATASET_PATH = "TARP.csv"
TEST_SIZE = 0.2
RANDOM_STATE = 42
N_CLUSTERS = 100  # Reduzido para ser mais eficiente no ESP32
K_NEIGHBORS = 3   # Número de vizinhos para KNN

def save_model_to_header(X_train_reduced, y_train_reduced, scaler_mean, scaler_scale, filename="model_data.h"):
    """Salva os dados do modelo em formato C++ header"""
    print(f"Salvando modelo no arquivo {filename}...")

    with open(filename, "w", encoding="UTF-8") as f:
        f.write("#ifndef MODEL_DATA_H\n")
        f.write("#define MODEL_DATA_H\n\n")
        
        # Dados de treinamento reduzidos
        f.write("static const float X_train_reduced[] = {\n")
        flat_X = X_train_reduced.flatten()
        for i, v in enumerate(flat_X):
            if i % X_train_reduced.shape[1] == 0:
                f.write("    ")
            f.write(f"{v:.6f}")
            if i < len(flat_X) - 1:
                f.write(",")
                if (i + 1) % X_train_reduced.shape[1] == 0:
                    f.write("\n")
                else:
                    f.write("    ")
        f.write("\n};\n\n")
        
        # Labels de treinamento reduzidos
        f.write("static const int y_train_reduced[] = {\n    ")
        for i, v in enumerate(y_train_reduced):
            f.write(f"{int(v)}")
            if i < len(y_train_reduced) - 1:
                f.write(", ")
                if (i + 1) % 15 == 0:
                    f.write("\n    ")
        f.write("\n};\n\n")
        
        # Parâmetros do scaler (média)
        f.write("static const float scaler_mean[] = {\n    ")
        for i, v in enumerate(scaler_mean):
            f.write(f"{v:.6f}")
            if i < len(scaler_mean) - 1:
                f.write(", ")
        f.write("\n};\n\n")
        
        # Parâmetros do scaler (escala)
        f.write("static const float scaler_scale[] = {\n    ")
        for i, v in enumerate(scaler_scale):
            f.write(f"{v:.6f}")
            if i < len(scaler_scale) - 1:
                f.write(", ")
        f.write("\n};\n\n")
        
        f.write("#endif // MODEL_DATA_H\n")
    
    print("Modelo salvo com sucesso!")

def load_dataset(filepath):
    """Carrega e limpa o dataset"""
    print(f"Carregando dataset {filepath}...")
    
    df = pd.read_csv(filepath)
    df.columns = df.columns.str.strip()  # Remove espaços das colunas
    
    # Remove linhas com dados ausentes
    df_clean = df.dropna(subset=['Air temperature (C)', 'Air humidity (%)', 'Soil Moisture', 'Status'])
    
    print(f"Dataset carregado: {len(df_clean)} amostras válidas de {len(df)} totais")
    return df_clean

def preprocess_data(df):
    """Preprocessa os dados usando apenas as 3 features principais"""
    print("Preprocessando dados...")
    
    # Seleciona apenas as colunas necessárias (3 features + target)
    features_cols = ['Air temperature (C)', 'Air humidity (%)', 'Soil Moisture']
    
    X = df[features_cols].values
    y = df['Status'].apply(lambda x: 1 if x == 'ON' else 0).values
    
    print(f"Features: {features_cols}")
    print(f"Formato dos dados: X={X.shape}, y={y.shape}")
    print(f"Distribuição das classes: ON={np.sum(y)}, OFF={len(y) - np.sum(y)}")
    
    return X, y

def train_knn_model(X_train, y_train):
    """Treina o modelo KNN"""
    print(f"Treinando modelo KNN com k={K_NEIGHBORS}...")
    
    model = KNeighborsClassifier(n_neighbors=K_NEIGHBORS)
    model.fit(X_train, y_train)
    
    return model

def evaluate_model(model, X_test, y_test):
    """Avalia o modelo treinado"""
    print("Avaliando modelo...")
    
    y_pred = model.predict(X_test)
    accuracy = accuracy_score(y_test, y_pred)
    
    print(f"Acurácia: {accuracy:.2%}")
    print("\nMatriz de Confusão:")
    print(confusion_matrix(y_test, y_pred))
    print("\nRelatório de Classificação:")
    print(classification_report(y_test, y_pred, zero_division=0))
    
    return accuracy

def reduce_training_data(X_train, y_train, n_clusters=N_CLUSTERS):
    """Reduz os dados de treinamento usando KMeans para uso eficiente no ESP32"""
    print(f"Reduzindo dados de treinamento para {n_clusters} clusters...")
    
    kmeans = KMeans(n_clusters=n_clusters, random_state=RANDOM_STATE, n_init=10)
    kmeans.fit(X_train)
    
    X_reduced = kmeans.cluster_centers_
    y_reduced = []
    
    # Para cada cluster, encontra o label mais comum
    for center in X_reduced:
        # Encontra os pontos mais próximos do centro
        distances = np.linalg.norm(X_train - center, axis=1)
        closest_indices = np.argsort(distances)[:10]  # 10 pontos mais próximos
        closest_labels = y_train[closest_indices]
        
        # Label mais comum
        most_common_label = np.bincount(closest_labels).argmax()
        y_reduced.append(most_common_label)
    
    print(f"Dados reduzidos: {X_reduced.shape}")
    print(f"Distribuição dos clusters: ON={np.sum(y_reduced)}, OFF={len(y_reduced) - np.sum(y_reduced)}")
    
    return X_reduced, np.array(y_reduced)

def main():
    # Carrega o dataset
    df = load_dataset(DATASET_PATH)
    
    # Preprocessa os dados
    X, y = preprocess_data(df)
    
    # Divide em treino e teste
    print("Dividindo dados em treino e teste...")
    X_train, X_test, y_train, y_test = train_test_split(
        X, y, test_size=TEST_SIZE, random_state=RANDOM_STATE, stratify=y
    )
    
    # Padroniza os dados
    print("Padronizando dados...")
    scaler = StandardScaler()
    X_train_scaled = scaler.fit_transform(X_train)
    X_test_scaled = scaler.transform(X_test)
    
    # Treina o modelo KNN
    model = train_knn_model(X_train_scaled, y_train)
    
    # Avalia o modelo
    evaluate_model(model, X_test_scaled, y_test)
    
    # Reduz os dados para uso no ESP32
    X_train_reduced, y_train_reduced = reduce_training_data(X_train_scaled, y_train, N_CLUSTERS)
    
    # Salva o modelo no formato C++
    save_model_to_header(
        X_train_reduced, 
        y_train_reduced, 
        scaler.mean_, 
        scaler.scale_, 
        filename="model_data.h"
    )
    
    print("\nProcesso concluído com sucesso!")
    print("Acurácia final: {accuracy:.2%}")
    print("Modelo exportado para: model_data.h")
    print("Dados reduzidos para {N_CLUSTERS} clusters")
    print("Pronto para uso no ESP32!")

if __name__ == "__main__":
    main()
