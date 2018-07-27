// SPDX-License-Identifier: MIT
// Copyright 2018 Lars Christensen

#include "ESP8266CMD.h"
#include <Arduino.h>
#include <ESP8266WiFi.h>

#define MAX_ARGV 16
#define COMMAND_LENGTH 128

static const char *getWifiStatus(wl_status_t status) {
  switch (status) {
  case WL_IDLE_STATUS:
    return "Idle";
  case WL_NO_SSID_AVAIL:
    return "No SSID";
  case WL_SCAN_COMPLETED:
    return "Scan Done";
  case WL_CONNECTED:
    return "Connected";
  case WL_CONNECT_FAILED:
    return "Failed";
  case WL_CONNECTION_LOST:
    return "Lost";
  case WL_DISCONNECTED:
    return "Disconnected";
  default:
    return "Other";
  }
}

#define dumpInfo(stream, field, val) \
  stream->print(field ": ");         \
  stream->println(val);

static Stream* scanResponseStream = nullptr;

void printScanResponse(int networksFound)
{
  Stream* stream = scanResponseStream;
  stream->printf("%d network(s) found\n", networksFound);
  for (int i = 0; i < networksFound; i++)
  {
    stream->printf("%d: %s, Ch:%d (%ddBm) %s\n", i + 1, WiFi.SSID(i).c_str(), WiFi.channel(i), WiFi.RSSI(i), WiFi.encryptionType(i) == ENC_TYPE_NONE ? "open" : "");
  }
  scanResponseStream = nullptr;
}

struct Command {
  const char* cmd;
  handler_t handler;
  Command* next;
};

static Command* currentCommands = nullptr;

static void help(Stream* stream, int argc, const char* argv[])
{
  stream->print("Commands: ");
  Command* cmd = currentCommands;
  stream->print(cmd->cmd);
  cmd = cmd->next;
  while (cmd) {
    stream->print(',');
    stream->print(cmd->cmd);
    cmd = cmd->next;
  }
  stream->println("");
}

static void sysinfo(Stream* stream, int argc, const char* argv[])
{
  dumpInfo(stream, "Chip ID", ESP.getChipId());
  dumpInfo(stream, "Reset reason", ESP.getResetReason());
  dumpInfo(stream, "Free heap", ESP.getFreeHeap());
  dumpInfo(stream, "Core version", ESP.getCoreVersion());
  dumpInfo(stream, "SDK version", ESP.getSdkVersion());
  dumpInfo(stream, "CPU Freq", ESP.getCpuFreqMHz());
  dumpInfo(stream, "Sketch size", ESP.getSketchSize());
  dumpInfo(stream, "Sketch free space", ESP.getFreeSketchSpace());
  dumpInfo(stream, "Sketch MD5", ESP.getSketchMD5());
  dumpInfo(stream, "Flash chip ID", ESP.getFlashChipId());
  dumpInfo(stream, "Flash chip size", ESP.getFlashChipSize());
  dumpInfo(stream, "Flash chip real size", ESP.getFlashChipRealSize());
  dumpInfo(stream, "Flash chip speed", ESP.getFlashChipSpeed());
  dumpInfo(stream, "Cycle count", ESP.getCycleCount());
}

static void stainfo(Stream* stream, int argc, const char* argv[])
{
  dumpInfo(stream, "Is connected", WiFi.isConnected());
  dumpInfo(stream, "Auto connect", WiFi.getAutoConnect());
  dumpInfo(stream, "MAC address", WiFi.macAddress());
  dumpInfo(stream, "Status", WiFi.status());
  dumpInfo(stream, "Status text", getWifiStatus(WiFi.status()));
  if (WiFi.status() == WL_CONNECTED) {
    dumpInfo(stream, "Local IP", WiFi.localIP());
    dumpInfo(stream, "Subnet mask", WiFi.subnetMask());
    dumpInfo(stream, "Gateway IP", WiFi.gatewayIP());
    dumpInfo(stream, "DNS 1", WiFi.dnsIP(0));
    dumpInfo(stream, "DNS 2", WiFi.dnsIP(1));
    dumpInfo(stream, "BSSID", WiFi.BSSIDstr());
    dumpInfo(stream, "RSSI", WiFi.RSSI());
  }
  dumpInfo(stream, "Hostname", WiFi.hostname());
  dumpInfo(stream, "SSID", WiFi.SSID());
  dumpInfo(stream, "PSK", WiFi.psk());
}

static void apinfo(Stream* stream, int argc, const char* argv[])
{
  dumpInfo(stream, "Station count", WiFi.softAPgetStationNum());
  dumpInfo(stream, "AP IP", WiFi.softAPIP());
  dumpInfo(stream, "AP MAC", WiFi.softAPmacAddress());
}

