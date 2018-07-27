# ESP8266CMD #

Library for the [ESP8266 Core for Arduino](https://github.com/esp8266/Arduino) to handle simple commands via Serial port (or other [Stream](https://www.arduino.cc/reference/en/language/functions/communication/stream/) to inspect and configure the ESP.

## Usage ##

To enable the command handler on the default hardware serial point, simply create an instance of the `ESP8266CMD` class, call `begin()` and `run()` as often as you can:

``` c++
ESP8266CMD cmd;

void setup()
{
  cmd.begin();
}

void loop()
{
  cmd.run();
}
```

Via serial console, input `help` to see a list of commands:

```
Commands: help,sysinfo,stainfo,apinfo,restart,connect,disconnect,reconnect,diag,debug,hostname,uptime,scan
```

These commands are available:

* `help` - show list of commands
* `sysinfo` - show device info
* `stainfo` - show WiFi station info
* `apinfo` - show WiFi access point info
* `restart` - restart CPU
* `connect <ssid> [password]` - connect to WiFi
* `disconnect` - disconnect from WiFi
* `reconnect` - reconnect to WiFi
* `diag` - print WiFi diagnostics (`WiFi.printDiag()`)
* `debug 0|1` - disable/enable debug output
* `hostname [hostname]` - get/set hostname
* `uptime` - print uptime
* `scan` - scan & print avaiable access points

## Other interfaces ##

**ESP8266CMD** can work via any `Stream` interface, e.g. connection via WiFi:

``` c++
WiFiServer server(23);

void setup()
{
  server.begin();
}

void loop()
{
  WiFiClient client = server.available();
  if (client) {
    ESP8266CMD clientCmd;
    clientCmd.begin(client);
    while (client.connected())
      clientCmd.run();
  }
}
```

Connection using ```PuTTY``` or netcat:

```
$ nc <ip-address> 23
```

Be aware that there is no password protection.