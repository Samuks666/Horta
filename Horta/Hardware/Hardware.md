# Documentação do Hardware - Sistema de Irrigação Inteligente

Este documento apresenta um resumo completo de todos os componentes de hardware utilizados no sistema de irrigação inteligente, incluindo sensores, microcontrolador, sistema de IA e códigos de implementação.

## Índice

- [Microcontrolador](#microcontrolador)
- [Sensores](#sensores)
- [Sistema de Inteligência Artificial](#sistema-de-inteligência-artificial)
- [Esquema de Conexões](#esquema-de-conexões)
- [Especificações Técnicas](#especificações-técnicas)

---

## Microcontrolador
### ESP32 - Controlador Principal
**[Documentação Completa do ESP32](ESP32/esp32.md)**

O ESP32 é o microcontrolador principal do sistema, responsável por:
- Coleta de dados dos sensores
- Processamento das decisões de irrigação
- Comunicação Wi-Fi com ThingsBoard
- Controle das bombas e válvulas

**Características:**
- Processador dual-core 240MHz
- Wi-Fi e Bluetooth integrados
- 34 pinos GPIO configuráveis
- ADC de 12 bits para leitura analógica
- Suporte a protocolos I2C, SPI, UART

---

## Sensores

### 1. Sensor de Temperatura e Umidade do Ar - DHT11
📄 **[Documentação Completa do DHT11](Sensores/DHT11/DHT11.md)**

- **Função**: Mede temperatura ambiente e umidade relativa do ar
- **Faixa de Temperatura**: 0°C a 50°C (±2°C)
- **Faixa de Umidade**: 20% a 90% (±5%)
- **Interface**: Digital (1-Wire)
- **Pino Usado**: GPIO 4

### 2. Sensor de Umidade do Solo - FC-28
📄 **[Documentação Completa do FC-28](Sensores/FC-28/FC-28.md)**

- **Função**: Mede o nível de umidade no solo
- **Método**: Condutividade elétrica
- **Faixa de Medição**: 0% a 100% de umidade
- **Saídas**: Analógica (A0) e Digital (D0)
- **Pino Usado**: GPIO 36 (analógico)

### 3. Sensor de Chuva - FC-37
📄 **[Documentação Completa do FC-37](Sensores/FC-37/FC-37.md)**

- **Função**: Detecta presença de água/chuva
- **Método**: Condutividade elétrica
- **Saídas**: Analógica e Digital
- **Aplicação**: Suspender irrigação durante chuva
- **Pinos Usados**: GPIO 35 (analógico), GPIO 2 (digital)

### 4. Sensor de Pressão Atmosférica - BMP280
📄 **[Documentação Completa do BMP280](Sensores/BMP280/BMP280.md)**

- **Função**: Mede pressão atmosférica, temperatura e altitude
- **Faixa de Pressão**: 300 a 1100 hPa (±1 hPa)
- **Interface**: I2C (endereços 0x76 ou 0x77)
- **Aplicação**: Previsão climática para irrigação
- **Pinos Usados**: GPIO 21 (SDA), GPIO 22 (SCL)

---

## Sistema de Inteligência Artificial

### Sistema KNN para Decisões de Irrigação
📄 **[Documentação Completa da IA](IA/IA.md)**

- **Algoritmo**: K-Nearest Neighbors (KNN)
- **Features**: Temperatura, Umidade do Ar, Umidade do Solo
- **Dataset**: TARP.csv (1000+ amostras)
- **Acurácia**: ~87% nas decisões
- **Otimização**: Reduzido para 100 clusters (ESP32)

**Arquivos da IA:**
- `IA_simple.py` - Script de treinamento
- `model_data.h` - Dados do modelo em C++
- `TARP.csv` - Dataset para treinamento

---

## Esquema de Conexões

### Pinos Utilizados no ESP32

| Componente           | Pino ESP32    | Função              | Tipo      |
|---------------------|---------------|---------------------|-----------|
| DHT11               | GPIO 4        | Dados               | Digital   |
| FC-28 (Solo)        | GPIO 35       | Leitura Analógica   | ADC       |
| FC-37 (Chuva)       | GPIO 34       | Leitura Analógica   | ADC       |
| BMP280 (SDA)        | GPIO 21       | Dados I2C           | I2C       |
| BMP280 (SCL)        | GPIO 22       | Clock I2C           | I2C       |
| Sensor Nível 1      | GPIO 14       | Nível Baixo         | Digital   |
| Sensor Nível 2      | GPIO 27       | Nível Alto          | Digital   |
| Bomba Irrigação     | GPIO 26       | Controle            | Digital   |
| Válvula Solenoide   | GPIO 25       | Controle            | Digital   |

### Alimentação
- **ESP32**: 5V via USB ou 3.3V regulada
- **Sensores**: 3.3V ou 5V (compatíveis)
- **Bombas**: 12V com relés para isolamento

---

## Especificações Técnicas

### Consumo de Energia
| Componente          | Consumo Típico  | Observações                    |
|--------------------|-----------------|--------------------------------|
| ESP32              | 240mA (ativo)   | 10μA em deep sleep            |
| DHT11              | 2.5mA           | Durante leitura               |
| FC-28              | <20mA           | Contínuo                      |
| FC-37              | <20mA           | Contínuo                      |
| BMP280             | <1mA            | Baixo consumo                 |
| **Total Sistema**  | **~285mA**      | Em operação normal            |

### Características Operacionais
- **Temperatura Operação**: -10°C a 60°C
- **Umidade Operacional**: 10% a 90% (sem condensação)
- **Tensão de Alimentação**: 5V ±5%
- **Frequência Wi-Fi**: 2.4GHz (802.11 b/g/n)
- **Alcance Wi-Fi**: Até 100m (campo aberto)

### Precisão dos Sensores
| Sensor    | Parâmetro          | Precisão    | Resolução   |
|-----------|-------------------|-------------|-------------|
| DHT11     | Temperatura       | ±2°C        | 1°C         |
| DHT11     | Umidade do Ar     | ±5%         | 1%          |
| FC-28     | Umidade do Solo   | ±3%         | 0.1%        |
| FC-37     | Intensidade Chuva | ±5%         | 0.1%        |
| BMP280    | Pressão           | ±1 hPa      | 0.18 Pa     |
| BMP280    | Temperatura       | ±1°C        | 0.01°C      |

---

## Códigos de Implementação

### 1. Sistema Completo com IA
📄 **Arquivo**: `Esp32/esp32IA.cpp`
- Sistema completo com algoritmo KNN
- Integração com ThingsBoard
- Controle automático de tanque
- Múltiplos modos de operação

### 2. Sistema para Manjericão (Sem IA)
📄 **Arquivo**: `Esp32/esp32.cpp`
- Otimizado especificamente para manjericão
- Regras baseadas em parâmetros da planta
- Controle simplificado e eficiente

### 3. Códigos Individuais dos Sensores
Cada sensor possui código de exemplo individual em sua respectiva pasta de documentação.

---

## Recursos Adicionais

### ThingsBoard Integration
- **Dashboard**: Monitoramento em tempo real
- **Comandos RPC**: Controle remoto via Wi-Fi
- **Telemetria**: Envio de dados a cada 30 segundos
- **Alertas**: Notificações de status do sistema

### Funcionalidades Avançadas
- **Modo Manual**: Controle direto via ThingsBoard
- **Modo Automático**: Decisões baseadas em IA ou regras
- **Controle de Tanque**: Abastecimento automático
- **Prevenção de Encharcamento**: Múltiplas verificações
- **Detecção de Chuva**: Suspensão automática da irrigação

---

## Manutenção e Cuidados

### Manutenção Preventiva
- Limpar sensores FC-28 e FC-37 mensalmente
- Verificar conexões elétricas a cada 3 meses
- Calibrar sensores semestralmente
- Atualizar firmware quando necessário

### Resolução de Problemas
1. **Sensores com leituras incorretas**: Verificar conexões e calibração
2. **ESP32 não conecta Wi-Fi**: Verificar credenciais e sinal
3. **ThingsBoard sem dados**: Verificar token de acesso
4. **Irrigação não funciona**: Verificar bomba e válvulas

---

## Links Rápidos

- **[Sensor DHT11](Sensores/DHT11/DHT11.md)** - Temperatura e Umidade do Ar
- **[Sensor FC-28](Sensores/FC-28/FC-28.md)** - Umidade do Solo  
- **[Sensor FC-37](Sensores/FC-37/FC-37.md)** - Detecção de Chuva
- **[Sensor BMP280](Sensores/BMP280/BMP280.md)** - Pressão Atmosférica
- **[Esp32](ESP32/esp32.md)** - Microcontrolador
- **[Sistema de IA](IA/IA.md)** - Inteligência Artificial KNN

