# SEL0630 — Monitoramento de máquina industrial (protótipo 2)

Firmware para **ESP32 DevKit clássico ESP32-WROOM-32** com máquina de estados, sensores DHT11 / VL53L0X ou VL53L1X / potenciômetro, dois LEDs, buzzer ativo e servomotor, conectado via **Wi-Fi/TCP** a uma **Raspberry Pi** (servidor Python).

> **Protótipo didático**: não há garantias de tempo real rígido, segurança funcional ou redundância. Não conectar a equipamentos industriais ou cargas perigosas. O servo representa apenas um atuador de bancada.

## Estrutura dos arquivos

```
monitoramento_industrial_esp32_rpi/
├── esp32_maquina_industrial/
│   ├── esp32_maquina_industrial.ino    # setup / loop / comandos USB
│   ├── Config.h                       # Wi-Fi, pinos, sensor selecionado e periodos
│   ├── ConfigLimites.h                # limiares da maquina (sem dependencia Arduino)
│   ├── MaquinaEstados.h/.cpp          # logica dos 5 estados
│   ├── Sensores.h/.cpp                # DHT11, VL53 e ADC; simulacoes
│   ├── Atuadores.h/.cpp               # LED, buzzer ativo e PWM servo
│   └── Comunicacao.h/.cpp             # Wi-Fi cliente TCP / comandos / telemetria
├── raspberry_pi/servidor_tcp.py       # servidor TCP e console com START/STOP/RESET
└── testes/test_maquina.cpp           # testes de unidade executaveis em PC com g++
```

## Bibliotecas no Arduino IDE

Placa: **ESP32 Dev Module**, pacote **ESP32 by Espressif Systems** (preferencialmente versão 3.x). Gerenciador de bibliotecas:

1. **DHT sensor library** — Adafruit (instale também **Adafruit Unified Sensor**, dependência).
2. **ESP32Servo** — Kevin Harrington / madhephaestus.
3. **VL53L0X** — Pololu, **ou** **VL53L1X** — Pololu, conforme o modelo físico.

No arquivo `Config.h`, configure:

```cpp
constexpr const char* WIFI_SSID = "SUA_REDE_2G4";
constexpr const char* WIFI_SENHA = "SENHA";
constexpr uint8_t IP_RPI[4] = {192, 168, 1, 100};
#define MODELO_VL53L1X 0  // 0 = VL53L0X, 1 = VL53L1X
```

> A referência "VL53LXX" não especifica o modelo: **confira a inscrição na placa/embalagem do sensor e altere o seletor**. Os dois chips não são intercambiáveis em software.

Todos os arquivos da pasta `esp32_maquina_industrial` devem permanecer juntos, com o mesmo nome de pasta do sketch `.ino`. No Arduino IDE, abra `esp32_maquina_industrial.ino`.

## Ligações elétricas — ESP32 DevKit clássico

| Componente | Ligação | Observação |
|---|---|---|
| VL53L0X ou VL53L1X | SDA→GPIO21, SCL→GPIO22 | I²C, normalmente endereço 0x29; confira tensão compatível com seu **breakout** |
| DHT11 | DATA→GPIO4 | Alimente em 3,3 V; sensor avulso geralmente precisa de pull-up de ~10 kΩ entre DATA e 3,3 V |
| Potenciômetro 5 kΩ | pontas→3V3/GND, cursor→GPIO34 | ADC1; **nunca aplicar 5 V** ao GPIO34 |
| LED verde | GPIO25→resistor 220–330 Ω→ânodo; cátodo→GND | HIGH = aceso |
| LED vermelho | GPIO26→resistor 220–330 Ω→ânodo; cátodo→GND | HIGH = aceso |
| Buzzer **ativo** 3,3 V (baixo consumo) | GPIO27→positivo, negativo→GND | Se for 5 V ou consumir corrente elevada, usar transistor/driver e alimentação adequada |
| Servomotor (ex.: SG90) | sinal→GPIO18; V+→fonte 5 V externa; GND→GND comum | **Não alimentar o servo pelo 3V3 do ESP32**; conectar terra da fonte ao GND do ESP32 |

**Importante:** a ligação de alimentação e os níveis I²C do VL53 variam conforme a plaquinha comercial. Chips sem regulador/level-shifter exigem cuidados diferentes. Confira o datasheet do módulo antes de conectar.

## Estados e saídas

| Estado | LED verde | LED vermelho | Buzzer | Servo |
|---|---|---|---|---|
| DESLIGADA | off | off | off | 0° |
| INICIANDO | piscando | off | off | 0° |
| NORMAL | on | off | off | 90° |
| ALERTA | off | piscando | off | 90° |
| FALHA | off | on | on | 0° |

### Transições

- `DESLIGADA` + `START` + condições válidas/normais → `INICIANDO`.
- `INICIANDO` depois de **2 s** → `NORMAL`, caso sem anomalias, ou `ALERTA` se surgiu uma condição de alerta.
- `NORMAL` → `ALERTA` ao atingir qualquer limite de alerta.
- `NORMAL`, `ALERTA` ou `INICIANDO` → `FALHA` em limite crítico ou perda persistente de sensores.
- `ALERTA` → `NORMAL` automaticamente quando **todos** os sensores atingem limiares de recuperação, com histerese.
- `START` só é aceito quando a máquina estiver `DESLIGADA` e todos os sensores válidos e em faixa segura.
- `STOP` retorna `INICIANDO`, `NORMAL` ou `ALERTA` para `DESLIGADA`.
- `FALHA` fica travada. Só sai por `RESET` com as condições normalizadas; `STOP` não apaga a falha.

### Limiares iniciais (configuráveis em `ConfigLimites.h`)

