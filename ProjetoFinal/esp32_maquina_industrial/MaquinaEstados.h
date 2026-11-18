#pragma once
#include <cstdint>

// Nucleo de controle propositalmente independente de Arduino/Wi-Fi:
// pode ser testado por um programa C++ em um computador.
enum class EstadoMaquina : uint8_t {
  DESLIGADA, INICIANDO, NORMAL, ALERTA, FALHA
};

enum class ComandoMaquina : uint8_t { START, STOP, RESET };

enum class ResultadoComando : uint8_t {
  OK, ESTADO_INVALIDO, SENSORES_INDISPONIVEIS, CONDICAO_INSEGURA
};

struct Leituras {
  float temperaturaC = 0.0f;
  float umidadePct = 0.0f;
  int cargaPct = 0;
  int distanciaMm = 0;
  bool dhtValido = false;
  bool distanciaValida = false;
};

const char* nomeEstado(EstadoMaquina estado);
const char* nomeResultado(ResultadoComando resultado);

class MaquinaEstados {
public:
  EstadoMaquina estado() const { return estado_; }
  const char* motivo() const { return motivo_; }
  ResultadoComando comando(ComandoMaquina comando, const Leituras& leituras,
                           uint32_t agoraMs);
  void atualizar(const Leituras& leituras, uint32_t agoraMs);

private:
  enum class Nivel { OK, ALERTA, FALHA };
  EstadoMaquina estado_ = EstadoMaquina::DESLIGADA;
  uint32_t inicioEstadoMs_ = 0;
  const char* motivo_ = "INICIALIZACAO";

  static Nivel avaliar(const Leituras& leituras, const char*& motivo);
  static bool recuperado(const Leituras& leituras);
  void entrar(EstadoMaquina novoEstado, const char* motivo, uint32_t agoraMs);
};
