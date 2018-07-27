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

template<typename TYP> void dumpInfo(Stream* stream, const char* field, const TYP &val) {
  stream->print(field);
  stream->print(": ");
  stream->println(val);
}

static void dumpSystemInfo(Stream* stream)
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

static void dumpWifiInfo(Stream* stream)
{
  dumpInfo(stream, "Is connected", WiFi.isConnected());
  dumpInfo(stream, "Auto connect", WiFi.getAutoConnect());
  uint8_t mac[6];
  WiFi.macAddress(mac);
  char macStr[18];
  sprintf(macStr, "%02x:%02x:%02x:%02x:%02x:%02x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  dumpInfo(stream, "MAC address", macStr);
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
  dumpInfo(stream, "SSDI", WiFi.SSID());
  dumpInfo(stream, "PSK", WiFi.psk());
}

static void handleCommand(Stream* stream, int argc, const char *argv[])
{
  if (argc == 0) {
    stream->print("No command given");
    return;
  }

  if (strcmp(argv[0], "help") == 0) {
    stream->println("Commands: help,sysinfo,wifiinfo,restart,connect,disconnect");
  } else if (strcmp(argv[0], "sysinfo") == 0) {
    dumpSystemInfo(stream);
  } else if (strcmp(argv[0], "wifiinfo") == 0) {
    dumpWifiInfo(stream);
  } else if (strcmp(argv[0], "restart") == 0) {
    stream->println("OK");
    ESP.restart();
  } else if (strcmp(argv[0], "connect") == 0) {
    if (argc == 2) {
      WiFi.begin(argv[1]);
      stream->println("OK");
    } else if (argc == 3) {
      WiFi.begin(argv[1], argv[2]);
      stream->println("OK");
    } else {
      stream->println("Usage: connect <ssid> [password]");
    }
  } else if (strcmp(argv[0], "disconnect") == 0) {
    WiFi.disconnect();
    stream->println("OK");
  } else if (strcmp(argv[0], "reconnect") == 0) {
    WiFi.reconnect();
    stream->println("OK");
  } else {
    stream->println("Unknown command");
    stream->println(argv[0]);
  }
}

static void parseCommand(Stream* stream, char* command)
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
  handleCommand(stream, argi, argv);
}

ESP8266CMD::ESP8266CMD()
  : buffer(new char[COMMAND_LENGTH]),
    length(0)
{
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
        parseCommand(stream, buffer);
      length = 0;
    } else if (length + 1 < COMMAND_LENGTH) {
      buffer[length++] = byte;
    }
  }
}
