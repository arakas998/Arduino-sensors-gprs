#include <Arduino.h>
// #include <MemoryFree.h>
#include <dht11.h>
#include <helper.h>
#include <a6.h>
#include <buffer.h>

#define DEBUG 1 // debug verbose level (0, 1, 2)

#define SERIAL_BAUD        9600  // Baudrate between Arduino and PC
#define DEFAULT_INTERVAL   10000 // Default interval
#define SHOW_TIME_INTERVAL 1000  // Default interval

#define DEVICE_NUMBER   1  // device number
#define BTN_SEND        2  // send button pin
#define BTN_STOP        3  // stop button pin
#define LED_RED_PIN     4  // red led
#define LED_GREEN_PIN   5  // green led
#define LED_YELLOW_PIN  6  // yellow led
#define LED_WHITE_PIN   7  // white led
#define LED_GPRS_PIN    8  // GPRS status indicator
#define LED_NETWORK_PIN 9  // Network status indicator
#define LED_MODULE_PIN  10 // Module status indicator
#define DHT11_1_PIN     14 // DHT11 sensor
#define FAN_PIN         15 // fan
#define RST_PIN         15 // A6 module reset pin

static const char HOST[] PROGMEM
    = "www.europe-west1-arduino-sensors-754e5.cloudfunctions.net";
static const char SETTINGS_API[] PROGMEM = "/api/settings/1";
static const char DATA_API[] PROGMEM     = "/api/data";

struct button_t {
  uint8_t pin                    = 0;   // pin number on board
  uint8_t state                  = LOW; // the current reading from the input pin
  uint8_t lastState              = LOW; // the previous reading from the input pin
  uint8_t debounceDelay          = 50;  // the debounce time; increase if flickers
  unsigned long lastDebounceTime = 0;   // the last time the output pin was toggled
} buttonSend, buttonStop;

struct sensor_t {
  uint8_t vallue    = 0;
  uint8_t lastValue = 0;
  uint8_t min       = 20;
  uint8_t max       = 80;
};

struct settings_t {
  uint8_t errors            = 0;
  uint8_t stop              = 0;
  uint8_t led               = 0;
  uint8_t fan               = 0;
  uint16_t updateInterval   = DEFAULT_INTERVAL;
  uint16_t showTimeInterval = SHOW_TIME_INTERVAL;
  struct sensor_t co2;
  struct sensor_t humidity;
  struct sensor_t temperature;
  uint64_t updateStart   = 0;
  uint64_t showTimeStart = 0;
} settings;

void showTime(void);
void readSendButton(void);
void readStopButton(void);
void setSensorLeds(void);
void resetModuleLeds(void);
bool buttonDebounce(button_t& button);
int8_t initModem();
int8_t postDataToAPI(bool force);
int8_t readSettingsFromBuffer(void);
int8_t getSettingsFormAPI(uint8_t read);
int8_t readDataFromSensors(void);

dht11 DHT11; // sensors class

void setup()
{
  // init buttons pins
  buttonSend.pin = BTN_SEND;
  buttonStop.pin = BTN_STOP;
  pinMode(buttonSend.pin, INPUT);
  pinMode(buttonStop.pin, INPUT);

  // init leds pins
  pinMode(LED_MODULE_PIN, OUTPUT);
  pinMode(LED_NETWORK_PIN, OUTPUT);
  pinMode(LED_GPRS_PIN, OUTPUT);
  pinMode(LED_WHITE_PIN, OUTPUT);
  pinMode(LED_GREEN_PIN, OUTPUT);
  pinMode(LED_YELLOW_PIN, OUTPUT);
  pinMode(LED_RED_PIN, OUTPUT);

  // DHT11
  // pinMode(DHT11_1_PIN, INPUT);

  // init fan pins
  pinMode(FAN_PIN, OUTPUT);
  pinMode(RST_PIN, OUTPUT);
  digitalWrite(RST_PIN, LOW);

  // init hardware serial
  Serial.begin(SERIAL_BAUD);

  // init A6 board (software serial)
  begin(A6_BAUD_RATE);

  // init modem
  if (initModem() == 0)
    resetModuleLeds();
  else
    settings.errors += 1;

  void setSensorLeds();

  // set starting time
  settings.updateStart = settings.showTimeStart = millis();
}

