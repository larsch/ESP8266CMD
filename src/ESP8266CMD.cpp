// SPDX-License-Identifier: MIT
// Copyright 2018 Lars Christensen

#include "ESP8266CMD.h"
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <user_interface.h>
#include <time.h>

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
  do { stream.print(field ": ");     \
    stream.println(val); } while (0)

static Stream* scanResponseStream = nullptr;

void printScanResponse(int networksFound)
{
  if (scanResponseStream == nullptr)
    return;
  Stream& stream = *scanResponseStream;
  stream.printf("%d network(s) found\r\n", networksFound);
  for (int i = 0; i < networksFound; i++)
  {
    stream.printf("%d: %s, Ch:%d (%ddBm) %s\r\n", i + 1, WiFi.SSID(i).c_str(), WiFi.channel(i), WiFi.RSSI(i), WiFi.encryptionType(i) == ENC_TYPE_NONE ? "open" : "");
  }
  scanResponseStream = nullptr;
}

struct Command {
  const char* cmd;
  handler_t handler;
  Command* next;
};

static Command* currentCommands = nullptr;

static void help(Stream& stream, int argc, const char* argv[])
{
  stream.print("Commands: ");
  Command* cmd = currentCommands;
  stream.print(cmd->cmd);
  cmd = cmd->next;
  while (cmd) {
    stream.print(',');
    stream.print(cmd->cmd);
    cmd = cmd->next;
  }
  stream.println("");
}

static void sysinfo(Stream& stream, int argc, const char* argv[])
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

