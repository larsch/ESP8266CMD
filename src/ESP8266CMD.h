// SPDX-License-Identifier: MIT
// Copyright 2018 Lars Christensen

#ifndef _ESP8266CMD_h
#define _ESP8266CMD_h

#include <Arduino.h>

typedef void (*handler_t)(Stream* stream, int argc, const char *argv[]);

class ESP8266CMD
{
public:
  ESP8266CMD();
  ~ESP8266CMD();
  void addCommand(const char* command, handler_t handler);
  void begin(Stream& stream = Serial, const char* password=nullptr);
  void run();
  
private:
  char* buffer;
  int length;
  Stream* stream;
  struct Command* commands;
  const char* password;

  void parseCommand(char* command);
  void handleCommand(int argc, const char *argv[]);
};

#endif // _ESP8266CMD_h
