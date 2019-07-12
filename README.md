# Venetian-Blinds-Control

Control Shutters or Venetian blinds via buttons and MQTT. Use 2 relays (up and down) and 2 binary inputs to independently control position and tilt. The blinds tilt down when going down and tilt up going up. Using timers to calculate the position/tilt independently without the need for end stops. For shutters recognize "vent position", when shutters are closed but leave open slits. Designed to work with Sonoff 4ch or Sonoff 4ch Pro to control two units, or Sonocch Dual for a single unit. (Will work with any ESP 8266 board).

## Getting Started

* Add http://arduino.esp8266.com/stable/package_esp8266com_index.json to Preferences/Additional board manager
* Add ESP8266 board
* Add libraries
* Check instructions in sonoff.ino to configure project
* Set paramaters in config.h
* Build

### Prerequisites

What things you need to install the software and how to install them

```
Give examples
```

### Installing

A step by step series of examples that tell you how to get a development env running

Say what the step will be

```
Give the example
```

And repeat

```
until finished
```

End with an example of getting some data out of the system or using it for a little demo

## Running the tests

Explain how to run the automated tests for this system

### Break down into end to end tests

Explain what these tests test and why

```
Give an example
```

### And coding style tests

Explain what these tests test and why

```
Give an example
```

## Deployment

Add additional notes about how to deploy this on a live system

## Built With

* [Arduino](https://www.arduino.cc/en/Main/Software)

## Contributing

Please read [CONTRIBUTING.md](https://gist.github.com/bruxy70/b24679402957c63ec426) for details on our code of conduct, and the process for submitting pull requests to us.

## Versioning

We use [SemVer](http://semver.org/) for versioning. For the versions available, see the [tags on this repository](https://github.com/your/project/tags). 

## Authors

* **Václav Chaloupka** - *Initial work* - [bruxy70](https://github.com/bruxy70)

See also the list of [contributors](https://github.com/bruxy70/Venetian-Blinds-Control/contributors) who participated in this project.

## License

This project is licensed under the MIT License - see the [LICENSE.md](LICENSE.md) file for details

## Acknowledgments

* Hat tip to anyone whose code was used
* Inspiration
* etc
