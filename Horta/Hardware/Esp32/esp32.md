# Documentação do ESP32

## Introdução
O **ESP32** é um microcontrolador avançado amplamente utilizado em projetos de IoT devido à sua capacidade de integração com Wi-Fi, Bluetooth e diversos periféricos. Ele foi escolhido para este projeto pela sua flexibilidade, poder de processamento e baixo consumo de energia.

## Objetivo
Esta pasta contém os arquivos relacionados ao desenvolvimento e configuração do ESP32 para o projeto. O objetivo principal é integrar o ESP32 ao sistema de controle e monitoramento da horta automatizada.

## Conteúdo da Pasta
Abaixo está uma breve descrição dos arquivos presentes nesta pasta:

### `esp32_sensor.cpp`
Este arquivo é responsável por implementar o controle e leitura dos sensores utilizados no sistema de irrigação automatizado. Ele também gerencia a comunicação sem fio via **ESP-NOW** para enviar os dados dos sensores para outros dispositivos ESP32.  
**Sensores implementados**:
- **DHT11**: Medição de temperatura e umidade do ar.
- **FC-28**: Monitoramento da umidade do solo.
- **FC-37**: Detecção de chuva.

### `esp32IA_sensor.cpp`
Este arquivo integra um modelo de **KNN** treinado que utiliza os dados dos sensores para prever a necessidade de irrigação. Ele realiza a padronização dos dados, cálculo de similaridade com base em distância euclidiana e predição das classes para controle de irrigação.  
**Sensores utilizados**:
- **DHT11**: Temperatura e umidade do ar.
- **FC-28**: Umidade do solo.
- **FC-37**: Sensor de chuva.

## Observações
- Certifique-se de que os sensores estejam devidamente conectados e configurados nos pinos especificados no código.
- O modelo de **KNN** implementado no arquivo `esp32IA_sensor.cpp` é uma solução baseada em aprendizado de máquina e requer um conjunto de dados pré-treinados para funcionar corretamente.
- As configurações de comunicação via **ESP-NOW** no arquivo `esp32_sensor.cpp` assumem que há um receptor ESP32 configurado com o endereço MAC especificado no código.

