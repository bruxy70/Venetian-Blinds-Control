# Venetian-Blinds-Control

This is for ESP microcontrollers to control time based venetian blinds - controlling the action (open/close/stop), position (0-100) and the tilt (0-100).

For the position, 0 is closed, 100 is open - consistent with standard blinds and other 'things' that open and close (like valves) in Home Assistant and in general.
Intuitively I'd do it the other way around (so that it would represent how much of the cover is out). I had number of discussions with HA developers, even tried to reverse it in the custom cover controller. But at the end, I agreed that despite all concerns this does make most sense, and trying to hack it causes more problems than benefits.
If you do not like this from UI perspective, use a custom card and show it differently on front-end. But leave it standard at the back.

There are two versions of the controller: current - an ESPHome custom controller. And the legacy - based on Arduino code, using MQTT.

## Current - ESPHome controller

This uses a custom cover controller. Similar to the time based cover, but with added tilt control. To load the custom controller, add the following code to your ESPHome device configuration:

```yaml
external_components:
  - source:
      type: git
      url: https://github.com/bruxy70/Venetian-Blinds-Control
      ref: master
    components: [venetian_blinds]
```

Then you can configure the blinds. For the parameters and use refer to the [Time Based Cover](https://esphome.io/components/cover/time_based.html), with one additional parameter for `tilt_duration`.

Configuration example:

```yaml
cover:
  - platform: venetian_blinds
    name: "${cover1_name}"
    id: cover1
    open_action:
      - switch.turn_on: relay1
    open_duration: 51770ms
    close_action:
      - switch.turn_on: relay2
    close_duration: 51200ms
    tilt_duration: 1650ms
    stop_action:
      - switch.turn_off: relay1
      - switch.turn_off: relay2
```

There will be also a video on my YouTube channel, explaining the standard Time Based Cover, and this one as well.

## Legacy - Arduino code (not maintained)

Control Shutters or Venetian blinds via buttons and MQTT. Use 2 relays (up and down) and 2 binary inputs to independently control position and tilt. The blinds tilt down when going down and tilt up going up. Using timers to calculate the position/tilt independently without the need for end stops. For shutters recognize "vent position", when shutters are closed but leave open slits. Designed to work with Sonoff 4ch or Sonoff 4ch Pro to control two units, or Sonoff Dual for a single unit. (Will work with any ESP 8266 board).

<img src="https://github.com/bruxy70/Venetian-Blinds-Control/blob/master/images/Blinds.gif">

I have been using this for a long time and it works reliably.

## Getting Started

## Built With

- [Arduino](https://www.arduino.cc/en/Main/Software)

### Prerequisites

- Download source from the Sonoff_rolety directory
- Add http://arduino.esp8266.com/stable/package_esp8266com_index.json to Preferences/Additional board manager
- Add ESP8266 board
- Add libraries

### Building

- Check instructions in sonoff.ino to configure project
- Set paramaters in config.h
- Build

### Using

- Developed for shuters controled by 2 switches - one moves shutter up, the second for down. They switch on an L-wire.
- The project involves installing an ESP 8266 board with two relays, and connecting each L-wire to one relay
- It supports either 1 shutter (2 relays) or 2 shutters (4 relays)
- To use with Sonoff 4ch (or Sonoff 4ch pro):
  - You need to have an N wire - connect it to one of the Sonoff N sockets
  - Connect the common wire to the Sonoff In L socket (and to the C sockets if using Pro version)
  - Connect the wire for shutter 1 up direction to Out 1 L socket, shutter 1 down to Out 2, shutter 2 up to Out 3 and shutter 2 down to Out 4
  - If you want to use the original wall switch, solder wires to the 4 buttons on Sonoff 4ch, and connect 1st switch between button 1 and GND and so on
  - The program will make sure that it will not connect relay 1&2 and relay 3&4 at the same time. If relay 1 is on and you want to activate relay 2, it will automatically disconnect relay 1 (connecting both wires for up and down simultaneously is often use to enter the shutter programming mode and coudl mess-up setting of the end positions).

## Author

- **Václav Chaloupka** - _Initial work_ - [bruxy70](https://github.com/bruxy70)