| Variável | Alerta | Falha | Saída do alerta (histerese) |
|---|---:|---:|---:|
| Temperatura | ≥35 °C | ≥45 °C | <33 °C |
| Carga simulada | ≥75% | ≥95% | <68% |
| Distância | ≤120 mm | ≤50 mm | >150 mm |
| DHT11 ou ToF indisponível | — | Durante operação | Necessária leitura válida para RESET |

A umidade é telemetria, mas **não gera falha** nesta versão. O potenciômetro representa a carga; não há medição elétrica de corrente/torque. O sensor DHT11 possui precisão e taxa de atualização limitadas; não é instrumento para proteção térmica industrial.

O DHT11 é lido a cada **2,2 s**, o ToF a cada **200 ms** e o potenciômetro a cada **100 ms**. Leituras que ficam sem atualização válida por 7 s (DHT) ou 1,2 s (ToF) tornam o sensor inválido; durante a operação causam `FALHA`. O loop é cooperativo e pode apresentar atrasos (I²C e reconexão TCP); **não é controle hard real-time**.

## 1 — Raspberry Pi (servidor)

Conecte a Raspberry à mesma LAN do ESP32, e descubra seu IP:

```bash
hostname -I
python3 raspberry_pi/servidor_tcp.py
```

Se a Raspberry tiver IP `192.168.1.100`, altere `IP_RPI` no `Config.h` para `{192,168,1,100}`. Não use `0.0.0.0` como IP do destino. O ESP32 clássico usa Wi-Fi 2,4 GHz; evite rede de convidados com isolamento de clientes. Uma reserva DHCP no roteador é recomendada para manter o IP da Raspberry.

No terminal do servidor, digite:

```
status
start
stop
reset
quit
```

A Raspberry envia `CMD|START`, `CMD|STOP`, `CMD|RESET` ou `CMD|STATUS` para o ESP32. O ESP32 responde `ACK|...|OK` ou `NACK|...|MOTIVO`. Ao conectar, a Raspberry pergunta o status e responde aos PINGs do ESP32.

**Se a conexão Wi-Fi cair, o firmware continua executando a máquina de estados localmente.** Os comandos remotos ficam indisponíveis até a reconexão. O Monitor Serial via USB continua funcionando.

## 2 — ESP32 (firmware)

1. Instale as bibliotecas e selecione o modelo VL53 correto em `Config.h`.
2. Abra o sketch, ajuste SSID/senha/IP, selecione **ESP32 Dev Module** e carregue o firmware.
3. Abra o **Monitor Serial em 115200 baud, final de linha `Newline`**.
4. Aguarde o DHT11 e o VL53 entregarem leituras válidas (`DHT=1`, `TOF=1`) antes de usar `START`.
5. Digite `STATUS`, `START`, `STOP` e `RESET` no console da Raspberry ou via USB.

## 3 — Simular falhas com segurança, sem aquecer o sensor

O Monitor Serial implementa sobreposição temporária de leituras, apenas para demonstração:

```
SIM TEMP 25
SIM LOAD 50
SIM DIST 400
START
STATUS
SIM LOAD 80
STATUS
SIM LOAD 50
SIM DIST 40
STATUS
RESET
SIM DIST 400
RESET
SIM OFF
```

Interpretacao: (a) valores normais e partida; (b) carga alta provoca `ALERTA`; (c) carga volta ao normal; (d) obstaculo critico provoca `FALHA`; (e) o primeiro `RESET` e recusado enquanto distancia permanece critica; (f) `RESET` aceito apos normalizacao. `SIM OFF` volta a usar apenas leituras fisicas. Se parte dos sensores nao estiver conectada, defina **as tres simulacoes** para permitir `START` no modo de demonstracao. A umidade nessa simulacao continua com valor real/ultimo valor (ou zero sem leitura).

## Protocolo Wi-Fi/TCP

ASCII separado por `|` e encerrado por `\n`, na porta 5000. Exemplos:

```
RPI -> ESP32: CMD|START
ESP32 -> RPI: ACK|START|OK
ESP32 -> RPI: EVENT|STATE|DESLIGADA|INICIANDO|COMANDO_START
ESP32 -> RPI: DATA|STATE=NORMAL|TEMP_C=25.0|HUM_PCT=60.0|LOAD_PCT=50|DIST_MM=400|DHT_OK=1|TOF_OK=1|REASON=OPERACAO_NORMAL
ESP32 -> RPI: PING|1|23450
RPI -> ESP32: PONG|1|1794800000
```

`DATA` é enviado a cada 1 s, mesmo quando `DESLIGADA`. `EVENT` relata transições de estados e causa. Não há autenticação nem TLS: **uso exclusivo em rede local confiável**; não expor a porta TCP na Internet.

## Testes automatizados do núcleo de estados (opcional)

Em Linux com g++:

```bash
g++ -std=c++11 -Wall -Wextra -Werror -Iesp32_maquina_industrial \
    testes/test_maquina.cpp esp32_maquina_industrial/MaquinaEstados.cpp \
    -o /tmp/test_maquina
/tmp/test_maquina
```

Testa partida, tempo de inicialização, alerta, histerese, falha travada, reset, distância crítica e falha de sensor. Não substitui teste em ESP32 físico.

## Limitações e próximos passos

- Entradas e saídas são demonstrativas; o servo não é um motor industrial.
- Sem dispositivo de parada de emergência independente, watchdog de aplicação, redundância ou hardware de proteção.
- Sem autenticação ou TLS em TCP; também não há reconciliação de eventos perdidos durante desconexão (status periódico resolve estado atual).
- Reconexões TCP podem causar pausas curtas no loop; para restrições de tempo real reais, isolar rede em tarefa FreeRTOS de menor prioridade e usar controlador físico de segurança.
- Futuro: dashboard web na Raspberry, SQLite, medições de latência e logs históricos.
