#include "../esp32_maquina_industrial/MaquinaEstados.h"
#include <cassert>
#include <cstdio>

static Leituras seguro() {
  Leituras l;
  l.dhtValido = true;
  l.distanciaValida = true;
  l.temperaturaC = 25;
  l.umidadePct = 45;
  l.cargaPct = 50;
  l.distanciaMm = 400;
  return l;
}

int main() {
  MaquinaEstados m;
  Leituras l = seguro();
  assert(m.estado() == EstadoMaquina::DESLIGADA);
  Leituras invalido = l;
  invalido.dhtValido = false;
  assert(m.comando(ComandoMaquina::START, invalido, 0) ==
         ResultadoComando::SENSORES_INDISPONIVEIS);
  assert(m.comando(ComandoMaquina::START, l, 0) == ResultadoComando::OK);
  assert(m.estado() == EstadoMaquina::INICIANDO);
  m.atualizar(l, 1999);
  assert(m.estado() == EstadoMaquina::INICIANDO);
  m.atualizar(l, 2000);
  assert(m.estado() == EstadoMaquina::NORMAL);

  l.cargaPct = 80;
  m.atualizar(l, 2100);
  assert(m.estado() == EstadoMaquina::ALERTA);
  l.cargaPct = 70;  // Histerese: ainda ALERTA
  m.atualizar(l, 2200);
  assert(m.estado() == EstadoMaquina::ALERTA);
  l.cargaPct = 50;
  m.atualizar(l, 2300);
  assert(m.estado() == EstadoMaquina::NORMAL);

  l.temperaturaC = 45;
  m.atualizar(l, 2400);
  assert(m.estado() == EstadoMaquina::FALHA);
  assert(m.comando(ComandoMaquina::STOP, l, 2500) ==
         ResultadoComando::ESTADO_INVALIDO);
  assert(m.comando(ComandoMaquina::RESET, l, 2600) ==
         ResultadoComando::CONDICAO_INSEGURA);
  l.temperaturaC = 25;
  assert(m.comando(ComandoMaquina::RESET, l, 2700) == ResultadoComando::OK);
  assert(m.estado() == EstadoMaquina::DESLIGADA);

  // Falha de sensor durante a operacao.
  assert(m.comando(ComandoMaquina::START, l, 3000) == ResultadoComando::OK);
  m.atualizar(l, 5000);
  assert(m.estado() == EstadoMaquina::NORMAL);
  l.distanciaValida = false;
  m.atualizar(l, 5100);
  assert(m.estado() == EstadoMaquina::FALHA);
  assert(m.comando(ComandoMaquina::RESET, l, 5200) ==
         ResultadoComando::SENSORES_INDISPONIVEIS);
  l.distanciaValida = true;
  assert(m.comando(ComandoMaquina::RESET, l, 5300) == ResultadoComando::OK);

  // Distancia proxima emite ALERTA; distancia critica causa FALHA.
  assert(m.comando(ComandoMaquina::START, l, 6000) == ResultadoComando::OK);
  m.atualizar(l, 8000);
  l.distanciaMm = 100;
  m.atualizar(l, 8100);
  assert(m.estado() == EstadoMaquina::ALERTA);
  l.distanciaMm = 45;
  m.atualizar(l, 8200);
  assert(m.estado() == EstadoMaquina::FALHA);
  std::puts("TODOS OS TESTES DA MAQUINA DE ESTADOS PASSARAM");
}
