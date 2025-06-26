# Microcontrolador ESP32

O ESP32 é um microcontrolador avançado amplamente utilizado em projetos de IoT devido à sua capacidade de integração com Wi-Fi, Bluetooth e diversos periféricos.

<img src="ESP32.jpg" alt="Microcontrolador ESP32" width="300">

# Dados Técnicos do ESP32

| Característica        | Descrição                              |
|-----------------------|----------------------------------------|
| Processador           | Dual-core Xtensa LX6 32-bit          |
| Frequência            | Até 240 MHz                           |
| Memória Flash         | 4MB (padrão)                          |
| RAM                   | 520KB SRAM                            |
| Wi-Fi                 | 802.11 b/g/n (2.4 GHz)              |
| Bluetooth             | v4.2 BR/EDR e BLE                     |
| GPIO                  | 34 pinos configuráveis                |
| ADC                   | 2x 12-bit SAR ADC                     |
| DAC                   | 2x 8-bit                              |
| Tensão Operação       | 2.2V a 3.6V                          |
| Consumo               | 240mA (ativo), 10μA (deep sleep)     |

## Pinagem do ESP32 (DevKit)

### Pinos de Alimentação
| Pino | Função          | Descrição                             |
|------|-----------------|---------------------------------------|
| 3V3  | VCC             | Saída regulada 3.3V                  |
| 5V   | VIN             | Entrada 5V (via USB)                 |
| GND  | Terra           | Referência 0V                        |
| EN   | Enable          | Reset do microcontrolador             |

### Pinos GPIO Utilizados no Projeto
| Pino GPIO | Função no Projeto    | Sensor/Dispositivo        | Tipo      |
|-----------|---------------------|---------------------------|-----------|
| 4         | Dados DHT11         | DHT11                     | Digital   |
| 36        | Leitura Solo        | FC-28 (Analógico)         | ADC       |
| 35        | Intensidade Chuva   | FC-37 (Analógico)         | ADC       |
| 2         | Detecção Chuva      | FC-37 (Digital)           | Digital   |
| 21        | I2C SDA             | BMP280                    | I2C       |
| 22        | I2C SCL             | BMP280                    | I2C       |
| 14        | Sensor Nível 1      | Nível Baixo Tanque        | Digital   |
| 27        | Sensor Nível 2      | Nível Alto Tanque         | Digital   |
| 12        | Bomba Irrigação     | Relé Bomba                | Digital   |
| 13        | Válvula Solenoide   | Relé Válvula              | Digital   |
| 32        | Bomba Abastecimento | Relé Bomba Tanque         | Digital   |

## Componentes Necessários
- Microcontrolador ESP32 DevKit
- Cabo USB Micro-B para programação
- Fonte de alimentação 5V/2A
- Resistores pull-up (opcional para alguns sensores)
- Capacitores de desacoplamento (100nF)

## Recomendações

- **Alimentação**: Use fonte estável de 5V. Evite alimentar apenas via 3.3V com múltiplos sensores.
- **Reset**: O pino EN permite reset por hardware. Útil para debugging.
- **ADC**: Os pinos 36 e 39 são apenas de entrada (input-only).
- **I2C**: Use resistores pull-up de 4.7kΩ nos pinos SDA e SCL.
- **PWM**: Qualquer GPIO pode gerar PWM por software.

## Arquivos de Implementação

### 1. Sistema Completo com IA - `esp32IA.cpp`
**[Código Completo](esp32IA.cpp)**

Sistema completo de irrigação inteligente com as seguintes funcionalidades:

**Recursos:**
- Algoritmo KNN para decisões de irrigação
- Integração com ThingsBoard (IoT)
- Controle automático de tanque de água
- Múltiplos modos de operação (Auto/Manual/Personalizado)
- Comandos RPC via Wi-Fi

**Sensores Integrados:**
- DHT11: Temperatura e umidade do ar
- FC-28: Umidade do solo
- FC-37: Detecção de chuva
- BMP280: Pressão atmosférica
- Sensores de nível do tanque

### 2. Sistema para Manjericão - `esp32_manjericao.cpp`
Sistema otimizado especificamente para cultivo de manjericão:

**Características:**
- Regras baseadas nas necessidades do manjericão
- Umidade do solo ideal: 60-85%
- Temperatura ideal: 18-30°C
- Controle simplificado sem IA

## Exemplo de Código Básico

