#!/usr/bin/env python3
"""Servidor TCP / terminal de supervisao para o ESP32 (apenas Python 3 padrao).

Comandos digitados no terminal: start, stop, reset, status, help, quit.
Sem interface Web: objetivo desta etapa e testar firmware + maquina de estados.
"""
import queue
import socket
import threading
import time
from datetime import datetime

HOST = "0.0.0.0"
PORT = 5000
COMANDOS = {"start", "stop", "reset", "status"}
entrada = queue.Queue()


def log(mensagem: str) -> None:
    print(f"[{datetime.now():%H:%M:%S}] {mensagem}", flush=True)


def ler_comandos() -> None:
    print("Comandos: start, stop, reset, status, help, quit", flush=True)
    while True:
        try:
            texto = input().strip().lower()
        except EOFError:
            return
        if texto in COMANDOS or texto == "quit":
            entrada.put(texto)
        elif texto == "help":
            print("start | stop | reset | status | quit", flush=True)
        elif texto:
            log("Comando invalido. Digite help.")


def enviar(cliente: socket.socket, mensagem: str) -> None:
    cliente.sendall((mensagem + "\n").encode("ascii"))
    log("RPI -> ESP32: " + mensagem)


def processar(cliente: socket.socket, mensagem: str) -> None:
    campos = mensagem.split("|")
    if campos[0] == "PING" and len(campos) == 3 and campos[1].isdigit():
        enviar(cliente, f"PONG|{campos[1]}|{int(time.time())}")
    elif campos[0] == "DATA":
        # Periodico. Util para observar a maquina em tempo real.
        log("DADOS: " + " ".join(campos[1:]))
    elif campos[0] == "EVENT":
        log("TRANSICAO: " + " -> ".join(campos[2:4]) +
            (f" ({campos[4]})" if len(campos) > 4 else ""))
    elif campos[0] in {"STATUS", "ACK", "NACK"}:
        log("ESP32 -> RPI: " + mensagem)
    else:
        log("ESP32 -> RPI (desconhecido): " + mensagem)


def servir_conexao(cliente: socket.socket, endereco: tuple) -> bool:
    """Retorna False se usuario pediu encerrar."""
    cliente.settimeout(0.2)
    log(f"ESP32 conectado: {endereco[0]}:{endereco[1]}")
    enviar(cliente, "HELLO|RPI")
    enviar(cliente, "CMD|STATUS")
    buffer = b""

    while True:
        while not entrada.empty():
            comando = entrada.get_nowait()
            if comando == "quit":
                return False
            enviar(cliente, "CMD|" + comando.upper())
        try:
            pacote = cliente.recv(512)
        except socket.timeout:
            continue
        except OSError as erro:
            log(f"Conexao encerrada: {erro}")
            return True
        if not pacote:
            log("ESP32 desconectado.")
            return True
        buffer += pacote
        if len(buffer) > 4096:
            log("Buffer TCP excessivo. Desconectando.")
            return True
        while b"\n" in buffer:
            linha, buffer = buffer.split(b"\n", 1)
            linha = linha.strip(b"\r \t")
            if not linha:
                continue
            if len(linha) > 250:
                log("Mensagem longa descartada.")
                continue
            try:
                processar(cliente, linha.decode("ascii"))
            except UnicodeDecodeError:
                log("Mensagem nao ASCII descartada.")


def main() -> None:
    threading.Thread(target=ler_comandos, daemon=True).start()
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as servidor:
        servidor.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        servidor.bind((HOST, PORT))
        servidor.listen(1)
        servidor.settimeout(0.5)
        log(f"Servidor em {HOST}:{PORT}. Aguardando ESP32...")
        try:
            while True:
                if not entrada.empty() and entrada.queue[0] == "quit":
                    break
                try:
                    cliente, endereco = servidor.accept()
                except socket.timeout:
                    # Descartar comandos digitados enquanto o ESP32 estiver offline.
                    while not entrada.empty():
                        comando = entrada.get_nowait()
                        if comando == "quit":
                            return
                        log(f"ESP32 offline; '{comando}' nao enviado.")
                    continue
                with cliente:
                    try:
                        if not servir_conexao(cliente, endereco):
                            break
                    except (ConnectionError, OSError) as erro:
                        log(f"Conexao interrompida: {erro}")
        except KeyboardInterrupt:
            pass
    log("Servidor finalizado.")


if __name__ == "__main__":
    main()
