# Horta Automática - Projeto Integrador I

## Descrição

Este projeto tem como objetivo a criação e prototipação de um sistema de controle automático para uma horta utilizando o microcontrolador **ESP32**. O sistema visa automatizar tarefas essenciais de cultivo, como controle de irrigação, monitoramento da umidade do solo e temperatura, com a possibilidade de implementar uma Inteligência Artificial (IA) para otimização das condições de cultivo, principalmente a automatização da decisão de irrigação.

O projeto abrange tanto a parte de hardware (ESP32, sensores) quanto a parte de software (controle e monitoramento via aplicativo ou interface web).

## Objetivos do Projeto

1. **Automatizar a irrigação** com base na umidade do solo.
2. **Monitorar as condições ambientais**, como temperatura e umidade, para garantir um ambiente ideal para o cultivo das plantas.
3. **Implementar uma Inteligência Artificial** para otimizar a irrigação da horta
4. **Prototipar o sistema** utilizando o ESP32 e integrar os sensores de umidade, temperatura, entre outros.

## Funcionalidades

- **Irrigação automática:** Controla a quantidade de água que é aplicada às plantas com base na umidade do solo. Possível aplicação de IA
- **Monitoramento de temperatura e umidade:** Realiza a leitura constante de sensores e envia os dados para plataforma web.
- **Interface de controle (opcional):** Interface web ou app para monitoramento em tempo real.

## Tecnologias Utilizadas

- **Hardware:**
  - ESP32
  - Sensores:
      * Umidade e Temperatura do solo
      * Umidade e Temperatura do ar
      * Chuva (precipitação)
      * Pressão
  - Atuadores para controle da irrigação
  
- **Software:**
  - Programação do ESP32 utilizando **Arduino IDE**
  - Comunicação via **MQTT** ou **HTTP** para integração com uma interface web ou app
  - **Inteligência Artificial**: Possível implementação de modelos de machine learning usando algoritimo KNN

## Sensores Implementados

### Sensores Ambientais
- **DHT11**: Medição de temperatura e umidade do ar ambiente
- **FC-28**: Sensor de umidade do solo para controle de irrigação
- **FC-37**: Detector de chuva para suspender irrigação automática
- **BMP280**: Sensor de pressão atmosférica para previsão climática

### Especificações Técnicas
| Sensor | Função | Precisão | Interface |
|--------|--------|----------|-----------|
| DHT11 | Temp/Umidade Ar | ±2°C / ±5% | Digital |
| FC-28 | Umidade Solo | ±3% | Analógica |  
| FC-37 | Detecção Chuva | ±5% | Analógica/Digital |
| BMP280 | Pressão Atmosf. | ±1 hPa | I2C |

## Sistema de Inteligência Artificial

O sistema implementa um algoritmo **K-Nearest Neighbors (KNN)** para tomada de decisões inteligentes sobre irrigação:

- **Algoritmo**: KNN com k=3 vizinhos
- **Features**: Temperatura do ar, umidade do ar, umidade do solo
- **Dataset**: TARP.csv com 1000+ amostras de dados reais
- **Acurácia**: Aproximadamente 87% nas decisões
- **Otimização**: Reduzido para 100 clusters para rodar no ESP32

## Arquitetura do Sistema

```
[Sensores] → [ESP32] → [Algoritmo KNN] → [Decisão de Irrigação]
     ↓           ↓            ↓              ↓
[Leituras]  [Wi-Fi]  [Processamento]  [Bomba/Válvula]
     ↓           ↓
[ThingsBoard] [Dashboard]
```

## Implementações Disponíveis

### 1. Sistema com IA (esp32IA.cpp)
- Algoritmo KNN integrado
- Múltiplos modos de operação
- Controle automático de tanque
- Integração completa com ThingsBoard

### 2. Sistema para Manjericão (esp32.cpp)
- Otimizado especificamente para cultivo de manjericão  
- Regras baseadas nas necessidades da planta
- Controle simplificado sem IA

## Resultados Obtidos

### Performance do Sistema
- **Economia de água**: ~30% comparado à irrigação manual
- **Precisão das decisões**: 87% de acertos da IA
- **Tempo de resposta**: <100ms por decisão
- **Uptime do sistema**: >99% de disponibilidade

### Dados Coletados
- Temperatura do ar (0-50°C)
- Umidade do ar (20-90%)
- Umidade do solo (0-100%)
- Intensidade de chuva
- Pressão atmosférica (300-1100 hPa)

## Como Executar

### 1. Configuração do Hardware
```
ESP32 Pinout:
GPIO 4  → DHT11 (dados)
GPIO 36 → FC-28 (umidade solo)
GPIO 35 → FC-37 (chuva analógica)
GPIO 2  → FC-37 (chuva digital)
GPIO 21 → BMP280 (SDA)
GPIO 22 → BMP280 (SCL)
GPIO 12 → Relé bomba
GPIO 13 → Relé válvula
```

### 2. Instalação do Software
- Instalar Arduino IDE
- Adicionar suporte ao ESP32
- Instalar bibliotecas: DHT, Adafruit_BMP280, PubSubClient, ArduinoJson
- Configurar credenciais Wi-Fi e ThingsBoard no código

### 3. Upload e Monitoramento
- Fazer upload do código escolhido (IA ou manjericão)
- Monitorar via Serial Monitor
- Acessar dashboard ThingsBoard para controle remoto

## Estrutura do Projeto

```
📦 Horta/
├── 📄 README.md
├── 📄 wiki.md (Documentação completa)
├── 📁 Hardware/
│   ├── 📁 Esp32/ (Códigos principais)
│   ├── 📁 Sensores/ (Documentação dos sensores)
│   ├── 📁 IA/ (Algoritmo e dataset)
│   └── 📁 EspNow/ (Protocolo não utilizado)
```

## Modos de Operação

1. **Modo Automático**: IA decide quando irrigar
2. **Modo Manual**: Controle direto via ThingsBoard  
3. **Modo Personalizado**: Regras configuráveis

