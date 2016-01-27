#include <SimpleDHT.h>
#include <SimpleSerial.h>

// the serial communicate with RaspberryPi.
SimpleSerial ss;

// the DHT11 temperatue and humidity sensor.
int pinDHT11 = 2;
SimpleDHT11 dht11;

// the flash LED.
int pinLED = 13;
bool ledEnabled = false;

// the params for heater.
int pinHeater = 3;
bool heaterEnabled = false;
unsigned long heaterExpire = 0;
byte reqTargetTemperature = 0;
byte reqHeaterCommandExpire = 0;

// the params for fan.
int pinFan = 4;
bool fanEnabled = false;
unsigned long fanExpire = 0;
byte reqTargetHumidity = 0;
byte reqFanCommandExpire = 0; 

void setup() {
  ss.begin(115200);
  
  pinMode(pinHeater, OUTPUT);
  digitalWrite(pinHeater, LOW);

  pinMode(pinFan, OUTPUT);
  digitalWrite(pinFan, LOW);

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

  // terminate fan when enable heater.
  terminate_fan();
  
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
  return;
}

void start_fan() {
  // check whether need to start fan.
  byte temperature = 0, humidity = 0;
  if (dht11.read(pinDHT11, &temperature, &humidity, NULL)) {
    return;
  }
  delay(200);
  
  if (humidity < reqTargetHumidity) {
    ss.write4(SSC_FAN_CLOSED, reqTargetHumidity, reqFanCommandExpire, humidity, 0);
    return;
  }

  // terminate heater when start fan.
  terminate_heater();
  
  // start fan.
  fanEnabled = true;
  digitalWrite(pinFan, HIGH);
  ss.write0(SSC_FAN_OPENED);
}

void terminate_fan() {
  digitalWrite(pinLED, LOW);
  digitalWrite(pinFan, LOW);
  fanExpire = 0;
  fanEnabled = false;
}

void execute_fan() {
  if (!fanEnabled) {
    return;
  }

  // always light LED for fan.
  digitalWrite(pinLED, HIGH);

  // detect the temperature and humidity.
  byte temperature = 0, humidity = 0;
  if (dht11.read(pinDHT11, &temperature, &humidity, NULL)) {
    return;
  }
  delay(200);

  // humidity ok, terminate fan.
  if (humidity < reqTargetHumidity) {
    terminate_fan();
    ss.write4(SSC_FAN_CLOSED, reqTargetHumidity, reqFanCommandExpire, humidity, 0);
    return;
  }
  
  // expired, terminate fan.
  if (millis() >= fanExpire) {
    terminate_fan();
    ss.write4(SSC_FAN_CLOSED, reqTargetHumidity, reqFanCommandExpire, humidity, 1);
    return;
  }

  // keep fan.
  digitalWrite(pinFan, HIGH);
  return;
}

void loop() {
  // execute previous tasks.
  execute_heater();
  execute_fan();
  
  // got new command.
  if (ss.read()) {
    delay(300);
    return;
  }

  // response the ping message.
  if (ss.command() == SSC_PING) {
    ss.write0(SSC_PING);
    return;
  }

  // response the DHT11 temperature and humidity.
  if (ss.command() == SSC_QUERY_TH) {
    byte temperature = 0, humidity = 0;
    if (dht11.read(pinDHT11, &temperature, &humidity, NULL)) {
      return;
    }
    delay(200);
    
    ss.write2(SSC_RESP_TH, temperature, humidity);
    return;
  }

  // open the heater, set the target and expire.
  if (ss.command() == SSC_OPEN_HEATER) {
    reqTargetTemperature = ss.arg0();
    reqHeaterCommandExpire = ss.arg1();
    heaterExpire = millis() + (unsigned long)reqHeaterCommandExpire * 1000;
    start_heater();
    return;
  }

  // open the fan, set the target and expire.
  if (ss.command() == SSC_OPEN_FAN) {
    reqTargetHumidity = ss.arg0();
    reqFanCommandExpire = ss.arg1();
    fanExpire = millis() + (unsigned long)reqFanCommandExpire * 1000;
    start_fan();
    return;
  }

  // unknown command, mirror the command.
  ss.write1(SSC_NOT_SUPPORT, ss.command());
}
