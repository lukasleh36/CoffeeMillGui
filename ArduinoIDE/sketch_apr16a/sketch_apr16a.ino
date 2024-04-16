#include <Arduino.h>

#define LEDPIN 4

void setup() {
  // put your setup code here, to run once:
pinMode(LEDPIN, OUTPUT);
}

void loop() {
  digitalWrite(LEDPIN, HIGH);
  delay(1000);
  digitalWrite(LEDPIN, LOW);
  // put your main code here, to run repeatedly:

}
