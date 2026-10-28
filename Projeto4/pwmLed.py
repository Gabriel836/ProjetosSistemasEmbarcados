from gpiozero import PWMLED
from time import sleep

led = PWMLED(18)

while True:
    led.value = 0
    sleep(1)
    led.value = 0.5
    sleep(1)
    led.value = 1
    sleep(1)
    led.value = 0
    led.pulse()
    sleep(1)