void loop()
{
  if (settings.errors > 2) {
    resetModem();
    waitSeconds(PSTR("Reseting modem"), 10);
    initModem();
    settings.errors      = 0;        // reset errors
    settings.updateStart = millis(); // reset start time
  }

  readSendButton();
  readStopButton();

#if 1
  // check if interval time is passed
  if (!settings.stop && (millis() - settings.updateStart) > settings.updateInterval) {

    // ! TODO need optimization

    // fetch settings from API
    if (getSettingsFormAPI(true) == 0)
      // post sensor data to API
      if (postDataToAPI(false) == 0) {
        resetModuleLeds();
        settings.errors = 0;
      } else {
        settings.errors += 1;
        if (initModem() == 0) resetModuleLeds();
      }
    else {
      settings.errors += 1;
      if (initModem() == 0) resetModuleLeds();
    }
    settings.updateStart = millis(); // reset start time
  }
#endif

  showTime();

  // while (1)
  delay(100);
}

void showTime()
{
  auto entry = millis();
  auto time  = entry - settings.showTimeStart;
  if (time > settings.showTimeInterval) {
    auto start = millis();
    print(Serial, start, F(" ms"));
    settings.showTimeStart = start;
  }
}

int8_t initModem()
{
  resetModuleLeds();

  // init module
  if (initModule() == -1) {
    print(Serial, F("Module error"));
    return -1;
  } else
    digitalWrite(LED_MODULE_PIN, HIGH);

  // init network
  if (initNetwork() == -1) {
    print(Serial, F("Network error"));
    return -2;
  } else
    digitalWrite(LED_NETWORK_PIN, HIGH);

  // init GPRS
  if (initGPRS() == -1) {
    print(Serial, F("GPRS error"));
    return -1;
  } else
    digitalWrite(LED_GPRS_PIN, HIGH);

  return 0;
}

void resetModule()
{
  digitalWrite(RST_PIN, LOW);
}

void readSendButton()
{
  // only execute if button state is changed to HIGH
  if (buttonDebounce(buttonSend) && buttonSend.state == HIGH) {
    postDataToAPI(true);
    // getSettingsFormAPI(true);
    // readDataFromSensors();
  }
}

void readStopButton()
{
  // only execute if button state is changed to HIGH
  if (buttonDebounce(buttonStop) && buttonStop.state == HIGH) {
    settings.stop = !settings.stop;
    print(Serial, F("STOP: "), settings.stop);
  }
}

int8_t postDataToAPI(bool force)
{
  if (readDataFromSensors() == -1) return -1;
  if (initModem() == -1) return -1;

  // post only if sensor data has changed
  if (!force
      && settings.co2.vallue == settings.co2.lastValue
      && settings.humidity.vallue == settings.humidity.lastValue
      && settings.temperature.vallue == settings.temperature.lastValue)
    return 1;

  // save last reading
  settings.co2.lastValue         = settings.co2.vallue;
  settings.humidity.lastValue    = settings.humidity.vallue;
  settings.temperature.lastValue = settings.temperature.vallue;

  // POST data to API
  if (httpPost(HOST,
               DATA_API,
               DEVICE_NUMBER,
               settings.co2.vallue,
               settings.humidity.vallue,
               settings.temperature.vallue)
      == -1) {
    print(Serial, F("HTTP POST error"));
    return -1;
  }

  return 0;
}

int8_t getSettingsFormAPI(uint8_t read)
{
  if (initModem() == -1) return -1;

  // fetch settings API
  if (httpGet(HOST, SETTINGS_API, "\n{") == -1) {
    print(Serial, F("HTTP GET error"));
    return -1;
  }

  // read settings form buffer
  if (read && readSettingsFromBuffer() == -1) return -1;

  return 0;
}