```cpp
#include <WiFi.h>
#include <DHT.h>

// Configurações Wi-Fi
const char* ssid = "SUA_REDE_WIFI";
const char* password = "SUA_SENHA_WIFI";

// Configuração DHT11
#define DHTPIN 4
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

void setup() {
  Serial.begin(115200);
  Serial.println("Iniciando ESP32...");
  
  // Inicializar DHT11
  dht.begin();
  
  // Conectar Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("\nWi-Fi conectado!");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
  
  // Configurar pinos GPIO
  pinMode(12, OUTPUT); // Bomba
  pinMode(13, OUTPUT); // Válvula
  
  digitalWrite(12, LOW);
  digitalWrite(13, LOW);
}

void loop() {
  // Ler sensores
  float temperatura = dht.readTemperature();
  float umidade = dht.readHumidity();
  int umidadeSolo = analogRead(36);
  
  // Verificar se as leituras são válidas
  if (isnan(temperatura) || isnan(umidade)) {
    Serial.println("Erro ao ler DHT11!");
    delay(5000);
    return;
  }
  
  // Converter umidade do solo para percentual
  int umidadeSoloPercent = map(umidadeSolo, 0, 4095, 100, 0);
  
  // Mostrar dados
  Serial.printf("Temp: %.1f°C | Umid.Ar: %.1f%% | Umid.Solo: %d%%\n", 
                temperatura, umidade, umidadeSoloPercent);
  
  // Lógica simples de irrigação
  if (umidadeSoloPercent < 30) {
    Serial.println("Solo seco - Ligando irrigação");
    digitalWrite(12, HIGH); // Liga bomba
    digitalWrite(13, HIGH); // Abre válvula
  } else if (umidadeSoloPercent > 70) {
    Serial.println("Solo úmido - Desligando irrigação");
    digitalWrite(12, LOW);  // Desliga bomba
    digitalWrite(13, LOW);  // Fecha válvula
  }
  
  delay(10000); // 10 segundos
}
```

# Saída no Terminal

```
Iniciando ESP32...
..........
Wi-Fi conectado!
IP: 192.168.1.100
Temp: 25.3°C | Umid.Ar: 58.0% | Umid.Solo: 25%
Solo seco - Ligando irrigação
Temp: 25.4°C | Umid.Ar: 58.5% | Umid.Solo: 35%
Temp: 25.2°C | Umid.Ar: 59.0% | Umid.Solo: 45%
Temp: 25.1°C | Umid.Ar: 60.0% | Umid.Solo: 65%
Temp: 25.0°C | Umid.Ar: 61.0% | Umid.Solo: 75%
Solo úmido - Desligando irrigação
```

## Configuração do Ambiente de Desenvolvimento

### 1. Arduino IDE
```bash
# Instalar ESP32 no Arduino IDE
# File -> Preferences -> Additional Board Manager URLs:
https://dl.espressif.com/dl/package_esp32_index.json

# Tools -> Board -> Boards Manager
# Buscar "ESP32" e instalar
```

### 2. Bibliotecas Necessárias
```cpp
// Instalar via Library Manager:
#include <WiFi.h>          // ESP32 Core (já incluída)
#include <DHT.h>           // DHT sensor library
#include <Wire.h>          // I2C (já incluída)
#include <Adafruit_BMP280.h> // Adafruit BMP280 Library
#include <PubSubClient.h>  // MQTT para ThingsBoard
#include <ArduinoJson.h>   // JSON para comunicação
```

### 3. Configurações da IDE
- **Board**: ESP32 Dev Module
- **Upload Speed**: 921600
- **CPU Frequency**: 240MHz (WiFi/BT)
- **Flash Size**: 4MB
- **Port**: Selecionar porta COM correta

## Características de Desempenho

### Consumo de Energia
| Modo              | Consumo Típico | Duração da Bateria* |
|-------------------|----------------|---------------------|
| Ativo (Wi-Fi On)  | 240mA         | ~4 horas           |
| Modem Sleep       | 20mA          | ~50 horas          |
| Light Sleep       | 0.8mA         | ~52 dias           |
| Deep Sleep        | 10μA          | ~11 anos           |

*Com bateria de 1000mAh

### Conectividade Wi-Fi
- **Padrões**: 802.11 b/g/n
- **Frequência**: 2.4 GHz
- **Alcance**: Até 100m (campo aberto)
- **Modos**: Station, Access Point, Station+AP
- **Segurança**: WPA/WPA2/WPA3

## Aplicações no Sistema de Irrigação

- **Controle Central**: Gerencia todos os sensores e atuadores
- **Conectividade IoT**: Comunicação com ThingsBoard via Wi-Fi
- **Processamento Local**: Executa algoritmos de IA localmente
- **Controle Remoto**: Recebe comandos via internet
- **Monitoramento**: Coleta e transmite dados em tempo real
- **Economia de Energia**: Modos de baixo consumo quando inativo

## Resolução de Problemas

### Problemas Comuns
1. **ESP32 não conecta Wi-Fi**
   - Verificar SSID e senha
   - Verificar sinal Wi-Fi (mínimo -70dBm)
   - Reiniciar o roteador

2. **Upload falha**
   - Segurar botão BOOT durante upload
   - Verificar porta COM
   - Diminuir velocidade de upload

3. **Leituras de sensores incorretas**
   - Verificar conexões
   - Usar resistores pull-up quando necessário
   - Verificar alimentação dos sensores

4. **Alto consumo de energia**
   - Implementar modos de sleep
   - Desligar Wi-Fi quando não necessário
   - Otimizar frequência de leituras

## Links Úteis

- **[Documentação Oficial ESP32](https://docs.espressif.com/projects/esp32/en/latest/)**
- **[ESP32 Arduino Core](https://github.com/espressif/arduino-esp32)**
- **[Exemplos de Código](https://github.com/espressif/arduino-esp32/tree/master/libraries)**
- **[ESP32 Pinout Reference](https://randomnerdtutorials.com/esp32-pinout-reference-gpios/)**