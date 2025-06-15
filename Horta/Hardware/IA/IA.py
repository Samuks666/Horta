"""
    Script otimizado para treinar e exportar modelo usando dados de sensores.
    Exporta arrays reduzidos e parâmetros de scaler em formato C++ (model_data.h).
"""

import os
import pandas as pd
import numpy as np
from sklearn.model_selection import train_test_split, StratifiedKFold, GridSearchCV
from sklearn.preprocessing import StandardScaler
from sklearn.metrics import accuracy_score, classification_report, confusion_matrix
from xgboost import XGBClassifier

# Configurações
DATASET_PATH = "TARP.csv"
TEST_SIZE = 0.2
RANDOM_STATE = 42
CV_FOLDS = 5

def save_model_to_header(X_train_reduced, y_train_reduced, scaler_mean, scaler_scale, filename="model_data.h"):
    "Função para salvar os dados do modelo em um arquivo de cabeçalho C++."
    with open(filename, "w") as f:
        f.write("#ifndef MODEL_DATA_H\n")
        f.write("#define MODEL_DATA_H\n\n")
        f.write("static const float X_train_reduced[] = {\n")
        flat_X = X_train_reduced.flatten()
        for i, v in enumerate(flat_X):
            f.write(f"    {v:.6f}")
            if i < len(flat_X) - 1:
                f.write(",")
            if (i + 1) % X_train_reduced.shape[1] == 0:
                f.write("\n")
        f.write("};\n\n")
        f.write("static const int y_train_reduced[] = {\n    ")
        for i, v in enumerate(y_train_reduced):
            f.write(f"{int(v)}")
            if i < len(y_train_reduced) - 1:
                f.write(", ")
            if (i + 1) % 20 == 0:
                f.write("\n    ")
        f.write("\n};\n\n")
        f.write("static const float scaler_mean[] = {\n    ")
        for i, v in enumerate(scaler_mean):
            f.write(f"{v:.6f}")
            if i < len(scaler_mean) - 1:
                f.write(", ")
        f.write("\n};\n\n")
        f.write("static const float scaler_scale[] = {\n    ")
        for i, v in enumerate(scaler_scale):
            f.write(f"{v:.6f}")
            if i < len(scaler_scale) - 1:
                f.write(", ")
        f.write("\n};\n\n")
        f.write("#endif // MODEL_DATA_H\n")

def load_dataset(filepath):
    "Função para carregar o dataset e garantir que não haja linhas com dados ausentes."
    if not os.path.exists(filepath):
        raise FileNotFoundError(f"O arquivo {filepath} não foi encontrado.")
    print(f"Carregando o dataset completo...")
    df = pd.read_csv(filepath)
    df.columns = df.columns.str.strip()
    last_valid_index = df.dropna(how='any').index[-1]
    df = df.loc[:last_valid_index].reset_index(drop=True)
    print(f"Usando até a linha {last_valid_index + 1} com dados completos.")
    return df

def preprocess_data(df):
    "Função para pré-processar os dados"
    print("Preprocessando os dados...")
    required_columns = ['Air temperature (C)', 'Air humidity (%)', 'Soil Moisture', 'Status']
    missing_columns = [col for col in required_columns if col not in df.columns]
    if missing_columns:
        raise KeyError(f"Colunas ausentes no dataset: {missing_columns}")
    df = df[required_columns].copy()
    numeric_columns = ['Air temperature (C)', 'Air humidity (%)', 'Soil Moisture']
    df[numeric_columns] = df[numeric_columns].fillna(df[numeric_columns].mean())
    df['Status'] = df['Status'].apply(lambda x: 1 if x == 'ON' else 0)
    X = df[numeric_columns].values
    y = df['Status'].values.astype(int)
    return X, y

