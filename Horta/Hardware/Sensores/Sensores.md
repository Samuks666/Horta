# 🔍 Sensores - Sistema de Irrigação Inteligente

Esta seção contém a documentação completa de todos os sensores utilizados no sistema de irrigação inteligente, incluindo especificações técnicas, códigos de exemplo e aplicações específicas.

## 📋 Índice de Sensores

### 🌡️ Sensores Ambientais
- **[DHT11](DHT11/DHT11.md)** - Temperatura e Umidade do Ar
- **[BMP280](BMP280/BMP280.md)** - Pressão Atmosférica e Temperatura

### 🌱 Sensores de Solo e Água
- **[FC-28](FC-28/FC-28.md)** - Umidade do Solo
- **[FC-37](FC-37/FC-37.md)** - Detecção de Chuva

---

## 📊 Resumo dos Sensores

| Sensor | Função Principal | Interface | Precisão | Consumo |
|--------|------------------|-----------|----------|---------|
| **DHT11** | Temp/Umidade Ar | Digital | ±2°C / ±5% | 2.5mA |
| **FC-28** | Umidade Solo | Analógica | ±3% | <20mA |
| **FC-37** | Detecção Chuva | Analógica/Digital | ±5% | <20mA |
| **BMP280** | Pressão/Temp | I2C | ±1 hPa / ±1°C | <1mA |

---

## 🔧 Pinagem no ESP32

| Sensor | Pino ESP32 | Função | Tipo |
|--------|------------|--------|------|
| DHT11 | GPIO 4 | Dados | Digital |
| FC-28 | GPIO 36 | Umidade Solo | ADC |
| FC-37 | GPIO 35 | Intensidade Chuva | ADC |
| FC-37 | GPIO 2 | Detecção Chuva | Digital |
| BMP280 | GPIO 21 | SDA (I2C) | I2C |
| BMP280 | GPIO 22 | SCL (I2C) | I2C |

---

## 📖 Documentações Detalhadas

### 🌡️ [Sensor DHT11](DHT11/DHT11.md)
**Temperatura e Umidade do Ar**
- Mede temperatura ambiente (0-50°C) e umidade relativa (20-90%)
- Interface digital com protocolo 1-Wire
- Ideal para monitoramento das condições climáticas
- **Aplicação**: Determinar condições ideais para irrigação

### 🌱 [Sensor FC-28](FC-28/FC-28.md)
**Umidade do Solo**
- Detecta nível de umidade no solo por condutividade elétrica
- Saída analógica e digital configurável
- Faixa de medição: 0-100% de umidade
- **Aplicação**: Principal sensor para decisão de irrigação

### 🌧️ [Sensor FC-37](FC-37/FC-37.md)
**Detecção de Chuva**
- Detecta presença e intensidade de água/chuva
- Dupla saída: analógica (intensidade) e digital (presença)
- Resposta rápida (<1 segundo)
- **Aplicação**: Suspender irrigação durante precipitações

### 🌤️ [Sensor BMP280](BMP280/BMP280.md)
**Pressão Atmosférica**
- Mede pressão (300-1100 hPa), temperatura e altitude
- Interface I2C com alta precisão
- Baixíssimo consumo de energia
- **Aplicação**: Previsão climática para otimizar irrigação

---

## 🔄 Integração dos Sensores

### Fluxo de Dados
```
[Sensores] → [ESP32] → [Processamento] → [Decisão IA] → [Irrigação]
     ↓           ↓            ↓             ↓           ↓
[Leituras]  [ADC/Digital] [Normalização] [KNN]    [Bomba/Válvula]
```

### Frequência de Leitura
- **DHT11**: A cada 2 segundos (limitação do sensor)
- **FC-28**: Contínua (analógica)
- **FC-37**: Contínua (analógica)
- **BMP280**: A cada 500ms (configurável)

---

## ⚙️ Configuração e Calibração

### Calibração Recomendada

#### FC-28 (Umidade do Solo)
```cpp
// Valores de calibração (ajustar conforme solo)
const int SOIL_DRY = 4095;    // Solo seco
const int SOIL_WET = 1500;    // Solo saturado
```

#### FC-37 (Detecção de Chuva)
```cpp
// Valores de calibração
const int RAIN_DRY = 4095;    // Sem água
const int RAIN_WET = 1000;    // Com água
```

### Manutenção dos Sensores
- **DHT11**: Limpar periodicamente, evitar condensação
- **FC-28**: Limpeza mensal dos contatos, proteção contra corrosão
- **FC-37**: Limpeza regular, verificar oxidação
- **BMP280**: Sem manutenção específica necessária

---

## 🎯 Aplicações no Sistema

### Algoritmo de Decisão de Irrigação

#### Modo Automático (IA)
```cpp
float features[3] = {temperatura, umidadeAr, umidadeSolo};
int decisao = knn_predict(features);
// 1 = Irrigar, 0 = Não irrigar
```

#### Modo Manual (Regras)
```cpp
if (umidadeSolo < 30 && !chuva && temperatura > 20) {
    irrigar = true;
} else {
    irrigar = false;
}
```

### Condições de Segurança
1. **Não irrigar se**: Detecção de chuva ativa
2. **Parar irrigação se**: Umidade do solo > 80%
3. **Alerta se**: Temperatura < 5°C ou > 40°C
4. **Verificar**: Pressão atmosférica para prever tempo

---

## 📊 Exemplo de Leituras

### Dados Típicos em Operação
```
=== Leituras dos Sensores ===
DHT11:
  Temperatura: 25.3°C
  Umidade Ar: 58.0%
  
FC-28:
  Valor Bruto: 2850
  Umidade Solo: 45%
  
FC-37:
  Intensidade: 0%
  Status: SECO
  
BMP280:
  Pressão: 1013.25 hPa
  Altitude: 45.2m
========================
```

---

## 🔗 Links Rápidos

- 🌡️ **[DHT11](DHT11/DHT11.md)** - Temperatura e Umidade do Ar
- 🌱 **[FC-28](FC-28/FC-28.md)** - Umidade do Solo  
- 🌧️ **[FC-37](FC-37/FC-37.md)** - Detecção de Chuva
- 🌤️ **[BMP280](BMP280/BMP280.md)** - Pressão Atmosférica
- 🔙 **[Hardware Geral](../Hardware.md)** - Voltar para documentação do hardware
