#include <Arduino.h>

#ifndef BUFFER_HPP
#define BUFFER_HPP

/*******************************************************
 * DEFINITIONS
 *******************************************************/

#define MAX_BUFFER 256

extern char _buff[]; // buffer

#define DUMP_BUFFER(x)            \
  Serial.print(F("hex: "));       \
  for (int i = 0; x[i]; i++) {    \
    Serial.print(F("#"));         \
    Serial.print((int)x[i], HEX); \
    Serial.print(F(", "));        \
  }                               \
  Serial.print(F("\nlen: "));     \
  Serial.println(strlen(x));

/*******************************************************
 * BUFFER FUNCTIONS
 *******************************************************/

char* getBuffer(void);
void clearBuff(bool);
char* buffFindHelper(char*, char*&, const char*);
int8_t buffFind(char* buff,
                const char* target1,
                const char* target2 = NULL,
                const char* target3 = NULL);
// int8_t getBoolAfterStr(const char* target);
int16_t getIntAfterStr(const char* target);
int16_t getIntAfterStartStr(const char* start, const char* target);
void postRecquest(const char* host,
                  const char* path,
                  const char* data);

#endif
