import RPi.GPIO as GPIO
import time
import random
import threading

GPIO.setmode(GPIO.BCM)
GPIO.setup(17, GPIO.OUT)

# Função para piscar o LED
def gpio_blink():
    print("Tarefa de Blink LED iniciada")

    #GPIO.setmode(GPIO.BCM)
    #GPIO.setup(17, GPIO.OUT)

    while True:
        GPIO.output(17, GPIO.HIGH)
        time.sleep(1)

        GPIO.output(17, GPIO.LOW)
        time.sleep(1)


# Função para gerar e exibir uma contagem aleatória
def random_count():
    print("Tarefa de Contagem Aleatória iniciada")

    while True:
        count = random.randint(1, 100)
        print(f"Contagem aleatória: {count}")
        time.sleep(2)


# Criação das threads
thread1 = threading.Thread(target=gpio_blink)
thread2 = threading.Thread(target=random_count)

# Programa principal
try:
    thread1.start()
    thread2.start()
    thread1.join()
    thread2.join()

except KeyboardInterrupt:
    print("Programa interrompido.")

finally:
    GPIO.cleanup()
