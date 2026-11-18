#include "Sensores.h"
#include <math.h>

void Sensores::iniciar() {
  dht_.begin();
  Wire.begin(Config::PINO_SDA, Config::PINO_SCL);
  Wire.setClock(100000);
  analogReadResolution(12);
  pinMode(Config::PINO_POT, INPUT);

  tof_.setTimeout(150);
  tofInicializado_ = tof_.init();
  if (tofInicializado_) {
#if MODELO_VL53L1X
    tof_.setDistanceMode(VL53L1X::Short);
    tof_.setMeasurementTimingBudget(50000);
    tof_.startContinuous(100);
#else
    tof_.startContinuous(100);
#endif
    Serial.println("[SENSOR] VL53 inicializado.");
  } else {
    Serial.println("[SENSOR] VL53 nao encontrado. Verifique modelo, I2C e alimentacao.");
  }

  // Disparar as primeiras leituras assim que possivel.
  tentativaDht_ = millis() - Config::PERIODO_DHT_MS;
  tentativaTof_ = millis() - Config::PERIODO_TOF_MS;
  tentativaPot_ = millis() - Config::PERIODO_POT_MS;
}

void Sensores::atualizar(uint32_t agoraMs) {
  if (agoraMs - tentativaPot_ >= Config::PERIODO_POT_MS) {
    tentativaPot_ = agoraMs;
    const int valor = analogRead(Config::PINO_POT);
    cargaPct_ = constrain((valor * 100L + 2047L) / 4095L, 0L, 100L);
  }
  if (agoraMs - tentativaDht_ >= Config::PERIODO_DHT_MS) {
    tentativaDht_ = agoraMs;
    const float novaTemp = dht_.readTemperature();
    const float novaUmidade = dht_.readHumidity();
    if (!isnan(novaTemp) && !isnan(novaUmidade)) {
      temperaturaC_ = novaTemp;
      umidadePct_ = novaUmidade;
      ultimaLeituraDht_ = millis();
      dhtJaLeu_ = true;
    }
  }
  if (tofInicializado_ && agoraMs - tentativaTof_ >= Config::PERIODO_TOF_MS) {
    tentativaTof_ = agoraMs;
#if MODELO_VL53L1X
    const uint16_t mm = tof_.read();
#else
    const uint16_t mm = tof_.readRangeContinuousMillimeters();
#endif
    if (!tof_.timeoutOccurred() && mm > 0 && mm < 4000) {
      distanciaMm_ = mm;
      ultimaLeituraTof_ = millis();
      tofJaLeu_ = true;
    }
  }
}

Leituras Sensores::obter(uint32_t agoraMs) const {
  Leituras l;
  l.temperaturaC = simTemp_ ? tempSimC_ : temperaturaC_;
  l.umidadePct = umidadePct_;
  l.cargaPct = simCarga_ ? cargaSimPct_ : cargaPct_;
  l.distanciaMm = simDist_ ? distSimMm_ : distanciaMm_;
  l.dhtValido = simTemp_ || (dhtJaLeu_ && agoraMs - ultimaLeituraDht_ < Config::VALIDADE_DHT_MS);
  l.distanciaValida = simDist_ || (tofJaLeu_ && agoraMs - ultimaLeituraTof_ < Config::VALIDADE_TOF_MS);
  return l;
}

void Sensores::simularTemperatura(float t) { simTemp_ = true; tempSimC_ = t; }
void Sensores::simularCarga(int c) { simCarga_ = true; cargaSimPct_ = constrain(c, 0, 100); }
void Sensores::simularDistancia(int d) { simDist_ = true; distSimMm_ = max(1, d); }
void Sensores::desligarSimulacao() { simTemp_ = simCarga_ = simDist_ = false; }
