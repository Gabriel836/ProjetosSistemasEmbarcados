#include "Atuadores.h"
#include "Config.h"

void Atuadores::iniciar() {
  pinMode(Config::PINO_LED_VERDE, OUTPUT);
  pinMode(Config::PINO_LED_VERM, OUTPUT);
  pinMode(Config::PINO_BUZZER, OUTPUT);
  digitalWrite(Config::PINO_LED_VERDE, LOW);
  digitalWrite(Config::PINO_LED_VERM, LOW);
  digitalWrite(Config::PINO_BUZZER, LOW);
  servo_.setPeriodHertz(50);
  servo_.attach(Config::PINO_SERVO, 500, 2400);
  servo_.write(Config::SERVO_PARADO_GRAUS);
}

void Atuadores::atualizar(EstadoMaquina estado, uint32_t agoraMs) {
  if (estado != estadoAnterior_) {
    servo_.write((estado == EstadoMaquina::NORMAL || estado == EstadoMaquina::ALERTA)
                   ? Config::SERVO_ATIVO_GRAUS : Config::SERVO_PARADO_GRAUS);
    estadoAnterior_ = estado;
  }
  const bool piscar = (agoraMs / 500U) % 2U == 0U;
  const bool verde = estado == EstadoMaquina::NORMAL ||
                     (estado == EstadoMaquina::INICIANDO && piscar);
  const bool vermelho = estado == EstadoMaquina::FALHA ||
                        (estado == EstadoMaquina::ALERTA && piscar);
  digitalWrite(Config::PINO_LED_VERDE, verde ? HIGH : LOW);
  digitalWrite(Config::PINO_LED_VERM, vermelho ? HIGH : LOW);
  digitalWrite(Config::PINO_BUZZER, estado == EstadoMaquina::FALHA ? HIGH : LOW);
}
