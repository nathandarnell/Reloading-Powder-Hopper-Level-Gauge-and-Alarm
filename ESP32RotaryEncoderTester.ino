#include <Arduino.h>

// Rotary encoder is wired to 
const int encClk = 4;
const int encDt = 15;

// variable for storing the potentiometer value
int encValue = 0;
float printValue = 0;

void setup()
{
  Serial.begin(115200);
  delay(1000);
}

void loop()
{
  encValue = analogRead(encClk);
  // Use if you want to convert to volts
  // printValue = 3.3 / 4096 * encValue;
  printValue = encValue;
  Serial.print(printValue);

  Serial.print(" - ");

  encValue = analogRead(encDt);
  // Use if you want to convert to volts
  // printValue = 3.3 / 4096 * encValue;
  printValue = encValue;
  Serial.println(printValue);
  
  delay(500);
}
