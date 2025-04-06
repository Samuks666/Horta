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
      * Chuva
      * Pressão
  - Atuadores para controle da irrigação
  
- **Software:**
  - Programação do ESP32 utilizando **Arduino IDE**
  - Comunicação via **MQTT** ou **HTTP** para integração com uma interface web ou app
  - **Inteligência Artificial**: Possível implementação de modelos de machine learning usando algoritimo KNN

