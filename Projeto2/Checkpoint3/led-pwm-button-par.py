import RPi.GPIO as GPIO
import threading
import queue
import time


# ============================================================
# Configuração dos pinos físicos
# ============================================================

LED_PIN = 11
BUTTON_INC = 13
BUTTON_SET = 15


# ============================================================
# Comunicação entre as threads
# ============================================================

# Fila utilizada para enviar um novo duty cycle
# da thread dos botões para a thread do LED
pwm_queue = queue.Queue()

# Evento usado para encerrar as threads
stop_event = threading.Event()


# ============================================================
# Thread 1 - Controle do LED
# ============================================================

def led_thread():
    print("Thread do LED iniciada")

    # Frequência do PWM em Hz
    pwm = GPIO.PWM(LED_PIN, 100)

    # Duty cycle inicial
    duty_cycle = 0

    pwm.start(50)

    try:
        while not stop_event.is_set():

            # Verifica se a outra thread enviou
            # um novo valor de PWM
            try:
                duty_cycle = pwm_queue.get_nowait()

                print(
                    f"[LED] Novo duty cycle recebido: "
                    f"{duty_cycle}%"
                )

            except queue.Empty:
                pass

            # LED ligado com o duty cycle selecionado
            pwm.ChangeDutyCycle(duty_cycle)

            # Mantém ligado por 0,5 s
            if stop_event.wait(0.5):
                break

            # LED apagado
            pwm.ChangeDutyCycle(0)

            # Mantém apagado por 0,5 s
            if stop_event.wait(0.5):
                break

    finally:
        pwm.stop()

        print("Thread do LED encerrada")


# ============================================================
# Thread 2 - Controle dos botões
# ============================================================

def button_thread():
    print("Thread dos botões iniciada")

    # Valor sendo escolhido pelo usuário.
    # Ele só será enviado para o LED quando SET for pressionado.
    selected_duty = 0

    while not stop_event.is_set():

        # ----------------------------------------------------
        # Botão de incremento
        # ----------------------------------------------------

        if GPIO.input(BUTTON_INC) == GPIO.LOW:

            selected_duty += 10

            # Se ultrapassar 100%, volta para 0%
            if selected_duty > 100:
                selected_duty = 0

            print(
                f"[BOTÃO +] Duty cycle selecionado: "
                f"{selected_duty}%"
            )

            # Espera o botão ser solto
            while (
                GPIO.input(BUTTON_INC) == GPIO.LOW
                and not stop_event.is_set()
            ):
                time.sleep(0.01)

            # Debounce
            time.sleep(0.05)


        # ----------------------------------------------------
        # Botão SET
        # ----------------------------------------------------

        if GPIO.input(BUTTON_SET) == GPIO.LOW:

            print(
                f"[SET] Duty cycle confirmado: "
                f"{selected_duty}%"
            )

            # Envia o valor para a thread do LED
            pwm_queue.put(selected_duty)

            # Espera o botão ser solto
            while (
                GPIO.input(BUTTON_SET) == GPIO.LOW
                and not stop_event.is_set()
            ):
                time.sleep(0.01)

            # Debounce
            time.sleep(0.05)


        # Evita ocupar 100% da CPU
        time.sleep(0.01)

    print("Thread dos botões encerrada")


# ============================================================
# Programa principal
# ============================================================

GPIO.setmode(GPIO.BOARD)

# LED
GPIO.setup(LED_PIN, GPIO.OUT)

# Botões usando resistores pull-up internos
GPIO.setup(BUTTON_INC, GPIO.IN, pull_up_down=GPIO.PUD_UP)
GPIO.setup(BUTTON_SET, GPIO.IN, pull_up_down=GPIO.PUD_UP)


# Criação das threads
thread_led = threading.Thread(target=led_thread)
thread_buttons = threading.Thread(target=button_thread)


try:

    # Inicia as duas threads
    thread_led.start()
    thread_buttons.start()

    print("Programa iniciado")
    print("Botão + : aumenta o duty cycle em 10%")
    print("Botão SET: envia o valor para o LED")
    print("Pressione Ctrl+C para encerrar")

    # Mantém a thread principal viva
    while True:
        time.sleep(1)


except KeyboardInterrupt:

    print("\nEncerrando programa...")

    # Solicita encerramento das threads
    stop_event.set()


finally:

    # Espera as threads terminarem
    thread_led.join()
    thread_buttons.join()

    GPIO.output(LED_PIN, GPIO.LOW)
    GPIO.cleanup()

    print("GPIO liberado")
