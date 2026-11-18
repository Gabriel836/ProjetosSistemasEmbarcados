#pragma once
#include <cstdint>

// Configuracoes do comportamento da maquina, sem dependencias Arduino.
// Valores APENAS didaticos; calibrar antes de qualquer uso fisico.
namespace Limites {
constexpr uint32_t TEMPO_INICIO_MS = 2000;
constexpr float TEMP_ALERTA_C = 35.0f;
constexpr float TEMP_FALHA_C = 45.0f;
constexpr float TEMP_RETORNO_C = 33.0f;
constexpr int CARGA_ALERTA_PCT = 75;
constexpr int CARGA_FALHA_PCT = 95;
constexpr int CARGA_RETORNO_PCT = 68;
constexpr int DIST_ALERTA_MM = 120;
constexpr int DIST_FALHA_MM = 50;
constexpr int DIST_RETORNO_MM = 150;
}
