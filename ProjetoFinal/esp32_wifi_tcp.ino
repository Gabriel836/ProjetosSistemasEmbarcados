/*
 * Prototipo de comunicacao Wi-Fi entre ESP32 e Raspberry Pi.
 * Placa: ESP32 DevKit (framework Arduino / ESP32 by Espressif Systems).
 * Cliente TCP; mensagens de texto terminadas em '\n'.
 *
 * ALTERE WIFI_SSID, WIFI_PASSWORD e RASPBERRY_IP antes de gravar.
 */

#include <WiFi.h>

const char *WIFI_SSID = "NOME_DA_REDE";
const char *WIFI_PASSWORD = "SENHA_DA_REDE";

// Coloque aqui o IPv4 exibido por 'hostname -I' na Raspberry Pi.
IPAddress RASPBERRY_IP(192, 168, 1, 100);
const uint16_t RASPBERRY_PORT = 5000;

const unsigned long INTERVALO_PING_MS = 2000;
const unsigned long INTERVALO_RECONEXAO_TCP_MS = 3000;
const unsigned long INTERVALO_RECONEXAO_WIFI_MS = 10000;
const unsigned long TIMEOUT_RECEPCAO_MS = 12000;

WiFiClient cliente;
String bufferRecepcao;

bool wifiAnteriormenteConectado = false;
unsigned long ultimoPing = 0;
unsigned long ultimoEnvioPing = 0;
unsigned long ultimaResposta = 0;
unsigned long ultimaTentativaTCP = 0;
unsigned long ultimaTentativaWiFi = 0;
uint32_t numeroPing = 0;
uint32_t pingPendente = 0;
bool aguardandoPong = false;

void processarMensagem(const String &mensagem) {
  ultimaResposta = millis();
  Serial.print("[RPI -> ESP32] ");
  Serial.println(mensagem);

  if (mensagem == "CMD|STATUS") {
    // A Raspberry Pi pode enviar comandos mesmo sem aguardar um PING.
    cliente.printf("STATUS|ESP32|UPTIME_MS=%lu|RSSI_DBM=%d\n",
                   (unsigned long)millis(), WiFi.RSSI());
    Serial.println("[ESP32 -> RPI] STATUS enviado");
    return;
  }

  if (mensagem.startsWith("PONG|")) {
    int separador = mensagem.indexOf('|', 5);
    if (separador > 5) {
      uint32_t numeroRecebido = (uint32_t)mensagem.substring(5, separador).toInt();
      if (aguardandoPong && numeroRecebido == pingPendente) {
        unsigned long rtt = millis() - ultimoEnvioPing;
        Serial.printf("[OK] PONG #%lu recebido | RTT = %lu ms\n",
                      (unsigned long)numeroRecebido, rtt);
        aguardandoPong = false;
      }
    }
  }
}

void lerMensagens() {
  while (cliente.available() > 0) {
    char caractere = (char)cliente.read();
    if (caractere == '\n') {
      bufferRecepcao.trim();
      if (!bufferRecepcao.isEmpty()) {
        processarMensagem(bufferRecepcao);
      }
      bufferRecepcao = "";
    } else if (caractere != '\r') {
      if (bufferRecepcao.length() < 200) {
        bufferRecepcao += caractere;
      } else {
        Serial.println("[ERRO] Mensagem muito longa; desconectando.");
        cliente.stop();
        bufferRecepcao = "";
        return;
      }
    }
  }
}

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("\n=== Teste Wi-Fi ESP32 <-> Raspberry Pi ===");

  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  ultimaTentativaWiFi = millis();
  bufferRecepcao.reserve(210);
  Serial.printf("Conectando ao Wi-Fi: %s\n", WIFI_SSID);
}

void loop() {
  const unsigned long agora = millis();

  // 1. Garantir Wi-Fi, sem bloquear indefinidamente.
  if (WiFi.status() != WL_CONNECTED) {
    if (wifiAnteriormenteConectado) {
      Serial.println("[Wi-Fi] Conexao perdida.");
      cliente.stop();
      wifiAnteriormenteConectado = false;
    }
    if (agora - ultimaTentativaWiFi >= INTERVALO_RECONEXAO_WIFI_MS) {
      ultimaTentativaWiFi = agora;
      Serial.println("[Wi-Fi] Tentando reconectar...");
      WiFi.reconnect();
    }
    delay(20);
    return;
  }

  if (!wifiAnteriormenteConectado) {
    wifiAnteriormenteConectado = true;
    Serial.print("[Wi-Fi] Conectado! IP do ESP32: ");
    Serial.println(WiFi.localIP());
    ultimaTentativaTCP = agora - INTERVALO_RECONEXAO_TCP_MS;
  }

  // 2. Estabelecer / restabelecer conexao TCP com o servidor.
  if (!cliente.connected()) {
    cliente.stop();
    if (agora - ultimaTentativaTCP >= INTERVALO_RECONEXAO_TCP_MS) {
      ultimaTentativaTCP = agora;
      Serial.printf("[TCP] Conectando a %s:%u...\n",
                    RASPBERRY_IP.toString().c_str(), RASPBERRY_PORT);
      if (cliente.connect(RASPBERRY_IP, RASPBERRY_PORT)) {
        Serial.println("[TCP] Conectado a Raspberry Pi!");
        bufferRecepcao = "";
        ultimaResposta = millis();
        ultimoPing = millis() - INTERVALO_PING_MS;
        aguardandoPong = false;
      } else {
        Serial.println("[TCP] Servidor indisponivel; nova tentativa em 3 s.");
      }
    }
    delay(20);
    return;
  }

  // 3. Processar respostas e comandos recebidos da Raspberry Pi.
  lerMensagens();
  if (!cliente.connected()) {
    return;
  }

  // 4. Enviar PING periodico.
  if (agora - ultimoPing >= INTERVALO_PING_MS) {
    ultimoPing = agora;
    numeroPing++;
    pingPendente = numeroPing;
    ultimoEnvioPing = millis();
    aguardandoPong = true;

    cliente.printf("PING|%lu|%lu\n",
                   (unsigned long)numeroPing, ultimoEnvioPing);
    Serial.printf("[ESP32 -> RPI] PING #%lu\n", (unsigned long)numeroPing);
  }

  // 5. Se o socket continuar aberto, mas a Raspberry Pi nao responder,
  //    forcar reconexao TCP.
  if (millis() - ultimaResposta >= TIMEOUT_RECEPCAO_MS) {
    Serial.println("[TCP] Sem resposta por 12 s; reconectando...");
    cliente.stop();
  }

  delay(10);
}
