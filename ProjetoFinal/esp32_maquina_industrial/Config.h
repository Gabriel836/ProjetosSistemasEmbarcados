#pragma once
#include <Arduino.h>

// Projeto para ESP32 DevKit / ESP32-WROOM-32 (nao ESP32-C3/S3).
namespace Config {
// NUNCA publique a senha real do Wi-Fi em um repositorio publico.
constexpr const char* WIFI_SSID = "NOME_DA_REDE";
constexpr const char* WIFI_SENHA = "SENHA_DA_REDE";
constexpr uint8_t IP_RPI[4] = {192, 168, 1, 100};
constexpr uint16_t PORTA_TCP = 5000;

// Altere para 1 se o sensor fisico for VL53L1X; 0 = VL53L0X.
#define MODELO_VL53L1X 0

constexpr uint8_t PINO_SDA       = 21;
constexpr uint8_t PINO_SCL       = 22;
constexpr uint8_t PINO_DHT       = 4;
constexpr uint8_t PINO_POT       = 34; // ADC1: funciona durante Wi-Fi
constexpr uint8_t PINO_LED_VERDE = 25;
constexpr uint8_t PINO_LED_VERM  = 26;
constexpr uint8_t PINO_BUZZER    = 27; // buzzer ATIVO 3,3 V, baixo consumo
constexpr uint8_t PINO_SERVO     = 18;

// Agendamento cooperativo: millis(), sem delay bloqueante no loop.
constexpr uint32_t PERIODO_TOF_MS     = 200;
constexpr uint32_t PERIODO_DHT_MS     = 2200; // DHT11: nao ler muito rapido
constexpr uint32_t PERIODO_POT_MS     = 100;
constexpr uint32_t VALIDADE_TOF_MS    = 1200;
constexpr uint32_t VALIDADE_DHT_MS    = 7000;
constexpr uint32_t TEMPO_INICIO_MS    = 2000;
constexpr uint32_t PERIODO_TELEMETRIA_MS = 1000;
constexpr uint32_t PERIODO_PING_MS    = 3000;

// Limites e histerese ficam em ConfigLimites.h (nucleo C++ testavel).
constexpr int SERVO_PARADO_GRAUS = 0;
constexpr int SERVO_ATIVO_GRAUS  = 90;
} // namespace Config
