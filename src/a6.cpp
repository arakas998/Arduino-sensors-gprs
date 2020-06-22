#include <A6.h>
#include <buffer.h>

/*******************************************************
 * VARIABLES
 *******************************************************/

static const char END_C[] PROGMEM   = { 0x1a, 0x00 };
static const char CGDCONT[] PROGMEM = "AT+CGDCONT=1,\"IP\",\"internet.ht.hr\"";

// const char* _apn;
SoftwareSerial _board(A6_RX, A6_TX);

/*******************************************************
 * SERIAL FUNCTIONS
 *******************************************************/

void begin(long speed)
{
  _board.begin(speed);
}

void setTimeout(unsigned int timeout)
{
  _board.setTimeout(timeout);
}

void resetModem()
{
  command(PSTR("AT"), 1000, 20);
  commandSend(PSTR("AT+CFUN=1"));
}

/*******************************************************
 * STREAM FUNCTIONS
 *******************************************************/

/**
 * @brief flush serial buffer
 * 
 */
void _flush()
{
  while (_board.available()) _board.read();
}

/**
 * @brief Read one line from stream
 * 
 * @param print dump buffer on console
 * @return uint8_t number of bytes placed in buffer
 */
uint8_t readLine(bool print)
{
  // uint8_t c;
  // while (1) {
  //   c = _board.peek();
  //   if (c != 10 && c != 13) break;
  //   _board.read();
  // }

  // remove first ocurance of CR and LF from the stream
  // for (uint8_t c = _board.peek(); c == 10 || c == 13; c = _board.peek()) {
  //   Serial.print("c: ");
  //   Serial.println(c);
  //   _board.read();
  // }
  // if (!_board.available()) return 0;

  uint8_t last = _board.readBytesUntil('\n', _buff, MAX_BUFFER);
  _buff[last]  = 0x00;

  if (print) // print if there is at least one char
    Serial.println(_buff);

#if DEBUG > 1
  DUMP_BUFFER(_buff)
#endif

  return last;
}

/*******************************************************
 * COMMAND FUNCTIONS
 *******************************************************/

/**
 * @brief send comand to module via serial
 * 
 * @param cmd AT command, pointer to PROGMEM string
 */
void commandSend(CPSTR cmd)
{
  _flush();        // Empty stream buffer
  clearBuff(true); // Clear local buffer

  // Send to hardware serial
  Serial.print(F("\ncmd: "));
  Serial.println((const __FlashStringHelper*)cmd);

  //Send to software serial
  _board.println((const __FlashStringHelper*)cmd);
}

/**
 * @brief send AT command to modem and wait for response
 * 
 * @param cmd AT command, pointer to PROGMEM string
 * @param response1 wait for response 1
 * @param response2 wait for response 2
 * @param response3 wait for response 3
 * @param timeout timeout in mils
 * @param repetitions trial number
 * @return int8_t -1 - timeout
                   0 - found first response
                   1 - found second response
                   2 - found third response
 */
int8_t command(CPSTR cmd,
               CPSTR response1,
               CPSTR response2,
               CPSTR response3,
               CPSTR response4,
               uint16_t timeout,
               uint8_t repetitions)
{
  int8_t success = -1;
  int8_t n       = 0;

  response1&& n++;
  response2&& n++;
  response3&& n++;
  response4&& n++;

  _board.setTimeout(timeout); // set timeout

  while (repetitions--) { // outer loop, repetition
    commandSend(cmd);     // send command to serial

#if DEBUG > 0
    print(Serial, F("rep: "), repetitions);
#endif

    struct Stream::MultiTarget t[4] = {
      { response1, strlen(response1), 0 },
      { response2, strlen(response2), 0 },
      { response3, strlen(response3), 0 },
      { response4, strlen(response4), 0 },
    };

    if ((success = _board.findMulti(t, n)) > -1) {
      // print(Serial, F("res: "), t[success].str);
      break; // break if response found
    }
  }
  _board.setTimeout(A6_DEFAULT_TIMEOUT); // restore timeout
  return success;                        // index response found
}

int8_t command(CPSTR cmd,
               CPSTR response1,
               CPSTR response2,
               CPSTR response3,
               uint16_t timeout,
               uint8_t repetitions)
{
  return command(cmd, response1, response2, response3, NULL, timeout, repetitions);
}

int8_t command(CPSTR cmd,
               CPSTR response1,
               CPSTR response2,
               uint16_t timeout,
               uint8_t repetitions)
{
  return command(cmd, response1, response2, NULL, timeout, repetitions);
}

int8_t command(CPSTR cmd,
               CPSTR response1,
               uint16_t timeout,
               uint8_t repetitions)
{
  return command(cmd, response1, NULL, NULL, timeout, repetitions);
}

int8_t command(CPSTR cmd,
               uint16_t timeout,
               uint8_t repetitions)
{
  return command(cmd, "OK", NULL, NULL, timeout, repetitions);
}

/*******************************************************
  * HELPER FUNCTIONS
  *******************************************************/

