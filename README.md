# Venetian-Blinds-Control

Control Shutters or Venetian blinds via buttons and MQTT. Use 2 relays (up and down) and 2 binary inputs to independently control position and tilt. The blinds tilt down when going down and tilt up going up. Using timers to calculate the position/tilt independently without the need for end stops. For shutters recognize "vent position", when shutters are closed but leave open slits. Designed to work with Sonoff 4ch or Sonoff 4ch Pro to control two units, or Sonocch Dual for a single unit. (Will work with any ESP 8266 board).

## Getting Started

## Built With

* [Arduino](https://www.arduino.cc/en/Main/Software)

### Prerequisites

* Add http://arduino.esp8266.com/stable/package_esp8266com_index.json to Preferences/Additional board manager
* Add ESP8266 board
* Add libraries

### Building

* Check instructions in sonoff.ino to configure project
* Set paramaters in config.h
* Build

### Using

* Developed for shuters controled by 2 switches - one moves shutter up, the second for down. They switch on an L-wire.
* The project involves installing an ESP 8266 board with two relays, and connecting each L-wire to one relay
* It supports either 1 shutter (2 relays) or 2 shutters (4 relays)
* To use with Sonoff 4ch (or Sonoff 4ch pro):
** sss

## Authors

* **Václav Chaloupka** - *Initial work* - [bruxy70](https://github.com/bruxy70)

See also the list of [contributors](https://github.com/bruxy70/Venetian-Blinds-Control/contributors) who participated in this project.

## Acknowledgments
