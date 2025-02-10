## Night Clock

### Hardware
ESP8266 board (D1 Mini Pro)

### Short description

The intention was to have a very simple night clock which is understandable for a toddler. So, the basic idea is to have a red light when it's bed-time and a green light when it's okay to get up - as easy as that 😉

The very first concept approach was to use an ESP8266 and to periodically get the time from the network (NTP), but this requires to always have a WiFi connection and to periodically request time, which was quite power consuming.<br/>

### Configuration

You need to rename & complete `credentials.h` with your WiFi connection details.