def feature_engineering(X):
    "Função para aplicar engenharia de recursos nos dados"
    print("Aplicando engenharia de recursos...")
    temp = X[:, 0:1]
    hum = X[:, 1:2]
    soil = X[:, 2:3]
    features = [
        temp, hum, soil,
        temp * hum,
        temp * soil,
        hum * soil,
        temp ** 2,
        hum ** 2,
        soil ** 2,
        np.log1p(np.abs(temp)),
        np.log1p(np.abs(hum)),
        np.log1p(np.abs(soil)),
        (temp - hum),
        (temp - soil),
        (hum - soil)
    ]
    X_new = np.hstack(features)
    return X_new

def optimize_hyperparameters(X_train, y_train):
    "Função para otimizar os hiperparâmetros do modelo XGBoost usando GridSearchCV."
    print("Otimizando hiperparâmetros para XGBoost...")
    param_grid_xgb = {
        "n_estimators": [100, 200, 500],
        "learning_rate": [0.01, 0.05, 0.1, 0.2],
        "max_depth": [3, 6, 10],
        "subsample": [0.8, 1.0],
        "colsample_bytree": [0.8, 1.0],
        "gamma": [0, 1, 5],
        "scale_pos_weight": [1, 2, 5]
    }
    xgb = XGBClassifier(random_state=RANDOM_STATE, eval_metric="logloss")
    grid_xgb = GridSearchCV(xgb, param_grid_xgb, scoring="accuracy", cv=CV_FOLDS, n_jobs=-1)
    grid_xgb.fit(X_train, y_train)
    print(f"Melhores parâmetros XGB: {grid_xgb.best_params_} | Score: {grid_xgb.best_score_:.4f}")
    return grid_xgb.best_estimator_

def evaluate_model(X_test, y_test, model):
    "Função para avaliar o modelo treinado."
    print("Avaliando o modelo...")
    y_pred = model.predict(X_test)
    accuracy = accuracy_score(y_test, y_pred)
    print(f"Acurácia: {accuracy:.2%}")
    print("Matriz de Confusão:")
    print(confusion_matrix(y_test, y_pred))
    print("\nRelatório de Classificação:")
    print(classification_report(y_test, y_pred, zero_division=0))

def reduce_training_data(X_train, y_train, n_clusters=200):
    "Função para reduzir os dados de treinamento usando KMeans."
    from sklearn.cluster import KMeans
    print(f"Reduzindo os dados de treinamento para {n_clusters} clusters para exportação...")
    kmeans = KMeans(n_clusters=n_clusters, random_state=RANDOM_STATE)
    kmeans.fit(X_train)
    X_reduced = kmeans.cluster_centers_
    y_reduced = []
    for center in X_reduced:
        distances = np.linalg.norm(X_train - center, axis=1)
        closest_points = np.argsort(distances)[:5]
        labels = y_train[closest_points]
        y_reduced.append(np.bincount(labels).argmax())
    return X_reduced, np.array(y_reduced)

def main():
    df = load_dataset(DATASET_PATH)
    X, y = preprocess_data(df)

    print("Dividindo os dados em treino e teste...")
    X_train, X_test, y_train, y_test = train_test_split(
        X, y, test_size=TEST_SIZE, random_state=RANDOM_STATE, stratify=y
    )

    print("Padronizando os dados...")
    scaler = StandardScaler()
    X_train = scaler.fit_transform(X_train)
    X_test = scaler.transform(X_test)

    X_train = feature_engineering(X_train)
    X_test = feature_engineering(X_test)

    # Treina e seleciona o melhor modelo
    model = optimize_hyperparameters(X_train, y_train)

    # Avalia o modelo
    evaluate_model(X_test, y_test, model)

    # Reduz e exporta os dados para uso embarcado
    X_train_reduced, y_train_reduced = reduce_training_data(X_train, y_train, n_clusters=200)
    save_model_to_header(X_train_reduced, y_train_reduced, scaler.mean_, scaler.scale_, filename="model_data.h")

    print("Processo concluído com sucesso!")

if __name__ == "__main__":
    main()
