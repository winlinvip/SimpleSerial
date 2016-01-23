#include <SimpleDHT.h>
#include <SimpleSerial.h>

int pinLED = 13;
SimpleDHT11 dht11;

SimpleSerial ss;
int pinDHT11 = 2;

int pinHeater = 3;
byte targetTemperature = 0;
unsigned long heaterExpire = 0;
bool heaterTask = false;

void setup() {
  pinMode(pinLED, OUTPUT);
  pinMode(pinHeater, OUTPUT);
  ss.begin(115200);

  // default to light LED.
  digitalWrite(pinLED, HIGH);
}

void terminate_heater() {
  digitalWrite(pinLED, HIGH);
  digitalWrite(pinHeater, LOW);
  heaterTask = false;
}

void execute_heater() {
  if (!heaterTask) {
    return;
  }
  
  // when heater not expired, check the task.
  if (millis() < heaterExpire) {
    byte temperature = 0, humidity = 0;
    if (dht11.read(pinDHT11, &temperature, &humidity, NULL)) {
      return;
    }
    delay(1000);
    
    if (temperature >= targetTemperature) {
      terminate_heater();
      return;
    }

    // open heater.
    digitalWrite(pinHeater, HIGH);
    digitalWrite(pinLED, LOW);
    return;
  }

  // close heater.
  terminate_heater();
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
    ss.write2(SSC_RESP_TH, temperature, humidity);
    return;
  }

  // open the heater, set the target and expire.
  if (command = SSC_OPEN_HEATER) {
    heaterTask = true;
    targetTemperature = param[0];
    heaterExpire = millis() + (unsigned long)(param[1]) * 1000;
    ss.write0(SSC_HEATER_OPENED);
    return;
  }

  // unknown command, mirror the command.
  ss.write1(SSC_NOT_SUPPORT, command);
}
