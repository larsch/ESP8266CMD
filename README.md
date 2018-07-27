# ESP8266CMD #

**ESP8266CMD** is a library for the [ESP8266 Core for Arduino](https://github.com/esp8266/Arduino) to handle simple commands via Serial port (or other [Stream](https://www.arduino.cc/reference/en/language/functions/communication/stream/) to inspect and configure the ESP. It is very useful when learning the ESP8266 and for configuring the WiFi credentials without having to put it in the source code. The ESP stores the SSID and password persistently.

## Usage ##

### Basics ###

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
* `scan` - scan & print available access points

### Other interfaces ###

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

### Custom commands ###

You can easily add custom commands:

``` c++
void greet(Stream* stream, int argc, const char* argv[])
{
  if (argc == 2) {
    stream->print("Hello, ");
    stream->print(argv[1]);
    stream->println("!");
  } else {
    stream->println("Please tell me who to greet!");
  }
}

void setup()
{
  cmd.addCommand("greet", greet);
  cmd.begin();
}
```

## Author ##

  * Lars Christensen <larsch@belunktum.dk>

## License ##

This project is licensed under the MIT License - see the LICENSE.md file for details
