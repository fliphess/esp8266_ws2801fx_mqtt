# ESP8266 RGB Ledstrip controller for ws2801 5050 5v led strips

**This repository is unmaintained since I moved all my esp ledstrips to esphome**


This little sketch is created by copy pasting from many other led controller projects for esp8266 nodemcu.
(Thanks to all who semi-voluntary contributed to this project by adding their code on github :))

It currently only supports controlling ws2801 led strips using a nodemcu.
It works out of the box with homeassistant using the `mqtt_json` light and supports OTA updates and has a beautiful web interface.


## Setup

### Connecting the ws2801 to the esp8266

| WS 2801 LED PINS | ESP8266 PINS  |
| ----             | ----          |
| 5V               | Vin           |
| GND              | GND           |
| CK               | GPIO2 (D4)    |
| SI               | GPIO3 (RX)    |


### Installing the nodemcu

- Install the arduino IDE, use the [esp8266 board manager](http://arduino.esp8266.com/stable/package_esp8266com_index.json)
- Open this project as a new arduino project
- Connect your nodemcu to usb
- Configure the nodemcu settings in Tools.
- Install the libs listed in `requirements.txt`
- Compile && Upload
- Profit


## Thanks to / Learned from / Used in / Wow!

- https://github.com/r41d/WS2801FX
- https://github.com/adafruit/Adafruit-WS2801-Library
- https://github.com/toblum/McLighting/
- https://github.com/corbanmailloux/esp-mqtt-rgb-led
- https://github.com/bruhautomation/ESP-MQTT-JSON-Digital-LEDs
- https://github.com/doctormord/Responsive_LED_Control

