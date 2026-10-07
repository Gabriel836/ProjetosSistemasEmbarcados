from gpiozero import DistanceSensor
from time import sleep

sensor = DistanceSensor(23,24)

while True:
    print('Distancia do objeto mais próximo é', sensor.distance, 'm')
    sleep(1)
