# Venetian-Blinds-Control

Control Shutters or Venetian blinds via buttons and MQTT. Use 2 relays (up and down) and 2 binary inputs to independently control position and tilt. The blinds tilt down when going down and tilt up going up. Using timers to calculate the position/tilt independently without the need for end stops. For shutters recognize "vent position", when shutters are closed but leave open slits. Designed to work with Sonoff 4ch or Sonoff 4ch Pro to control two units, or Sonocch Dual for a single unit. (Will work with any ESP 8266 board).

## Getting Started

### Prerequisites

* Add http://arduino.esp8266.com/stable/package_esp8266com_index.json to Preferences/Additional board manager
* Add ESP8266 board
* Add libraries

### Installing

* Check instructions in sonoff.ino to configure project
* Set paramaters in config.h
* Build

## Built With

* [Arduino](https://www.arduino.cc/en/Main/Software)

## Authors

* **Václav Chaloupka** - *Initial work* - [bruxy70](https://github.com/bruxy70)

See also the list of [contributors](https://github.com/bruxy70/Venetian-Blinds-Control/contributors) who participated in this project.

## Acknowledgments
