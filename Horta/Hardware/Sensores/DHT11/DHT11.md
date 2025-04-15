# Sensor DHT11

O sensor DHT11 é amplamente utilizado para medir temperatura e umidade em projetos de eletrônica e automação. Ele é conhecido por sua simplicidade e custo acessível.

![Descrição da Imagem](caminho/para/imagem.png)

---

## Pinagem do DHT11

| Pino | Função          | Descrição                     |
|------|-----------------|-------------------------------|
| 1    | VCC             | Alimentação (3.3V ou 5V)     |
| 2    | Data            | Dados do sensor              |
| 3    | Não Conectado   | Não utilizado                |
| 4    | GND             | Terra (Ground)               |

---

## Recomendações

- **Uso de resistor**: Para comunicação estável, recomenda-se o uso de um resistor pull-up de 5kΩ-10kΩ entre o pino de dados e o VCC.
- **Capacitor**: Em ambientes ruidosos, você pode usar um capacitor de desacoplamento (100nF) entre o VCC e o GND.

---

## Exemplo de Código (Arduino)

Segue um exemplo de código para leitura de dados do DHT11 usando Arduino:

```cpp
#include <DHT.h> // Biblioteca Necessária

// Definir o pino do sensor e o tipo
#define DHTPIN 2 // Pino de dados conectado ao pino digital 2
#define DHTTYPE DHT11 // Definir o modelo do sensor (DHT11)

DHT dht(DHTPIN, DHTTYPE);

void setup() {
  Serial.begin(9600);
  Serial.println("Iniciando leitura do DHT11");
  dht.begin();
}

void loop() {
  float temperatura = dht.readTemperature();
  float umidade = dht.readHumidity();

  if (isnan(temperatura) || isnan(umidade)) {
    Serial.println("Falha ao ler os dados do sensor!");
    return;
  }

  Serial.print("Temperatura: ");
  Serial.print(temperatura);
  Serial.println(" °C");
  
  Serial.print("Umidade: ");
  Serial.print(umidade);
  Serial.println(" %");
  
  delay(1000); // Intervalo de 1 segundos entre leituras
}
