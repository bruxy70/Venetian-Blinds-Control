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

Then you can configure the blinds. For the parameters and use refer to the [Time Based Cover](https://esphome.io/components/cover/time_based.html), with two additional parameters:
- **tilt_duration** (Required, Time): The amount of time it takes, to tilt between 0% and 100%
- **actuator_activation_duration** (Optional, Time): The amount of time it takes for the actuator/motor to start moving. Defaults to `0`.
Example: The Somfy J4 WT DataSheet states `Continuous orders of at least 200 ms must be sent to the drive to ensure proper execution.` The amount of time it takes is nondeterministic. Set `actuator_activation_duration: 200ms` for the blinds to move definitely.

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

To control the blinds by the wall switch, configure a binary switch with action to open and close them.
```yaml
  - id: key1
    platform: gpio
    pin:
      number: ${key1_gpio}
      mode: INPUT_PULLUP
      inverted: True
    on_press:
      then:
        - cover.open: cover1
    on_release:
      then:
        - cover.stop: cover1
  - id: key2
    platform: gpio
    pin:
      number: ${key2_gpio}
      mode: INPUT_PULLUP
      inverted: True
    on_press:
      then:
        - cover.close: cover1
    on_release:
      then:
        - cover.stop: cover1
```

I use a more advanced version, that will atomatically stop the cover when I hold it for a short time (less than 1s). If I hold it longer, it will keep running until the cover is fully open or closed (or unil I shortly press one of teh buttons). So for tilting it works as above. But to open or close it fully, you can release the button after 1s and it will continue moving.

```yaml
  - id: key1
    platform: gpio
    pin:
      number: ${key1_gpio}
      mode: INPUT_PULLUP
      inverted: True
    on_press:
      then:
        - cover.open: cover1
        - wait_until:
            condition: 
                binary_sensor.is_off: key1
            timeout: 1s
        - if:
            condition:
                binary_sensor.is_off: key1
            then:
                - cover.stop: cover1
  - id: key2
    platform: gpio
    pin:
      number: ${key2_gpio}
      mode: INPUT_PULLUP
      inverted: True
    on_press:
      then:
        - cover.close: cover1
        - wait_until:
            condition: 
                binary_sensor.is_off: key2
            timeout: 1s
        - if:
            condition:
                binary_sensor.is_off: key2
            then:
                - cover.stop: cover1
```

For the regular roller shades I use the opposite logic - short press will fully open or close the cover. Holding the button longer will stop the cover after releasing it. This is done by adding a separate action for a long press. This would not make sense for venetian blades - as it would not allow controlling the tilt.

```yaml
    on_multi_click:
    - timing:
        - ON for at least 500ms
      then:
        - cover.open: cover1
        - wait_until:
            condition:
              binary_sensor.is_off: key1
        - cover.stop: cover1
```

There are also videos on my YouTube channel, explaining the standard Time Based Cover, and this one as well.
(https://youtu.be/tg3nOoBFJLU and https://youtu.be/dk5yDBRHhbQ)

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
