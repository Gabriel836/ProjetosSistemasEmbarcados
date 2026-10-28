import RPI.GPIO as GPIO
from time import sleep

LED_PIN = 23

GPIO.setmode(GPIO.BCM)

while (1):
    GPIO.output(LED_PIN, GPIO.LOW)
    sleep(1)
    GPIO.output(LED_PIN, GPIO.HIGH)
    sleep(1)
