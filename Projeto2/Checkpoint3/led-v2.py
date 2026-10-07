import RPi.GPIO as GPIO
import time
import random
import threading


# Evento compartilhado para encerrar as threads
stop_event = threading.Event()


# Função para piscar o LED
def gpio_blink():
    print("Tarefa de Blink LED iniciada")

    while not stop_event.is_set():
        GPIO.output(11, GPIO.HIGH)
        print("LED aceso")

        # Espera 1 segundo, mas pode ser interrompido
        if stop_event.wait(1):
            break

        GPIO.output(11, GPIO.LOW)
        print("LED apagado")

        if stop_event.wait(1):
            break


# Função para gerar contagens aleatórias
def random_count():
    print("Tarefa de Contagem Aleatória iniciada")

    while not stop_event.is_set():
        count = random.randint(1, 100)
        print(f"Contagem aleatória: {count}")

        if stop_event.wait(2):
            break


# Programa principal
GPIO.setmode(GPIO.BOARD)
GPIO.setup(11, GPIO.OUT)

# Criação das threads
thread_blink = threading.Thread(target=gpio_blink)
thread_count = threading.Thread(target=random_count)

try:
    # Inicia as duas tarefas
    thread_blink.start()
    thread_count.start()

    # Mantém o programa principal esperando as threads
    thread_blink.join()
    thread_count.join()

except KeyboardInterrupt:
    print("\nPrograma interrompido.")

    # Sinaliza para as threads encerrarem
    stop_event.set()

    # Espera as threads terminarem
    thread_blink.join()
    thread_count.join()

finally:
    GPIO.output(11, GPIO.LOW)
    GPIO.cleanup()

    print("GPIO liberado.")
