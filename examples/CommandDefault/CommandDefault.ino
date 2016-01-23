#include <SimpleDHT.h>
#include <SimpleSerial.h>

SimpleSerial ss;
SimpleDHT11 dht11;
int pinDHT11 = 2;

void setup() {
  ss.begin(115200);
}

void loop() {
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

  // unknown command, mirror the command.
  ss.write1(SSC_NOT_SUPPORT, command);
}
