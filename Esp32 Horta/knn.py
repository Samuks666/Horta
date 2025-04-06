import pandas as pd
import numpy as np
from sklearn.model_selection import train_test_split
from sklearn.preprocessing import StandardScaler
from sklearn.neighbors import KNeighborsClassifier
import pickle

# Carregar o dataset
url = "TARP.csv"  # Substitua pelo caminho correto do seu arquivo CSV
df = pd.read_csv(url)

# Converter o status 'on' e 'off' para 1 e 0
df['Status'] = df['Status'].apply(lambda x: 1 if x == 'ON' else 0)

# Selecionar as colunas de características e o rótulo
X = df[['Soil Moisture', 'Temperature', 'Soil Humidity Time',
        'Air temperature (C)', 'Air humidity (%)', 'Pressure (KPa)', 'rainfall']].values
y = df['Status'].values  # Rótulo (1 = Regar, 0 = Não regar)

# Dividir os dados em conjunto de treino e teste
X_train, X_test, y_train, y_test = train_test_split(X, y, test_size=0.2, random_state=42)

# Padronizar os dados
scaler = StandardScaler()
X_train = scaler.fit_transform(X_train)
X_test = scaler.transform(X_test)

# Treinar o modelo KNN
knn = KNeighborsClassifier(n_neighbors=3)
knn.fit(X_train, y_train)

# Avaliar o modelo
accuracy = knn.score(X_test, y_test)
print(f'Acurácia do modelo: {accuracy * 100:.2f}%')

# Salvar o modelo e os dados de treino em arquivos binários
with open('knn_model.pkl', 'wb') as model_file:
    pickle.dump(knn, model_file)

with open('scaler.pkl', 'wb') as scaler_file:
    pickle.dump(scaler, scaler_file)

# Salvar os dados de treino em um arquivo binário
np.save('X_train.npy', X_train)
np.save('y_train.npy', y_train)

# Se necessário, você também pode salvar as previsões
predictions = knn.predict(X_test)
np.save('predictions.npy', predictions)
