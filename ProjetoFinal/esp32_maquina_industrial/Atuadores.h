#pragma once
#include <Arduino.h>
#include <ESP32Servo.h>
#include "MaquinaEstados.h"

class Atuadores {
public:
  void iniciar();
  void atualizar(EstadoMaquina estado, uint32_t agoraMs);
private:
  Servo servo_;
  EstadoMaquina estadoAnterior_ = EstadoMaquina::DESLIGADA;
};
