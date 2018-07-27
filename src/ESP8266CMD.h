#ifndef _esp8266cmd_h
#define _esp8266cmd_h

#include <Arduino.h>

class ESP8266CMD
{
public:
  ESP8266CMD();
  void begin(Stream& stream = Serial);
  void run();
private:
  char* buffer;
  int length;
  Stream* stream;
};

#endif // _esp8266cmd_h
