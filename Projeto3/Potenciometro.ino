#include <Wire.h>

int analogPin = A0;
int val = 0;

void setup() {
  // Join I2C bus as slave with address 8
  Wire.begin(0x8);
  
  Wire.onRequest(requestEvent);

  Serial.begin(9600);
  
  // Setup pin 13 as output and turn LED off
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
}

void requestEvent() {
  Wire.write(map(val, 0, 1023, 0, 255));
}

void loop() {
  val = analogRead(analogPin);
  Serial.println(val);
  delay(200);
}