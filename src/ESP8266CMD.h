// SPDX-License-Identifier: MIT
// Copyright 2018 Lars Christensen

#ifndef _ESP8266CMD_h
#define _ESP8266CMD_h

#include <Arduino.h>

class ESP8266CMD
{
public:
  ESP8266CMD();
  ~ESP8266CMD();
  void begin(Stream& stream = Serial);
  void run();
  
private:
  char* buffer;
  int length;
  Stream* stream;
  struct Command* commands;

  void help(Stream* stream, int argc, const char* argv[]);
  void sysinfo(Stream* stream, int argc, const char* argv[]);
  void stainfo(Stream* stream, int argc, const char* argv[]);
  void apinfo(Stream* stream, int argc, const char* argv[]);
  void restart(Stream* stream, int argc, const char* argv[]);
  void connect(Stream* stream, int argc, const char* argv[]);
  void reconnect(Stream* stream, int argc, const char* argv[]);
  void disconnect(Stream* stream, int argc, const char* argv[]);
  void scan(Stream* stream, int argc, const char* argv[]);
};

#endif // _ESP8266CMD_h
