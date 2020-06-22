#include <helper.h>

void waitSeconds(const char* msg, uint8_t sec)
{
  Serial.println((const __FlashStringHelper*)msg);
  while (sec--) {
    Serial.print(F("."));
    delay(1000);
  }
  Serial.println();
}