# Sensor DHT11

O sensor DHT11 é utilizado para medir temperatura e umidade relativa do ar através de comunicação digital.

<img src="DHT11.jpg" alt="Sensor DHT11" width="300">

# Dados Técnicos do Sensor DHT11

| Característica        | Descrição                              |
|-----------------------|----------------------------------------|
| Tipo                  | Sensor digital de temperatura e umidade |
| Tensão de Operação    | 3.3V a 5.5V                           |
| Corrente de Operação  | 0.5mA a 2.5mA                         |
| Faixa de Temperatura  | 0°C a 50°C                            |
| Precisão Temperatura  | ±2°C                                  |
| Faixa de Umidade      | 20% a 90%                             |
| Precisão Umidade      | ±5%                                   |
| Tempo de Resposta     | 2 segundos                            |
| Interface             | 1-Wire (protocolo proprietário)       |

## Pinagem do DHT11

| Pino | Função          | Descrição                     |
|------|-----------------|-------------------------------|
| 1    | VCC             | Alimentação (3.3V ou 5V)     |
| 2    | Data            | Dados do sensor              |
| 3    | Não Conectado   | Não utilizado                |
| 4    | GND             | Terra (Ground)               |

## Componentes
- Módulo sensor DHT11
- Resistor pull-up 5kΩ-10kΩ (opcional, muitos módulos já incluem)
- Capacitor de desacoplamento 100nF (opcional)

## Recomendações

- **Resistor Pull-up**: Para comunicação estável, recomenda-se o uso de um resistor pull-up de 5kΩ-10kΩ entre o pino de dados e o VCC.
- **Capacitor**: Em ambientes ruidosos, use um capacitor de desacoplamento (100nF) entre o VCC e o GND.
- **Intervalo de Leitura**: Respeite o intervalo mínimo de 2 segundos entre leituras.
- **Posicionamento**: Instale em local ventilado, longe de fontes de calor direto.

## Exemplo de Código

Segue um exemplo de código para leitura de dados do DHT11 usando Arduino:

```cpp
#include <DHT.h> // Biblioteca Necessária

// Definir o pino do sensor e o tipo
#define DHTPIN 4 // Pino de dados conectado ao pino digital 4
#define DHTTYPE DHT11 // Definir o modelo do sensor (DHT11)

DHT dht(DHTPIN, DHTTYPE);

void setup() {
  Serial.begin(115200);
  Serial.println("Iniciando leitura do DHT11");
  dht.begin();
  
  // Aguardar estabilização
  delay(2000);
}

void loop() {
  // Ler temperatura e umidade
  float temperatura = dht.readTemperature();
  float umidade = dht.readHumidity();
  
  // Verificar se as leituras são válidas
  if (isnan(temperatura) || isnan(umidade)) {
    Serial.println("Falha ao ler os dados do sensor!");
    delay(2000);
    return;
  }
  
  // Calcular índice de calor (sensação térmica)
  float indiceCalor = dht.computeHeatIndex(temperatura, umidade, false);

  Serial.print("Temperatura: ");
  Serial.print(temperatura);
  Serial.println(" °C");
  
  Serial.print("Umidade: ");
  Serial.print(umidade);
  Serial.println(" %");
  
  Serial.print("Sensação Térmica: ");
  Serial.print(indiceCalor);
  Serial.println(" °C");
  
  // Classificação do ambiente
  if (temperatura < 18) {
    Serial.println("Ambiente: FRIO");
  } else if (temperatura > 30) {
    Serial.println("Ambiente: QUENTE");
  } else {
    Serial.println("Ambiente: AGRADÁVEL");
  }
  
  if (umidade < 40) {
    Serial.println("Umidade do Ar: BAIXA");
  } else if (umidade > 70) {
    Serial.println("Umidade do Ar: ALTA");
  } else {
    Serial.println("Umidade do Ar: IDEAL");
  }
  
  Serial.println("------------------------");
  
  delay(2000); // Intervalo de 2 segundos entre leituras
}
```

# Saída no Terminal

- Teste de 8 segundos total de 4 leituras

```
Iniciando leitura do DHT11
Temperatura: 25.60 °C
Umidade: 59.00 %
Sensação Térmica: 26.18 °C
Ambiente: AGRADÁVEL
Umidade do Ar: IDEAL
------------------------
Temperatura: 25.80 °C
Umidade: 60.00 %
Sensação Térmica: 26.42 °C
Ambiente: AGRADÁVEL
Umidade do Ar: IDEAL
------------------------
Temperatura: 26.00 °C
Umidade: 58.00 %
Sensação Térmica: 26.35 °C
Ambiente: AGRADÁVEL
Umidade do Ar: IDEAL
------------------------
Temperatura: 25.70 °C
Umidade: 59.50 %
Sensação Térmica: 26.25 °C
Ambiente: AGRADÁVEL
Umidade do Ar: IDEAL
------------------------
```

## Aplicações no Sistema de Irrigação

- **Monitoramento Ambiental**: Acompanhar condições climáticas em tempo real
- **Decisão de Irrigação**: Considerar umidade do ar para otimizar irrigação
- **Controle de Estufa**: Regular ventilação baseada na temperatura e umidade
- **Prevenção de Doenças**: Evitar condições favoráveis a fungos (alta umidade)
- **Economia de Água**: Ajustar irrigação conforme condições atmosféricas
- **Alerta de Condições Extremas**: Notificar sobre temperaturas ou umidade críticas