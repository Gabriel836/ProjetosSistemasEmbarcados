#pragma once
#include <WiFi.h>
#include "MaquinaEstados.h"
#include "Config.h"

class Comunicacao {
public:
  void iniciar();
  void atualizar(MaquinaEstados& maquina, const Leituras& leituras, uint32_t agoraMs);
  void notificarTransicao(EstadoMaquina anterior, const MaquinaEstados& maquina);
private:
  WiFiClient cliente_;
  String buffer_;
  bool wifiConectadoAnterior_ = false;
  uint32_t tentativaWifi_ = 0;
  uint32_t tentativaTcp_ = 0;
  uint32_t ultimoPing_ = 0;
  uint32_t ultimaTelemetria_ = 0;
  uint32_t ultimaResposta_ = 0;
  uint32_t seqPing_ = 0;
  void enviar(const String& linha);
  void processarLinha(const String& linha, MaquinaEstados& maquina,
                      const Leituras& leituras, uint32_t agoraMs);
  void enviarStatus(const MaquinaEstados& maquina, const Leituras& l, const char* tipo);
};