/**
 * @brief check mobile signal
 * 
 * @param repetition trial number
 * @return int8_t -1 - poor signal
                   0 - signal ok
 */
int8_t checkSignal(uint8_t repetition)
{
  while (repetition--) {
    if (command(PSTR("AT+CSQ"), "+CSQ: ") == -1) return -1;
    if (_board.parseInt() > 10) {
      print(Serial, F("Mobile signal OK"));
      return 0;
    } else
      delay(1000);
  }
  print(Serial, F("Poor mobile signal"));
  return -1;
}

int8_t closeTCP()
{
  // check if TCP is opened, if we get any response at index 3
  // we can asume that TCP is closed
  if (command(PSTR("AT+CIPSTATUS"),
              "IP CLOSE",
              "IP INITIAL",
              "IP GPRSACT",
              "CONNECT OK", 1000, 1)
      == 3)
    command(PSTR("AT+CIPCLOSE"), "OK", "ERROR", 1000, 1);
  return 0;
}

int8_t openTCP(CPSTR host)
{
  // close TCP
  closeTCP();

  command(PSTR("AT"));

  // open TCP
  _board.print(F("AT+CIPSTART=\"TCP\",\""));
  _board.print(CF(host));
  if (command(PSTR("\",80"), "OK", 10000, 1) == -1) return -1;
  return 0;
}

/*******************************************************
  * MODULE FUNCTIONS
  *******************************************************/

int8_t initModule()
{
  // check if module is ready, set echo off
  // and verbose errors on
  if (command(PSTR("AT"), 1000, 20) == -1) return -1;
  if (command(PSTR("ATE0")) == -1) return -1;
  if (command(PSTR("AT+CMEE=2")) == -1) return -1;
  return 0;
}

int8_t initNetwork()
{
  // check mobile signal
  if (checkSignal(30) == -1) return -1;

  // check if module is registered on T_MOBILE
  if (command(PSTR("AT+COPS?"), "1,2,\"21901\"", 1000, 20) == -1) return -1;

  // TODO Network registration to "HT-MOBILE"

  // check if module is registered on network
  if (command(PSTR("AT+CREG?"), "+CREG: 1,1", 1000, 20) == -1) return -1;

  return 0;
}

int8_t initGPRS()
{
  // first check if PDP context with CID 1 is activated,
  // it usually means that GPRS is active so we skip the rest
  if (command(PSTR("AT+CGACT?"), "+CGACT: 1, 1", 1000, 1) == 0)
    // chevk if PDP conetxt is valid
    if (command(PSTR("AT+CGDCONT?"), "internet.ht.hr", 1000, 1) == 0)
      return 0;

  // define a PDP context with CID 1
  if (command(CGDCONT) == -1) return -1;

  // activate the PDP context with CID 1
  if (command(PSTR("AT+CGACT=1,1"), 20000, 1) == -1) return -1;

  return 0;
}

int8_t httpGet(CPSTR host, CPSTR path, CPSTR target)
{
  // open TCP
  if (openTCP(host) == -1) return -1;

  // send data
  if (command(PSTR("AT+CIPSEND"), ">", 10000, 1) == -1) return -1;

  _board.print(F("GET "));
  _board.print(CF(path));
  _board.print(F(" HTTP/1.1\r\n"));
  _board.print(F("HOST: "));
  _board.print(CF(host));
  _board.print(F("\r\n"));
  _board.print(F("Connection: close\r\n\r\n"));

  // end data send
  if (command(END_C, target, 20000, 1) == -1) return -1;

  return 0;
}

int8_t httpPost(CPSTR host,
                CPSTR path,
                uint8_t deviceNumber,
                uint8_t co2,
                uint8_t humidity,
                uint8_t temperature)
{
  // open TCP
  if (openTCP(host) == -1) return -1;

  // get conten size
  uint8_t n = 46;
  n += numDigits(deviceNumber);
  n += numDigits(co2);
  n += numDigits(humidity);
  n += numDigits(temperature);

  // send data
  if (command(PSTR("AT+CIPSEND"), ">", 10000, 1) == -1) return -1;

  // HTTP header
  print(_board, F("POST "), CF(path), F(" HTTP/1.1"));
  print(_board, F("HOST: "), CF(host));
  print(_board, F("Content-Type: application/json"));
  print(_board, F("Content-Length: "), n, F("\r\n"));

  // HTTP content
  print(_board,
        F("{\"arduino\":"), deviceNumber,
        F(",\"co2\":"), co2,
        F(",\"humidity\":"), humidity,
        F(",\"temperature\":"), temperature, F("}"));

  // _board.print(F("POST /api/data HTTP/1.1\r\nHost: www.europe-west1-arduino-sensors-754e5.cloudfunctions.net\r\nContent-Type: application/json\r\nContent-Length: 50\r\n\r\n{\"arduino\":1,\"co2\":1,\"humidity\":2,\"temperature\":3}\r\n"));

  // end data send
  if (command(END_C, "+CIPRCV", 20000, 1) == -1) return -1;
  readLine(true);

  return 0;
}
