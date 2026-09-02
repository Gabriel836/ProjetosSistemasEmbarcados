import RPi.GPIO as GPIO
import time

# GPIO usados
BUTTON_PIN = 4
LED_PIN = 17

GPIO.setmode(GPIO.BCM)

# Desabilita avisos
GPIO.setwarnings(False)


def button_callback(pin):

    if GPIO.input(pin):
        GPIO.output(LED_PIN, GPIO.LOW)

    else:
        GPIO.output(LED_PIN, GPIO.HIGH)


GPIO.setup(LED_PIN, GPIO.OUT)

# Garante que o LED comece apagado
GPIO.output(LED_PIN, GPIO.LOW)


GPIO.setup(
    BUTTON_PIN,
    GPIO.IN,
    pull_up_down=GPIO.PUD_UP
)

GPIO.add_event_detect(
    BUTTON_PIN,
    GPIO.BOTH,
    callback=button_callback,
    bouncetime=200
)


try:

    while True:
        pass


except KeyboardInterrupt:

    print("\nPrograma encerrado pelo usuario.")


finally:

    GPIO.cleanup()