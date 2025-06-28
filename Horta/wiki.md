# 🌱 Sistema de Irrigação Inteligente - Wiki

Bem-vindo à documentação completa do **Sistema de Irrigação Inteligente**! Este projeto combina sensores IoT, inteligência artificial e automação para criar um sistema de irrigação eficiente e inteligente.

---

## 📋 Índice Principal

### 📟 [Hardware](Hardware/Hardware.md)
Documentação completa de todos os componentes de hardware utilizados no sistema.

#### 🔧 Microcontrolador
- 📱 **[ESP32](Hardware/Esp32/esp32.md)** - Controlador principal do sistema
- 📡 **[ESP-NOW](Hardware/EspNow/espNOW.md)** - Protocolo de comunicação (não utilizado)

#### 🔍 Sensores
- 🌡️ **[DHT11](Hardware/Sensores/DHT11/DHT11.md)** - Temperatura e Umidade do Ar
- 🌱 **[FC-28](Hardware/Sensores/FC-28/FC-28.md)** - Umidade do Solo
- 🌧️ **[FC-37](Hardware/Sensores/FC-37/FC-37.md)** - Detecção de Chuva
- 🌤️ **[BMP280](Hardware/Sensores/BMP280/BMP280.md)** - Pressão Atmosférica

#### 🤖 Inteligência Artificial
- 🧠 **[Sistema de IA](Hardware/IA/IA.md)** - Algoritmo KNN para decisões inteligentes

---

## 🎯 Características do Sistema

### 🌟 Principais Funcionalidades
- **Irrigação Inteligente**: Decisões baseadas em IA ou regras específicas por planta
- **Monitoramento IoT**: Integração com ThingsBoard para controle remoto
- **Múltiplos Sensores**: Temperatura, umidade do ar/solo, chuva e pressão
- **Controle Automático**: Gerenciamento de tanque e bomba de água
- **Economia de Água**: Evita irrigação desnecessária

### 🔄 Modos de Operação
1. **Modo Automático**: IA decide quando irrigar
2. **Modo Manual**: Controle direto via ThingsBoard
3. **Modo Personalizado**: Regras específicas por tipo de planta

---

## 💻 Códigos de Implementação

### 📂 Arquivos Principais
- **`esp32IA.cpp`** - Sistema completo com inteligência artificial
- **`esp32.cpp`** - Sistema otimizado para manjericão (sem IA)
- **`IA_simple.py`** - Script de treinamento do modelo KNN
- **`model_data.h`** - Dados do modelo convertidos para C++
- **`TARP.csv`** - Dataset para treinamento da IA

---

## 🔗 Conectividade e IoT

### 📡 ThingsBoard Integration
- **Dashboard**: Monitoramento em tempo real
- **Comandos RPC**: Controle remoto via Wi-Fi
- **Telemetria**: Envio de dados a cada 30 segundos
- **Alertas**: Notificações de status do sistema

### 📊 Dados Coletados
- Temperatura do ar (°C)
- Umidade do ar (%)
- Umidade do solo (%)
- Intensidade da chuva
- Pressão atmosférica (hPa)
- Status da irrigação
- Nível do tanque de água

---

## ⚙️ Especificações Técnicas

### 🔌 Pinagem do ESP32
| Pino GPIO | Função              | Sensor/Dispositivo    |
|-----------|---------------------|-----------------------|
| 4         | Dados DHT11         | DHT11                 |
| 35        | Umidade Solo        | FC-28 (Analógico)     |
| 34        | Intensidade Chuva   | FC-37 (Analógico)     |
| 21        | I2C SDA             | BMP280                |
| 22        | I2C SCL             | BMP280                |
| 14        | Nível Baixo         | Sensor Nível 1        |
| 27        | Nível Alto          | Sensor Nível 2        |
| 12        | Bomba Irrigação     | Relé                  |
| 13        | Válvula Solenoide   | Relé                  |

### ⚡ Consumo de Energia
- **ESP32**: 240mA (ativo), 10μA (deep sleep)
- **Sensores**: ~45mA total
- **Sistema Completo**: ~285mA em operação

---

## 🌿 Aplicações por Tipo de Planta

### 🌿 Manjericão (Implementado)
- **Umidade do Solo**: 60-85%
- **Temperatura**: 18-30°C
- **Umidade do Ar**: 40-80%
- **Código**: `esp32.cpp`

### 🔮 Outras Plantas (IA Genérica)
- **Algoritmo**: KNN com 3 vizinhos
- **Features**: Temperatura, umidade ar/solo
- **Acurácia**: ~87%
- **Código**: `esp32IA.cpp`

---

## 🚀 Como Começar

### 1. Hardware Necessário
- 1x ESP32 DevKit
- 1x Sensor DHT11
- 1x Sensor FC-28 (umidade solo)
- 1x Sensor FC-37 (chuva)
- 1x Sensor BMP280 (pressão)
- Relés, bombas e cabos de conexão

### 2. Software Necessário
- Arduino IDE com suporte ESP32
- Bibliotecas: DHT, Adafruit_BMP280, PubSubClient, ArduinoJson
- Conta no ThingsBoard (demo.thingsboard.io)

### 3. Configuração
1. Conectar sensores conforme pinagem
2. Configurar credenciais Wi-Fi no código
3. Criar dispositivo no ThingsBoard
4. Fazer upload do código escolhido
5. Monitorar via dashboard

---

## 📈 Dados e Estatísticas

### 🎯 Performance do Sistema
- **Acurácia da IA**: 87.31%
- **Tempo de Resposta**: <100ms por decisão
- **Alcance Wi-Fi**: Até 100m
- **Intervalo de Telemetria**: 30 segundos
- **Capacidade do Dataset**: 1000+ amostras

### 💾 Uso de Memória
- **Modelo IA**: ~3KB RAM
- **Código Principal**: ~1.2MB Flash
- **Dados de Calibração**: ~500 bytes

---

## 🔧 Manutenção e Suporte

### 🛠️ Manutenção Preventiva
- Limpar sensores FC-28 e FC-37 mensalmente
- Verificar conexões elétricas trimestralmente
- Calibrar sensores semestralmente
- Atualizar firmware quando necessário

### ❓ Resolução de Problemas
1. **Sensores incorretos**: Verificar conexões e calibração
2. **Sem Wi-Fi**: Verificar credenciais e sinal
3. **ThingsBoard offline**: Verificar token de acesso
4. **Irrigação não funciona**: Verificar bomba e relés

---

## 📊 Status do Desenvolvimento

### ✅ Implementado
- [x] Sistema básico de sensores
- [x] Integração com ThingsBoard
- [x] Algoritmo KNN para IA
- [x] Controle automático de tanque
- [x] Modo específico para manjericão
- [x] Comandos RPC remotos

### 🔄 Em Desenvolvimento
- [ ] Suporte a múltiplas plantas
- [ ] Análise de dados históricos

### 🔮 Roadmap Futuro
- [ ] Expansão para múltiplas hortas
- [ ] Integração com sensores de pH
- [ ] Machine Learning avançado

---

## 📚 Recursos Adicionais

### 🔗 Links Úteis
- **[Arduino IDE](https://www.arduino.cc/en/software)**
- **[ThingsBoard](https://thingsboard.io/)**
- **[ESP32 Documentation](https://docs.espressif.com/projects/esp32/)**
- **[Dataset TARP](https://www.kaggle.com/datasets/nelakurthisudheer/dataset-for-predicting-watering-the-plants)**

