#include <Arduino.h>
#include <SPI.h>
#include <FS.h>
#include <SD.h>

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
 
}

void loop() {
    Serial.print("MOSI: ");
  Serial.println(MOSI);
  Serial.print("MISO: ");
  Serial.println(MISO);
  Serial.print("SCK: ");
  Serial.println(SCK);
  Serial.print("SS: ");
  Serial.println(SS);  // put your main code here, to run repeatedly:
}