int8_t readSettingsFromBuffer()
{
  int16_t v;

  setTimeout(20000);
  readLine(true);
  setTimeout(1000);

  // led
  v = getIntAfterStr("led\":");
  if (v != -99) {
    settings.led = v;
    digitalWrite(LED_WHITE_PIN, v);
  }

  // fan
  v = getIntAfterStr("fan\":");
  if (v != -99) {
    settings.fan = v;
    digitalWrite(FAN_PIN, v);
  }

  // interval
  v = getIntAfterStr("updateInterval\":");
  if (v != -99) settings.updateInterval = v * 1000;

  // co2 min/max
  v = getIntAfterStartStr("co2\":", "min\":");
  if (v != -99) settings.co2.min = v;
  v = getIntAfterStartStr("co2\":", "max\":");
  if (v != -99) settings.co2.max = v;

  // humidity min/max
  v = getIntAfterStartStr("humidity\":", "min\":");
  if (v != -99) settings.humidity.min = v;
  v = getIntAfterStartStr("humidity\":", "max\":");
  if (v != -99) settings.humidity.max = v;

  // temperature min/max
  v = getIntAfterStartStr("temperature\":", "min\":");
  if (v != -99) settings.temperature.min = v;
  v = getIntAfterStartStr("temperature\":", "max\":");
  if (v != -99) settings.temperature.max = v;

#if DEBUG > 0
  print(Serial,
        F("\nSETTINGS: \n"),
        F("LED: "), settings.led, "\n",
        F("FAN: "), settings.fan, "\n",
        F("CO2(min,max): "), settings.co2.min, ", ", settings.co2.max, "\n",
        F("HUM(min,max): "), settings.humidity.min, ", ", settings.humidity.max, "\n",
        F("TEMP(min,max): "), settings.temperature.min, ", ", settings.temperature.max, "\n");
#endif

  return v == -99 ? -1 : 0;
}

int8_t readDataFromSensors()
{
  int chk = DHT11.read(DHT11_1_PIN); // read sensor

  if (chk != DHTLIB_OK) {
    Serial.println(F("Read sensor: ERROR"));
    return -1;
  }

  settings.co2.vallue         = rand() % 40 + 20; // generate random
  settings.humidity.vallue    = DHT11.humidity;
  settings.temperature.vallue = DHT11.temperature;

  print(Serial, F("CO2 (%): "), settings.co2.vallue);
  print(Serial, F("Humidity (%): "), settings.humidity.vallue);
  print(Serial, F("temperature (%): "), settings.temperature.vallue);

  setSensorLeds();

  return 0;
}

void setSensorLeds()
{
  if (settings.co2.vallue < settings.co2.min
      || settings.humidity.vallue < settings.humidity.min
      || settings.temperature.vallue < settings.temperature.min) {
    digitalWrite(LED_GREEN_PIN, LOW);
    digitalWrite(LED_RED_PIN, LOW);
    digitalWrite(LED_YELLOW_PIN, HIGH);
  } else if (settings.co2.vallue > settings.co2.max
             || settings.humidity.vallue > settings.humidity.max
             || settings.temperature.vallue > settings.temperature.max) {
    digitalWrite(LED_GREEN_PIN, LOW);
    digitalWrite(LED_YELLOW_PIN, LOW);
    digitalWrite(LED_RED_PIN, HIGH);
  } else {
    digitalWrite(LED_YELLOW_PIN, LOW);
    digitalWrite(LED_RED_PIN, LOW);
    digitalWrite(LED_GREEN_PIN, HIGH);
  }
}

bool buttonDebounce(button_t& button)
{
  bool changed = false;

  // read the state of the switch into a local variable:
  uint8_t reading = digitalRead(button.pin);

  // If the switch changed, due to noise or pressing:
  if (reading != button.lastState) {
    // reset the debouncing timer
    button.lastDebounceTime = millis();
  }

  if ((millis() - button.lastDebounceTime) > button.debounceDelay) {
    // if the button state has changed:
    if (reading != button.state) {
      button.state = reading;
      // print(Serial, F("BTN "), button.pin, F(": "), button.state);

      changed = true;
      // // only toggle the LED if the new button state is HIGH
      // if (b.buttonState == HIGH)
      //   // postDataToAPI(true);
      //   getSettingsFormAPI(true);
      // // readDataFromSensors();
    }
  }

  // save the reading. Next time through the loop, it'll be the lastButtonState:
  button.lastState = reading;
  return changed;
}

void resetModuleLeds(void)
{
  digitalWrite(LED_GPRS_PIN, LOW);
  digitalWrite(LED_NETWORK_PIN, LOW);
  digitalWrite(LED_MODULE_PIN, LOW);
}