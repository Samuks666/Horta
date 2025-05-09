#ifndef THINGSBOARD_H
#define THINGSBOARD_H

#include <WiFi.h>
#include <PubSubClient.h>
#include <DHTesp.h>
#include <DHT.h>
#include "secrets.h" 

// ======= CONFIGURAÇÃO =======
extern const char* ssid;
extern const char* password;
extern const char* thingsboardServer;
extern const char* accessToken;


extern bool relayState;

// ======= CALLBACK RPC (botao relay) =======
void callback(char* topic, byte* payload, unsigned int length);

// ======= conexoes =======
void connectWiFi();

void connectThingsBoard();

// ======= inicializacao =======
void setupThingsBoard();

// ======= loop principal (MQTT) =======
void maintainThingsBoard();

// ======= envia dados para a plataforma =======
void sendTelemetry(float temp, float hum, float s_moist, float rain);

// ======= envia valores aleatorios para testar =======
void sendTestData();

#endif