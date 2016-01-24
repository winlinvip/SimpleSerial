#include <SimpleDHT.h>
#include <SimpleSerial.h>

SimpleDHT11 dht11;

// the serial communicate with RaspberryPi.
SimpleSerial ss;
// the DHT11 temperatue and humidity sensor.
int pinDHT11 = 2;

// the heater and LED.
int pinHeater = 3;
int pinLED = 13;
bool ledEnabled = false;

// the params for heater.
bool heaterEnabled = false;
unsigned long heaterExpire = 0;
byte reqTargetTemperature = 0;
byte reqHeaterCommandExpire = 0;

void setup() {
  ss.begin(115200);
  
  pinMode(pinHeater, OUTPUT);
  digitalWrite(pinHeater, LOW);

  pinMode(pinLED, OUTPUT);
  digitalWrite(pinLED, LOW);
}

void start_heater() {
  // check whether need to start heater.
  byte temperature = 0, humidity = 0;
  if (dht11.read(pinDHT11, &temperature, &humidity, NULL)) {
    return;
  }
  delay(200);
  
  if (temperature >= reqTargetTemperature) {
    ss.write4(SSC_HEATER_CLOSED, reqTargetTemperature, reqHeaterCommandExpire, temperature, 0);
    return;
  }
  
  // start headter.
  heaterEnabled = true;
  digitalWrite(pinHeater, HIGH);
  ss.write0(SSC_HEATER_OPENED);
}

void terminate_heater() {
  digitalWrite(pinLED, LOW);
  digitalWrite(pinHeater, LOW);
  heaterExpire = 0;
  heaterEnabled = false;
}

void execute_heater() {
  if (!heaterEnabled) {
    return;
  }

  // flash the LED when heating.
  if (ledEnabled) {
    digitalWrite(pinLED, LOW);
  } else {
    digitalWrite(pinLED, HIGH);
  }
  ledEnabled = !ledEnabled;

  // detect the temperature and humidity.
  byte temperature = 0, humidity = 0;
  if (dht11.read(pinDHT11, &temperature, &humidity, NULL)) {
    return;
  }
  delay(200);

  // warm enough, terminate heater.
  if (temperature >= reqTargetTemperature) {
    terminate_heater();
    ss.write4(SSC_HEATER_CLOSED, reqTargetTemperature, reqHeaterCommandExpire, temperature, 0);
    return;
  }
  
  // expired, terminate heater.
  if (millis() >= heaterExpire) {
    terminate_heater();
    ss.write4(SSC_HEATER_CLOSED, reqTargetTemperature, reqHeaterCommandExpire, temperature, 1);
    return;
  }

  // keep heating.
  digitalWrite(pinHeater, HIGH);
  digitalWrite(pinLED, HIGH);
  return;
}

void loop() {
  // execute previous tasks.
  execute_heater();
  
  // got new command.
  byte command;
  byte param[12];
  if (ss.read(&command, param)) {
    delay(300);
    return;
  }

  // response the ping message.
  if (command == SSC_PING) {
    ss.write0(SSC_PING);
    return;
  }

  // response the DHT11 temperature and humidity.
  if (command == SSC_QUERY_TH) {
    byte temperature = 0, humidity = 0;
    if (dht11.read(pinDHT11, &temperature, &humidity, NULL)) {
      return;
    }
    delay(200);
    
    ss.write2(SSC_RESP_TH, temperature, humidity);
    return;
  }

  // open the heater, set the target and expire.
  if (command == SSC_OPEN_HEATER) {
    reqTargetTemperature = param[0];
    reqHeaterCommandExpire = param[1];
    heaterExpire = millis() + (unsigned long)reqHeaterCommandExpire * 1000;
    start_heater();
    return;
  }

  // unknown command, mirror the command.
  ss.write1(SSC_NOT_SUPPORT, command);
}
