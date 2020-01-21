#include <Arduino.h>
#include "AdvancedUnitProtocol.hpp"
#include "SoftwareSerial.h"

AdvancedUnitProtocol ap(A0, A1, 57600);

uint8_t Tdata[256], Rdata[256];

void setup()
{
  // put your setup code here, to run once:
  Serial.begin(256000);

  Tdata[0] = 32;
  Tdata[1] = 85;
  Tdata[2] = 109;
  Tdata[3] = 75;
  Tdata[4] = 49;
  while (1)
  {
    Tdata[0]++;
    Serial.print(ap.work(5, Tdata, Rdata));
    Serial.print('\t');
    Serial.print(Rdata[0]);
    Serial.print('\t');
    Serial.print(Rdata[1]);
    Serial.print('\t');
    Serial.print(Rdata[2]);
    Serial.print('\t');
    Serial.print(Rdata[3]);
    Serial.print('\t');
    Serial.print(Rdata[4]);
    Serial.println("");
  }
}

void loop()
{
  // put your main code here, to run repeatedly:
}