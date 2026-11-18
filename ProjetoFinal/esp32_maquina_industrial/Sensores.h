#pragma once
#include <Arduino.h>
#include <DHT.h>
#include <Wire.h>
#include "Config.h"
#include "MaquinaEstados.h"

#if MODELO_VL53L1X
  #include <VL53L1X.h>
#else
  #include <VL53L0X.h>
#endif

class Sensores {
public:
  void iniciar();
  void atualizar(uint32_t agoraMs);
  Leituras obter(uint32_t agoraMs) const;

  // Sobrescreve valores apenas para teste no Monitor Serial.
  void simularTemperatura(float celsius);
  void simularCarga(int porcentagem);
  void simularDistancia(int milimetros);
  void desligarSimulacao();
  bool simulacaoAtiva() const { return simTemp_ || simCarga_ || simDist_; }

private:
  DHT dht_{Config::PINO_DHT, DHT11};
#if MODELO_VL53L1X
  VL53L1X tof_;
#else
  VL53L0X tof_;
#endif
  bool tofInicializado_ = false;
  float temperaturaC_ = 0.0f;
  float umidadePct_ = 0.0f;
  int distanciaMm_ = 0;
  int cargaPct_ = 0;
  bool dhtJaLeu_ = false;
  bool tofJaLeu_ = false;
  uint32_t ultimaLeituraDht_ = 0;
  uint32_t ultimaLeituraTof_ = 0;
  uint32_t tentativaDht_ = 0;
  uint32_t tentativaTof_ = 0;
  uint32_t tentativaPot_ = 0;
  bool simTemp_ = false;
  bool simCarga_ = false;
  bool simDist_ = false;
  float tempSimC_ = 0.0f;
  int cargaSimPct_ = 0;
  int distSimMm_ = 0;
};