static void stainfo(Stream& stream, int argc, const char* argv[])
{
  dumpInfo(stream, "Persistent", WiFi.getPersistent());
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

static void apinfo(Stream& stream, int argc, const char* argv[])
{
  dumpInfo(stream, "Station count", WiFi.softAPgetStationNum());
  dumpInfo(stream, "AP IP", WiFi.softAPIP());
  dumpInfo(stream, "AP MAC", WiFi.softAPmacAddress());


  softap_config config;
  if (wifi_softap_get_config(&config)) {
    if (config.ssid_len)
      stream.printf("SSID: %.*s\r\n", config.ssid_len, (char*)config.ssid);
    else
      dumpInfo(stream, "SSID", (char*)config.ssid);
    dumpInfo(stream, "Password", (char*)config.password);
    dumpInfo(stream, "Channel", config.channel);
    dumpInfo(stream, "Auth mode", config.authmode);
    dumpInfo(stream, "Hidden", config.ssid_hidden);
    dumpInfo(stream, "Max connections", config.max_connection);
    dumpInfo(stream, "Beacon interval", config.beacon_interval);
  }

  auto station = wifi_softap_get_station_info();
  while (station) {
    uint8_t* mac = station->bssid;
    stream.printf("Station MAC: %02x:%02x:%02x:%02x:%02x:%02x\n",
      mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    dumpInfo(stream, "Station IP", IPAddress(station->ip));
    station = STAILQ_NEXT(station, next);
  }
  wifi_softap_free_station_info();
}

static void apdisconnect(Stream& stream, int argc, const char* argv[])
{
  if (argc == 1)
    WiFi.softAPdisconnect();
  else if (argc == 2)
    WiFi.softAPdisconnect(atoi(argv[1]));
}

static void ap(Stream& stream, int argc, const char* argv[])
{
  if (argc < 2 || argc > 5)
    stream.println("Usage: ap <ssid> [password [channel [hidden]]]");
  else if (argc == 2)
    WiFi.softAP(argv[1]);
  else if (argc == 3)
    WiFi.softAP(argv[1], argv[2]);
  else if (argc == 4)
    WiFi.softAP(argv[1], argv[2], atoi(argv[3]));
  else if (argc == 5)
    WiFi.softAP(argv[1], argv[2], atoi(argv[3]), atoi(argv[4]));
}

static void restart(Stream& stream, int argc, const char* argv[])
{
  ESP.restart();
}

static void reset(Stream& stream, int argc, const char* argv[])
{
  ESP.reset();
}

static void connect(Stream& stream, int argc, const char* argv[])
{
  if (argc == 2) {
    WiFi.begin(argv[1]);
    stream.println("OK");
  } else if (argc == 3) {
    WiFi.begin(argv[1], argv[2]);
    stream.println("OK");
  } else {
    stream.println("Usage: connect <ssid> [password]");
  }
}

static void reconnect(Stream& stream, int argc, const char* argv[])
{
  WiFi.reconnect();
}

static void disconnect(Stream& stream, int argc, const char* argv[])
{
  if (argc == 1)
    WiFi.disconnect();
  else if (argc == 2)
    WiFi.disconnect(!!atoi(argv[1]));
  else
    Serial.println("Usage: disconnect [0|1]");
}

const char *modes[] = { "OFF", "STA", "AP", "STA+AP" };

int dolookup(const char** args, int len, const char* str) {
  for (int i = 0; i < len; ++i) {
    if (strcmp(args[i], str) == 0)
      return i;
  }
  return -1;
}

#define lookup(strs, str) dolookup((strs), sizeof(strs)/sizeof(strs[0]), str)

static void mode(Stream& stream, int argc, const char* argv[])
{
  if (argc == 1) {
    stream.printf("Mode: %s\r\n", modes[WiFi.getMode()]);
    return;
  } else if (argc == 2) {
    int mode = lookup(modes, argv[1]);
    if (mode >= 0) {
      WiFi.enableAP(!!(mode & WIFI_AP));
      WiFi.enableSTA(!!(mode & WIFI_STA));
      return;
    }
  }
  stream.println("Usage: mode [OFF|STA|AP|STA+AP]");
}

static void scan(Stream& stream, int argc, const char* argv[])
{
  if (scanResponseStream)
    return;
  scanResponseStream = &stream;
  WiFi.scanNetworksAsync(printScanResponse);
}

static void diag(Stream& stream, int argc, const char* argv[])
{
  WiFi.printDiag(stream);
}

static void debug(Stream& stream, int argc, const char* argv[])
{
  if (argc == 2)
    Serial.setDebugOutput((bool)String(argv[1]).toInt());
  else
    stream.println("Usage: debug 0|1");
}

static void hostname(Stream& stream, int argc, const char* argv[])
{
  if (argc == 2)
    WiFi.hostname(argv[1]);
  else {
    stream.print("Hostname: ");
    stream.println(WiFi.hostname());
  }
}

static void uptime(Stream& stream, int argc, const char* argv[])
{
  uint32_t seconds = millis() / 1000;
  uint32_t days = seconds / 86400; seconds = seconds % 86400;
  uint32_t hours = seconds / 3600; seconds = seconds % 3600;
  uint32_t minutes = seconds / 60; seconds = seconds % 60;
  stream.print("Uptime: ");
  if (days) {
    stream.print(days);
    stream.print('d');
  }
  if (days||hours) {
    stream.print(hours);
    stream.print('h');
  }
  if (days||hours||minutes) {
    stream.print(minutes);
    stream.print('m');
  }
  stream.print(seconds);
  stream.println('s');
}

static void date(Stream& stream, int argc, const char* argv[])
{
    time_t now;
    time(&now);
    tm tm;
    localtime_r(&now, &tm);
    char timeString[20];
    strftime(timeString, sizeof(timeString), "%Y-%m-%d %H:%M:%S", &tm);
    stream.println(timeString);
}

static void configtime(Stream& stream, int argc, const char* argv[])
{
  if (argc < 3 || argc > 5) {
    Serial.println("Usage: configtime <TIMEZONE> <server1> [<server2> [<server3>]]");
    return;
  }
  const char* tz = argv[1];
  const char* server1 = argv[2];
  const char* server2 = argc > 3 ? argv[3] : nullptr;
  const char* server3 = argc > 4 ? argv[4] : nullptr;
  configTime(tz, server1, server2, server3);
}

static void persist(Stream&, int argc, const char* argv[])
{
  if (argc == 2) {
    WiFi.persistent(!!atoi(argv[1]));
  } else {
    Serial.println(WiFi.getPersistent());
  }
}

static void autoconnect(Stream&, int argc, const char* argv[])
{
  if (argc == 2) {
    WiFi.setAutoConnect(!!atoi(argv[1]));
  } else {
    Serial.println(WiFi.getAutoConnect());
  }
}

static void autoreconnect(Stream&, int argc, const char* argv[])
{
  if (argc == 2) {
    WiFi.setAutoReconnect(!!atoi(argv[1]));
  } else {
    Serial.println(WiFi.getAutoReconnect());
  }
}

static Command cmds[] = {
  { "help", help, &cmds[1] },
  { "hostname", hostname, &cmds[2] },
  { "uptime", uptime, &cmds[3] },
  { "sysinfo", sysinfo, &cmds[4] },
  { "restart", restart, &cmds[5] },
  { "reset", reset, &cmds[6] },
  { "scan", scan, &cmds[7] },
  { "mode", mode, &cmds[8] },
  { "stainfo", stainfo, &cmds[9] },
  { "connect", connect, &cmds[10] },
  { "disconnect", disconnect, &cmds[11] },
  { "reconnect", reconnect, &cmds[12] },
  { "ap", ap, &cmds[13] },
  { "apinfo", apinfo, &cmds[14] },
  { "apdisconnect", apdisconnect, &cmds[15] },
  { "diag", diag, &cmds[16] },
  { "date", date, &cmds[17] },
  { "configtime", configtime, &cmds[18] },
  { "persist", persist, &cmds[19] },
  { "autoconnect", autoconnect, &cmds[20] },
  { "autoreconnect", autoreconnect, &cmds[21] },
  { "debug", debug, nullptr }
};

struct Command* ESP8266CMD::commands = cmds;

ESP8266CMD::ESP8266CMD()
  : buffer(new char[COMMAND_LENGTH]),
    length(0),
    password(nullptr),
    prompt(nullptr)
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

void ESP8266CMD::begin(Stream& stream, const char* password)
{
  this->stream = &stream;
  this->password = password;
}

void ESP8266CMD::setPrompt(const char* prompt)
{
  this->prompt = prompt;
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
  // process backspace
  char* iter = command;
  char* out = command;
  while (*iter) {
    if (*iter == 8) // backspace
    {
      if (out > command)
        --out;
      ++iter;
    } else {
      *out++ = *iter++;
    }
  }
  *out = 0;
  const char* argv[MAX_ARGV];
  int argi = 0;
  while (*command && argi < MAX_ARGV) {
    /* skip whitespace */
    while (*command == ' ')
      *command++ = 0;
    char end = ' ';
    if (*command == '"') {
      end = '"';
      *command++ = 0; /* erase quote */
    }
    if (*command)
      argv[argi++] = command;
    while (*command && *command != end)
      command++; /* skip word */
    if (end == '"')
      *command++ = 0; /* erase quote */
  }
  handleCommand(argi, argv);
  if (this->prompt)
    Serial.print(this->prompt);
}

void ESP8266CMD::handleCommand(int argc, const char *argv[])
{
  if (argc == 0) {
    stream->print("No command given");
    return;
  }

  if (this->password) {
    if (strcmp(argv[0], "auth") == 0 && argc == 2) {
      if (strcmp(argv[1], password) == 0) {
        stream->println("Login OK");
        this->password = nullptr;
      } else {
        stream->println("Incorrect password");
      }
    } else {
      stream->println("Login required (auth <password>)");
    }
    return;
  }

  Command* cmd = commands;
  while (cmd) {
    if (strcmp(cmd->cmd, argv[0]) == 0) {
      currentCommands = commands;
      cmd->handler(*stream, argc, argv);
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
