#include "Comunicacao.h"

void Comunicacao::iniciar() {
  buffer_.reserve(240);
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.begin(Config::WIFI_SSID, Config::WIFI_SENHA);
  tentativaWifi_ = millis();
  Serial.println("[WIFI] Iniciando conexao. Maquina opera localmente mesmo offline.");
}

void Comunicacao::enviar(const String& linha) {
  if (cliente_.connected()) cliente_.print(linha + "\n");
}

void Comunicacao::enviarStatus(const MaquinaEstados& maquina, const Leituras& l, const char* tipo) {
  String s;
  s.reserve(220);
  s += tipo;
  s += "|STATE="; s += nomeEstado(maquina.estado());
  s += "|TEMP_C="; s += l.dhtValido ? String(l.temperaturaC, 1) : String("NA");
  s += "|HUM_PCT="; s += l.dhtValido ? String(l.umidadePct, 1) : String("NA");
  s += "|LOAD_PCT="; s += String(l.cargaPct);
  s += "|DIST_MM="; s += l.distanciaValida ? String(l.distanciaMm) : String("NA");
  s += "|DHT_OK="; s += String(l.dhtValido ? 1 : 0);
  s += "|TOF_OK="; s += String(l.distanciaValida ? 1 : 0);
  s += "|REASON="; s += maquina.motivo();
  enviar(s);
}

void Comunicacao::processarLinha(const String& linha, MaquinaEstados& maquina,
                                const Leituras& l, uint32_t agoraMs) {
  ultimaResposta_ = agoraMs;
  if (linha == "CMD|STATUS") {
    enviarStatus(maquina, l, "STATUS");
  } else if (linha == "CMD|START" || linha == "CMD|STOP" || linha == "CMD|RESET") {
    const String nome = linha.substring(4);
    const ComandoMaquina comando = nome == "START" ? ComandoMaquina::START :
                                   nome == "STOP" ? ComandoMaquina::STOP : ComandoMaquina::RESET;
    const ResultadoComando resultado = maquina.comando(comando, l, agoraMs);
    enviar(String(resultado == ResultadoComando::OK ? "ACK|" : "NACK|") +
           nome + "|" + nomeResultado(resultado));
  } else if (linha == "HELLO|RPI") {
    Serial.println("[TCP] Mensagem inicial da Raspberry Pi recebida.");
  } else if (linha.startsWith("PONG|")) {
    // Heartbeat recebido. O numero de sequencia e informativo neste prototipo.
  } else {
    enviar("NACK|COMANDO|DESCONHECIDO");
  }
}

void Comunicacao::atualizar(MaquinaEstados& maquina, const Leituras& l, uint32_t agoraMs) {
  if (WiFi.status() != WL_CONNECTED) {
    if (wifiConectadoAnterior_) {
      Serial.println("[WIFI] Conexao perdida.");
      cliente_.stop();
      wifiConectadoAnterior_ = false;
    }
    if (agoraMs - tentativaWifi_ > 10000) {
      tentativaWifi_ = agoraMs;
      WiFi.reconnect();
    }
    return;
  }
  if (!wifiConectadoAnterior_) {
    wifiConectadoAnterior_ = true;
    tentativaTcp_ = agoraMs - 3000;
    Serial.printf("[WIFI] Conectado. IP: %s\n", WiFi.localIP().toString().c_str());
  }
  if (!cliente_.connected()) {
    cliente_.stop();
    buffer_ = "";
    if (agoraMs - tentativaTcp_ >= 3000) {
      tentativaTcp_ = agoraMs;
      IPAddress ip(Config::IP_RPI[0], Config::IP_RPI[1],
                   Config::IP_RPI[2], Config::IP_RPI[3]);
      // Timeout limitado: conexao com Linux nao deve atrasar controle indefinidamente.
      if (cliente_.connect(ip, Config::PORTA_TCP, 400)) {
        ultimaResposta_ = millis();
        ultimaTelemetria_ = millis() - Config::PERIODO_TELEMETRIA_MS;
        ultimoPing_ = millis();
        Serial.println("[TCP] Raspberry Pi conectada.");
      }
    }
    return;
  }
  size_t bytesLidos = 0;
  while (cliente_.available() && bytesLidos++ < 256) {
    const char ch = static_cast<char>(cliente_.read());
    if (ch == '\n') {
      buffer_.trim();
      if (buffer_.length()) processarLinha(buffer_, maquina, l, millis());
      buffer_ = "";
    } else if (ch != '\r') {
      if (buffer_.length() >= 200) {
        Serial.println("[TCP] Mensagem longa. Desconectando.");
        cliente_.stop();
        return;
      }
      buffer_ += ch;
    }
  }
  if (agoraMs - ultimaTelemetria_ >= Config::PERIODO_TELEMETRIA_MS) {
    ultimaTelemetria_ = agoraMs;
    enviarStatus(maquina, l, "DATA");
  }
  if (agoraMs - ultimoPing_ >= Config::PERIODO_PING_MS) {
    ultimoPing_ = agoraMs;
    enviar("PING|" + String(++seqPing_) + "|" + String(agoraMs));
  }
  if (agoraMs - ultimaResposta_ >= 12000) {
    Serial.println("[TCP] Sem resposta: reconectando.");
    cliente_.stop();
  }
}

void Comunicacao::notificarTransicao(EstadoMaquina anterior, const MaquinaEstados& maquina) {
  if (anterior == maquina.estado()) return;
  Serial.printf("[ESTADO] %s -> %s (%s)\n", nomeEstado(anterior),
                nomeEstado(maquina.estado()), maquina.motivo());
  enviar(String("EVENT|STATE|") + nomeEstado(anterior) + "|" +
         nomeEstado(maquina.estado()) + "|" + maquina.motivo());
}
