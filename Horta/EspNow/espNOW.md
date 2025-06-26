# Protocolo ESP-NOW

O ESP-NOW é um protocolo de comunicação sem fio da Espressif para comunicação direta entre dispositivos ESP32/ESP8266 sem roteador Wi-Fi.

# Dados Técnicos do ESP-NOW

| Característica        | Descrição                              |
|-----------------------|----------------------------------------|
| Protocolo             | Comunicação peer-to-peer               |
| Frequência            | 2.4 GHz                               |
| Alcance               | Até 220m (campo aberto)               |
| Taxa de Dados         | Até 1 Mbps                            |
| Latência              | <20ms                                 |
| Máximo de Peers       | 20 dispositivos                       |
| Criptografia          | AES de 128 bits (opcional)            |

## Características

### Vantagens
- **Sem Roteador**: Comunicação direta entre dispositivos
- **Baixa Latência**: Comunicação quase instantânea
- **Baixo Consumo**: Ideal para dispositivos com bateria
- **Maior Alcance**: Superior ao Bluetooth

### Limitações
- **Sem Internet**: Não permite acesso direto à internet
- **Protocolo Proprietário**: Funciona apenas entre dispositivos ESP
- **Compatibilidade**: Limitado ao ecossistema Espressif

## ⚠️ **NÃO FOI USADO NO PROJETO**

### Por que não foi implementado:

1. **Conectividade necessária**: O projeto precisa de acesso à internet para ThingsBoard
2. **Arquitetura centralizada**: Todos os sensores estão no mesmo ESP32
3. **Complexidade desnecessária**: Wi-Fi tradicional é mais simples
4. **Alcance suficiente**: Distâncias curtas no projeto atual

### Decisão Técnica
```
❌ ESP-NOW: Comunicação local apenas
✅ Wi-Fi: Conexão direta com ThingsBoard
```

## Quando seria útil

ESP-NOW seria vantajoso em cenários como:
- **Sensores distribuídos** em grandes áreas
- **Múltiplas estufas** distantes
- **Locais sem Wi-Fi** disponível
- **Economia extrema de energia** com sensores à bateria

## Exemplo de Implementação (Referência)

```cpp
#include <esp_now.h>
#include <WiFi.h>

typedef struct {
    float temperatura;
    float umidade_solo;
    int sensor_id;
} sensor_data_t;

void setup() {
    WiFi.mode(WIFI_STA);
    esp_now_init();
    // Configuração de peers...
}

void enviar_dados() {
    sensor_data_t dados = {25.5, 45.0, 1};
    esp_now_send(gateway_mac, (uint8_t*)&dados, sizeof(dados));
}
```

---

## Status do Projeto

```
ESP-NOW: AVALIADO MAS NÃO IMPLEMENTADO
Motivo: Wi-Fi tradicional atende todos os requisitos
Decisão: Manter arquitetura atual (mais simples)
```