static void restart(Stream* stream, int argc, const char* argv[])
{
  ESP.restart();
}

static void connect(Stream* stream, int argc, const char* argv[])
{
  if (argc == 2) {
    WiFi.begin(argv[1]);
    stream->println("OK");
  } else if (argc == 3) {
    WiFi.begin(argv[1], argv[2]);
    stream->println("OK");
  } else {
    stream->println("Usage: connect <ssid> [password]");
  }
}

static void reconnect(Stream* stream, int argc, const char* argv[])
{
  WiFi.reconnect();
}

static void disconnect(Stream* stream, int argc, const char* argv[])
{
  WiFi.disconnect();
}

static void scan(Stream* stream, int argc, const char* argv[])
{
  if (scanResponseStream)
    return;
  scanResponseStream = stream;
  WiFi.scanNetworksAsync(printScanResponse);
}

static void diag(Stream* stream, int argc, const char* argv[])
{
  WiFi.printDiag(*stream);
}

static void debug(Stream* stream, int argc, const char* argv[])
{
  if (argc == 2)
    Serial.setDebugOutput((bool)String(argv[1]).toInt());
  else
    stream->println("Usage: debug 0|1");
}

static void hostname(Stream* stream, int argc, const char* argv[])
{
  if (argc == 2)
    WiFi.hostname(argv[1]);
  else {
    stream->print("Hostname: ");
    stream->println(WiFi.hostname());
  }
}

static void uptime(Stream* stream, int argc, const char* argv[])
{
  uint32_t seconds = millis() / 1000;
  uint32_t days = seconds / 86400; seconds = seconds % 86400;
  uint32_t hours = seconds / 3600; seconds = seconds % 3600;
  uint32_t minutes = seconds / 60; seconds = seconds % 60;
  Serial.print("Uptime: ");
  if (days) {
    Serial.print(days);
    Serial.print('d');
  }
  if (days||hours) {
    Serial.print(hours);
    Serial.print('h');
  }
  if (days||hours||minutes) {
    Serial.print(minutes);
    Serial.print('m');
  }
  Serial.print(seconds);
  Serial.println('s');
}

Command cmds[] = {
  { "help", help, &cmds[1] },
  { "sysinfo", sysinfo, &cmds[2] },
  { "stainfo", stainfo, &cmds[3] },
  { "apinfo", apinfo, &cmds[4] },
  { "restart", restart, &cmds[5] },
  { "connect", connect, &cmds[6] },
  { "disconnect", disconnect, &cmds[7] },
  { "reconnect", reconnect, &cmds[8] },
  { "diag", diag, &cmds[9] },
  { "debug", debug, &cmds[10] },
  { "hostname", hostname, &cmds[11] },
  { "uptime", uptime, &cmds[12] },
  { "scan", scan, nullptr },
};

ESP8266CMD::ESP8266CMD()
  : buffer(new char[COMMAND_LENGTH]),
    length(0),
    commands(cmds)
{
}

ESP8266CMD::~ESP8266CMD()
{
  while (commands != cmds) {
    Command* next = commands->next;
    delete commands;
    commands = next;
  }
  delete[] buffer;
}

void ESP8266CMD::begin(Stream& stream)
{
  this->stream = &stream;
}

void ESP8266CMD::run()
{
  while (stream->available() > 0) {
    char byte = stream->read();
    if (byte == '\n' || byte == '\r') {
      buffer[length] = 0;
      if (length > 0)
        parseCommand(buffer);
      length = 0;
    } else if (length + 1 < COMMAND_LENGTH) {
      buffer[length++] = byte;
    }
  }
}

void ESP8266CMD::parseCommand(char* command)
{
  const char* argv[MAX_ARGV];
  int argi = 0;
  while (*command && argi < MAX_ARGV) {
    while (*command == ' ')
      *command++ = 0;
    if (*command)
      argv[argi++] = command;
    while (*command && *command != ' ')
      command++; /* skip word */
  }
  handleCommand(argi, argv);
}

void ESP8266CMD::handleCommand(int argc, const char *argv[])
{
  if (argc == 0) {
    stream->print("No command given");
    return;
  }

  Command* cmd = commands;
  while (cmd) {
    if (strcmp(cmd->cmd, argv[0]) == 0) {
      currentCommands = commands;
      cmd->handler(stream, argc, argv);
      stream->println("OK");
      return;
    }
    cmd = cmd->next;
  }

  stream->print("Unknown command: ");
  stream->println(argv[0]);
}

void ESP8266CMD::addCommand(const char* command, handler_t handler)
{
  Command* cmd = new Command{command, handler, commands};
  commands = cmd;
}
