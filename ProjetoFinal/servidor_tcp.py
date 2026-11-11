#!/usr/bin/env python3
"""Servidor TCP de teste para Raspberry Pi (somente biblioteca padrao)."""

from datetime import datetime
import socket
import time

HOST = "0.0.0.0"  # Aceita conexoes vindas da rede local.
PORT = 5000
TIMEOUT_CLIENTE_S = 15


def log(mensagem: str) -> None:
    horario = datetime.now().strftime("%H:%M:%S")
    print(f"[{horario}] {mensagem}", flush=True)


def enviar(conexao: socket.socket, mensagem: str) -> None:
    conexao.sendall((mensagem + "\n").encode("utf-8"))
    log(f"RPI -> ESP32: {mensagem}")


def atender_cliente(conexao: socket.socket, endereco: tuple) -> None:
    conexao.settimeout(TIMEOUT_CLIENTE_S)
    log(f"ESP32 conectado: {endereco[0]}:{endereco[1]}")

    enviar(conexao, "HELLO|RPI")
    enviar(conexao, "CMD|STATUS")  # Demonstra um comando iniciado pela Raspberry Pi.

    # TCP e um fluxo de bytes: '\n' delimita cada mensagem do protocolo.
    with conexao.makefile("r", encoding="utf-8", newline="\n") as leitor:
        while True:
            try:
                linha = leitor.readline()
            except (TimeoutError, OSError) as erro:
                log(f"Cliente sem dados ou conexao interrompida: {erro}")
                break

            if not linha:  # Cliente fechou a conexao.
                break

            mensagem = linha.strip()
            if not mensagem:
                continue
            if len(mensagem) > 200:
                enviar(conexao, "ERRO|MENSAGEM_LONGA")
                continue

            log(f"ESP32 -> RPI: {mensagem}")
            campos = mensagem.split("|")

            if (
                len(campos) == 3
                and campos[0] == "PING"
                and campos[1].isdigit()
                and campos[2].isdigit()
            ):
                numero = campos[1]
                enviar(conexao, f"PONG|{numero}|{int(time.time())}")

            elif len(campos) >= 2 and campos[0] == "STATUS" and campos[1] == "ESP32":
                log("Status do ESP32 recebido com sucesso.")

            else:
                enviar(conexao, "ERRO|COMANDO_INVALIDO")

    log("ESP32 desconectado. Aguardando nova conexao...")


def main() -> None:
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as servidor:
        servidor.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        servidor.bind((HOST, PORT))
        servidor.listen(1)
        log(f"Servidor TCP iniciado em {HOST}:{PORT}")
        log("Aguardando conexao do ESP32... (Ctrl+C para encerrar)")

        try:
            while True:
                conexao, endereco = servidor.accept()
                with conexao:
                    try:
                        atender_cliente(conexao, endereco)
                    except (OSError, UnicodeError) as erro:
                        log(f"Falha na conexao com {endereco[0]}: {erro}")
        except KeyboardInterrupt:
            log("Servidor encerrado pelo usuario.")


if __name__ == "__main__":
    main()
