#include <Arduino.h>
#include <SoftwareSerial.h>
#include <buffer.h>
#include <helper.hpp>

#ifndef A6_2_H
#define A6_2_H

// #define DEBUG 1 // debug verbose level (0, 1, 2)

#define A6_RX        11   // This is your RX-Pin on arduino
#define A6_TX        12   // This is your TX-Pin on arduino
#define A6_BAUD_RATE 9800 // Baudrate between Arduino and A6

#define A6_DEFAULT_DELAY      1000
#define A6_DEFAULT_TIMEOUT    1000
#define A6_DEFAULT_REPETITION 1

// #define PSTR char *;
#define CPSTR const char*
// typedef const __FlashStringHelper* flashStr;
// #define CFP(x) (reinterpret_cast<flashStr>(x))
#define CF(x) (reinterpret_cast<const __FlashStringHelper*>(x))
// #define CPSTRF(x) (reinterpret_cast<CPSTR>(F(x)))

// extern SoftwareSerial _board;

// serial
void begin(long speed);
void setTimeout(unsigned int timeout = A6_DEFAULT_TIMEOUT);

// stream
void _flush(void);
uint8_t readLine(bool print = false);
void commandSend(CPSTR cmd);
int8_t command(CPSTR cmd,
               //  CPSTR response1     = "OK",
               //  CPSTR response2     = "ERROR",
               //  CPSTR response3     = NULL,
               //  uint16_t timeout    = A6_DEFAULT_TIMEOUT,
               //  uint8_t repetitions = A6_DEFAULT_REPETITION);
               CPSTR response1,
               CPSTR response2,
               CPSTR response3,
               uint16_t timeout,
               uint8_t repetitions);
int8_t command(CPSTR cmd,
               CPSTR response1,
               CPSTR response2,
               uint16_t timeout    = A6_DEFAULT_TIMEOUT,
               uint8_t repetitions = A6_DEFAULT_REPETITION);
int8_t command(CPSTR cmd,
               CPSTR response1,
               uint16_t timeout    = A6_DEFAULT_TIMEOUT,
               uint8_t repetitions = A6_DEFAULT_REPETITION);
int8_t command(CPSTR cmd,
               uint16_t timeout    = A6_DEFAULT_TIMEOUT,
               uint8_t repetitions = A6_DEFAULT_REPETITION);

// helper
int8_t checkSignal(uint8_t repetition);
int8_t openTCP(CPSTR host);
int8_t closeTCP(void);

int8_t initModule();
int8_t initNetwork(void);
int8_t initGPRS();
int8_t httpGet(CPSTR host, CPSTR path, CPSTR target);
// int8_t getSettings(CPSTR host, CPSTR path, CPSTR target);
// int8_t httpPost(CPSTR host, CPSTR path, CPSTR data);
int8_t httpPost(CPSTR host,
                CPSTR path,
                uint8_t deviceNumber,
                uint8_t co2,
                uint8_t humidity,
                uint8_t temperature);

#endif
