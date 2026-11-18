#include "MaquinaEstados.h"
#include "ConfigLimites.h"

const char* nomeEstado(EstadoMaquina estado) {
  switch (estado) {
    case EstadoMaquina::DESLIGADA: return "DESLIGADA";
    case EstadoMaquina::INICIANDO: return "INICIANDO";
    case EstadoMaquina::NORMAL: return "NORMAL";
    case EstadoMaquina::ALERTA: return "ALERTA";
    case EstadoMaquina::FALHA: return "FALHA";
  }
  return "DESCONHECIDO";
}

const char* nomeResultado(ResultadoComando resultado) {
  switch (resultado) {
    case ResultadoComando::OK: return "OK";
    case ResultadoComando::ESTADO_INVALIDO: return "ESTADO_INVALIDO";
    case ResultadoComando::SENSORES_INDISPONIVEIS: return "SENSORES_INDISPONIVEIS";
    case ResultadoComando::CONDICAO_INSEGURA: return "CONDICAO_INSEGURA";
  }
  return "ERRO";
}

void MaquinaEstados::entrar(EstadoMaquina novoEstado, const char* motivo, uint32_t agoraMs) {
  if (novoEstado == estado_) return;
  estado_ = novoEstado;
  inicioEstadoMs_ = agoraMs;
  motivo_ = motivo;
}

MaquinaEstados::Nivel MaquinaEstados::avaliar(const Leituras& l, const char*& motivo) {
  // Uma leitura com validade expirada nao e considerada segura para operar.
  if (!l.dhtValido || !l.distanciaValida) {
    motivo = "SENSOR_INDISPONIVEL";
    return Nivel::FALHA;
  }
  if (l.temperaturaC >= Limites::TEMP_FALHA_C) {
    motivo = "SOBRETEMPERATURA";
    return Nivel::FALHA;
  }
  if (l.cargaPct >= Limites::CARGA_FALHA_PCT) {
    motivo = "SOBRECARGA";
    return Nivel::FALHA;
  }
  if (l.distanciaMm <= Limites::DIST_FALHA_MM) {
    motivo = "OBSTACULO_CRITICO";
    return Nivel::FALHA;
  }
  if (l.temperaturaC >= Limites::TEMP_ALERTA_C) {
    motivo = "TEMPERATURA_ELEVADA";
    return Nivel::ALERTA;
  }
  if (l.cargaPct >= Limites::CARGA_ALERTA_PCT) {
    motivo = "CARGA_ELEVADA";
    return Nivel::ALERTA;
  }
  if (l.distanciaMm <= Limites::DIST_ALERTA_MM) {
    motivo = "OBSTACULO_PROXIMO";
    return Nivel::ALERTA;
  }
  motivo = "OPERACAO_NORMAL";
  return Nivel::OK;
}

bool MaquinaEstados::recuperado(const Leituras& l) {
  return l.dhtValido && l.distanciaValida &&
         l.temperaturaC < Limites::TEMP_RETORNO_C &&
         l.cargaPct < Limites::CARGA_RETORNO_PCT &&
         l.distanciaMm > Limites::DIST_RETORNO_MM;
}

ResultadoComando MaquinaEstados::comando(ComandoMaquina cmd,
                                        const Leituras& l, uint32_t agoraMs) {
  if (cmd == ComandoMaquina::START) {
    if (estado_ != EstadoMaquina::DESLIGADA)
      return ResultadoComando::ESTADO_INVALIDO;
    if (!l.dhtValido || !l.distanciaValida)
      return ResultadoComando::SENSORES_INDISPONIVEIS;
    if (!recuperado(l))
      return ResultadoComando::CONDICAO_INSEGURA;
    entrar(EstadoMaquina::INICIANDO, "COMANDO_START", agoraMs);
    return ResultadoComando::OK;
  }
  if (cmd == ComandoMaquina::STOP) {
    // FALHA e travada: STOP nao remove o motivo da falha.
    if (estado_ == EstadoMaquina::FALHA)
      return ResultadoComando::ESTADO_INVALIDO;
    entrar(EstadoMaquina::DESLIGADA, "COMANDO_STOP", agoraMs);
    return ResultadoComando::OK;
  }
  if (cmd == ComandoMaquina::RESET) {
    if (estado_ != EstadoMaquina::FALHA)
      return ResultadoComando::ESTADO_INVALIDO;
    if (!l.dhtValido || !l.distanciaValida)
      return ResultadoComando::SENSORES_INDISPONIVEIS;
    if (!recuperado(l))
      return ResultadoComando::CONDICAO_INSEGURA;
    entrar(EstadoMaquina::DESLIGADA, "COMANDO_RESET", agoraMs);
    return ResultadoComando::OK;
  }
  return ResultadoComando::ESTADO_INVALIDO;
}

void MaquinaEstados::atualizar(const Leituras& l, uint32_t agoraMs) {
  if (estado_ == EstadoMaquina::DESLIGADA || estado_ == EstadoMaquina::FALHA) return;

  const char* motivo;
  const Nivel nivel = avaliar(l, motivo);
  if (nivel == Nivel::FALHA) {
    entrar(EstadoMaquina::FALHA, motivo, agoraMs);
    return;
  }

  if (estado_ == EstadoMaquina::INICIANDO) {
    if (agoraMs - inicioEstadoMs_ >= Limites::TEMPO_INICIO_MS) {
      entrar(nivel == Nivel::ALERTA ? EstadoMaquina::ALERTA : EstadoMaquina::NORMAL,
             motivo, agoraMs);
    }
    return;
  }
  if (estado_ == EstadoMaquina::NORMAL && nivel == Nivel::ALERTA) {
    entrar(EstadoMaquina::ALERTA, motivo, agoraMs);
  } else if (estado_ == EstadoMaquina::ALERTA && recuperado(l)) {
    entrar(EstadoMaquina::NORMAL, "CONDICOES_NORMALIZADAS", agoraMs);
  }
}
