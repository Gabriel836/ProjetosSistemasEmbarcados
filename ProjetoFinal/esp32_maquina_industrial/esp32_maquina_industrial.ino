/*
 * SEL0630 - Maquina industrial simulada / ESP32 + Raspberry Pi Wi-Fi.
 * Compilar no Arduino IDE como "ESP32 Dev Module".
 * Veja README.md antes de energizar o servomotor.
 */
#include <Arduino.h>
#include "Config.h"
#include "Sensores.h"
#include "Atuadores.h"
#include "MaquinaEstados.h"
#include "Comunicacao.h"

Sensores sensores;
Atuadores atuadores;
MaquinaEstados maquina;
Comunicacao comunicacao;
String entradaSerial;
uint32_t ultimoLogMs = 0;

void comandoSerial(String linha, uint32_t agoraMs) {
  linha.trim();
  linha.toUpperCase();
  Leituras l = sensores.obter(agoraMs);
  ComandoMaquina cmd;
  bool ehComando = true;
  if      (linha == "START") cmd = ComandoMaquina::START;
  else if (linha == "STOP")  cmd = ComandoMaquina::STOP;
  else if (linha == "RESET") cmd = ComandoMaquina::RESET;
  else ehComando = false;

  if (ehComando) {
    ResultadoComando r = maquina.comando(cmd, l, agoraMs);
    Serial.printf("[SERIAL] %s: %s\n", linha.c_str(), nomeResultado(r));
    return;
  }
  if (linha.startsWith("SIM TEMP ")) {
    sensores.simularTemperatura(linha.substring(9).toFloat());
  } else if (linha.startsWith("SIM LOAD ")) {
    sensores.simularCarga(linha.substring(9).toInt());
  } else if (linha.startsWith("SIM DIST ")) {
    sensores.simularDistancia(linha.substring(9).toInt());
  } else if (linha == "SIM OFF") {
    sensores.desligarSimulacao();
  } else if (linha == "STATUS") {
    Serial.printf("[STATUS] %s, motivo=%s, temp=%.1f, umidade=%.1f, carga=%d%%, dist=%dmm, DHT=%d, TOF=%d, SIM=%d\n",
                  nomeEstado(maquina.estado()), maquina.motivo(), l.temperaturaC,
                  l.umidadePct, l.cargaPct, l.distanciaMm, l.dhtValido,
                  l.distanciaValida, sensores.simulacaoAtiva());
  } else if (linha == "HELP") {
    Serial.println("Comandos: START | STOP | RESET | STATUS | SIM TEMP 40 | SIM LOAD 80 | SIM DIST 80 | SIM OFF");
  } else if (linha.length()) {
    Serial.println("Comando desconhecido. Use HELP.");
  }
}

void lerSerial(uint32_t agoraMs) {
  while (Serial.available()) {
    const char c = static_cast<char>(Serial.read());
    if (c == '\n') {
      comandoSerial(entradaSerial, agoraMs);
      entradaSerial = "";
    } else if (c != '\r') {
      if (entradaSerial.length() < 80) entradaSerial += c;
      else entradaSerial = "";
    }
  }
}

void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.println("\n=== Monitoramento industrial: ESP32 + Raspberry Pi ===");
  entradaSerial.reserve(90);
  atuadores.iniciar();
  sensores.iniciar();
  comunicacao.iniciar();
  Serial.println("Digite HELP no Monitor Serial (115200, newline) para testar.");
}

void loop() {
  uint32_t agoraMs = millis();
  const EstadoMaquina anterior = maquina.estado();
  sensores.atualizar(agoraMs);
  lerSerial(agoraMs);
  const Leituras l = sensores.obter(millis());
  maquina.atualizar(l, millis());
  // Rede e interface nao decidem os limites: apenas enviam comandos.
  comunicacao.atualizar(maquina, l, millis());
  atuadores.atualizar(maquina.estado(), millis());
  comunicacao.notificarTransicao(anterior, maquina);

  if (millis() - ultimoLogMs >= 3000) {
    ultimoLogMs = millis();
    Serial.printf("[INFO] Estado=%s T=%.1f Carga=%d%% Dist=%dmm DHT=%d TOF=%d\n",
                  nomeEstado(maquina.estado()), l.temperaturaC, l.cargaPct,
                  l.distanciaMm, l.dhtValido, l.distanciaValida);
  }
  delay(5);  // Cedencia ao agendador, nao como mecanismo de temporizacao.
}
