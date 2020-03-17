import appdaemon.plugins.hass.hassapi as hass
from enum import Enum
import voluptuous as vol
import voluptuous_helper as vol_help
from datetime import datetime, time, timedelta

"""
Automatically adjust the gallery shutter position and tilt_position 
based on the sun elevation, sun azimuth, temperature instide and outside
Arguments:
- enable_automation         - input boolean to disable the automation
- outside_temperature       - outside temperature sensor
- weather                   - weather entity (check state for clouds)
- winter_treshold           - outside temperature theshold to switch to the winter_mode
- winter_max_temperature    - in winter suny day, let sun in if internal temperature is below
- heating_mode              - if defined - winter_mode will be On if the heating_mode is not 'Off'
- covers:                   - (required) list of rooms, each can have number if covers
  - entity_id:              - (required) cover entity ID
    inside_temperature      - room temperature sensor
    start_at                - do not run before this time (reflects sun azimuth)
    stop_at                 - do not run after this time (reflects sun azimuth)
    min_azimuth             - open if azimuth is smaller than
    max_azimuth             - open if azimuth is greater than
    angle_intercept         - blade angle = angle_intercept + angle_slope*sun_elevation
    angle_slope             - dtto
    tilting                 - does the shutter have tilting blades

configuration example (in AppDaemon's apps.yaml):

adjust_covers:
    module: adjust-covers
    class: AdjustCovers
    enable_automation: input_boolean.automation_cover_galerie
    outside_temperature: sensor.teplota_venku
    heating_mode: input_select.topeni
    weather: weather.dark_sky
    covers:
    - entity_id: cover.galerie_vlevo
      inside_temperature: sensor.teplota_uvnitr
      stop_at: '15:00'
      start_at: '8:00'
      min_azimuth: 25
      max_azimuth: 210
      tilting: True
    - entity_id: cover.galerie_vpravo
      inside_temperature: sensor.teplota_uvnitr
      stop_at: '15:00'
      start_at: '8:00'
      min_azimuth: 25
      max_azimuth: 210
      tilting: True

To run this script, you need to install the AppDaemon Add-on
And inclide these packages in its configuration:
  "python_packages": [
    "datetime",
    "voluptuous"
  ]
"""

ATTR_COVERS = "covers"
ATTR_OUTSIDE_TEMPERATURE = "outside_temperature"
ATTR_INSIDE_TEMPERATURE = "inside_temperature"
ATTR_WEARHER = "weather"
ATTR_COVERS = "covers"
ATTR_ENTITY_ID = "entity_id"
ATTR_ENABLE_AUTOMATION = "enable_automation"
ATTR_START_AT = "start_at"
ATTR_STOP_AT = "stop_at"
ATTR_MIN_AZIMITH = "min_azimuth"
ATTR_MAX_AZIMUTH = "max_azimuth"
ATTR_WINTER_TRESHOLD = "winter_treshold"
ATTR_WINTER_MAX_TEMPERATURE = "winter_max_temperature"
ATTR_HEATING_MODE = "heating_mode"
ATTR_ANGLE_INTERCEPT = "angle_intercept"
ATTR_ANGLE_SLOPE = "angle_slope"
ATTR_TILTING = "tilting"
DEFAULT_WINTER_TRESHOLD = 5.0
DEFAULT_WINTER_MAX_TEMPERATURE = 26.0
DEFAULT_ANGLE_INTERCEPT = 60.0
DEFAULT_ANGLE_SLOPE = 1.3
DEFAULT_MIN_AZIMUTH = 25
DEFAULT_MAX_AZIMUTH = 210
DEFAULT_TILTING = True
LOG_LEVEL = "INFO"
ADJUST_THRESHOLD_POSITION = 5
ADJUST_THRESHOLD_TILT = 10


CLEAR_SKY = [
    ["clear-night", False],
    ["cloudy", False],
    ["fog", False],
    ["hail", False],
    ["lightning", False],
    ["lightning-rainy", False],
    ["partlycloudy", True],
    ["pouring", False],
    ["rainy", False],
    ["snowy", False],
    ["snowy-rainy", False],
    ["sunny", True],
    ["windy", True],
    ["windy-variant", True],
    ["exceptional", True],
]


class Mode(Enum):
    SHUT = 1
    VARIABLE_BLOCKING = 2
    FIXED_OPEN = 3
    OPEN = 4


class Cover:
    def __init__(self, hassio, config):
        self.__hassio = hassio
        self.__entity_id = config.get(ATTR_ENTITY_ID)
        self.__inside_temperature = config.get(ATTR_INSIDE_TEMPERATURE)
        self.__start_at = config.get(ATTR_START_AT)
        self.__stop_at = config.get(ATTR_STOP_AT)
        self.__angle_intercept = config.get(ATTR_ANGLE_INTERCEPT)
        self.__angle_slope = config.get(ATTR_ANGLE_SLOPE)
        self.__min_azimuth = config.get(ATTR_MIN_AZIMITH)
        self.__max_azimuth = config.get(ATTR_MAX_AZIMUTH)
        self.__tilting = config.get(ATTR_TILTING)
        self.__inside_temperatre = None

    def update_cover(
        self,
        outside_temperature,
        sun_elevation,
        sun_azimuth,
        is_suny,
        is_winter,
        winter_max_temperature,
    ):
        now = datetime.now().time()
        if self.__start_at is not None and now < self.__start_at:
            return
        if self.__stop_at is not None and now >= self.__stop_at:
            return
        self.__get_inside_temperature()
        mode = self.__get_mode(
            sun_elevation, sun_azimuth, is_winter, is_suny, winter_max_temperature
        )
        self.__calculate_positions(mode, sun_elevation)
        self.__hassio.log(
            f"Updating {self.__entity_id} cover:\n"
            f"temperature_inside=[{self.__inside_temperature}],"
            f"temperature_outside=[{outside_temperature}]\n"
            f"sun_elevation=[{sun_elevation}],"
            f"sun_azimuth=[{sun_azimuth}]\n"
            f"is_suny=[{is_suny}],"
            f"is_winter=[{is_winter}]\n",
            f"{mode}: new position/tilt: "
            f"[{self.__position}/{self.__tilt_position}]",
            level=LOG_LEVEL,
        )
        position_adjusted = False
        try:
            current_position = self.get_state(
                self.__entity_id, attribute="current_position"
            )
            if self.__tilting:
                current_tilt_position = self.get_state(
                    self.__entity_id, attribute="current_tilt_position"
                )
        except:
            self.__hassio.error(f"Cannot find current tilt or position")
            current_position = current_tilt_position = None
        if current_position is None or (
            self.__tilting and current_tilt_position is None
        ):
            return
        if self.__tilting:
            self.__hassio.log(
                f"Current position/tilt: "
                f"[{current_position}/{current_tilt_position}]",
                level=LOG_LEVEL,
            )
        else:
            self.__hassio.log(
                f"Current position: " f"[{current_position}]", level=LOG_LEVEL
            )
        if abs(self.__position - current_position) > ADJUST_THRESHOLD_POSITION:
            self.set_position()
            position_adjusted = True
        if (
            self.__tilting
            and abs(self.__tilt_position - current_tilt_position)
            > ADJUST_THRESHOLD_TILT
        ):
            tilt_position_adjusted = True
            self.__hassio.run_in(self.set_tilt_position, 90 if position_adjusted else 0)
        if position_adjusted and tilt_position_adjusted:
            adjustments_made = (
                f"position set to {self.__position} and tilted {self.__tilt_position}°"
            )
        elif position_adjusted:
            adjustments_made = f"position set to {self.__position}"
        elif tilt_position_adjusted:
            adjustments_made = f"tilted {self.__tilt_position}°"
        else:
            adjustments_made = ""
        if position_adjusted or tilt_position_adjusted:
            self.__hassio.call_service(
                "logbook/log",
                name=self.__entity_id,
                message=f"{adjustments_made}, ({mode})",
            )

    def __get_inside_temperature(self):
        self.__inside_temperature = float(
            self.__hassio.get_state(self.__inside_temperature)
        )

    def __calculate_positions(self, mode, sun_elevation):
        if mode == Mode.SHUT:
            self.__position = 0
            if self.__tilting:
                self.__tilt_position = 90
        elif mode == Mode.OPEN:
            self.__position = 100
            if self.__tilting:
                self.__tilt_position = 0
        elif mode == Mode.FIXED_OPEN:
            if self.__tilting:
                self.__position = 0
                self.__tilt_position = 0
            else:
                self.__position = 100
        elif mode == Mode.VARIABLE_BLOCKING:
            self.__position = 0
            if self.__tilting:
                if sun_elevation > 46:
                    self.__tilt_position = 0
                else:
                    self.__tilt_position = int(
                        self.__angle_intercept - self.__angle_slope * sun_elevation
                    )

    def __get_mode(
        self, sun_elevation, sun_azimuth, is_winter, is_suny, winter_max_temperature
    ):
        if sun_elevation < 0:
            return Mode.SHUT
        if (
            (sun_azimuth < self.__min_azimuth)
            or (sun_azimuth > self.__max_azimuth)
            or not is_suny
        ):
            return Mode.FIXED_OPEN
        if is_winter and self.__inside_temperature < winter_max_temperature:
            return Mode.OPEN
        if self.__tilting:
            return Mode.VARIABLE_BLOCKING
        else:
            return Mode.SHUT

    def set_position(self):
        self.__hassio.call_service(
            "cover/set_cover_position",
            entity_id=self.__entity_id,
            position=self.__position,
        )
        return

    def set_tilt_position(self):
        self.__hassio.call_service(
            "cover/set_cover_tilt_position",
            entity_id=self.__entity_id,
            tilt_position=self.__tilt_position,
        )
        return


class AdjustCovers(hass.Hass):
    def initialize(self):
        COVER_SCHEMA = vol.Schema(
            {
                vol.Required(ATTR_ENTITY_ID): vol_help.existing_entity_id(self),
                vol.Optional(ATTR_INSIDE_TEMPERATURE): vol_help.existing_entity_id(
                    self
                ),
                vol.Optional(ATTR_START_AT): vol_help.time,
                vol.Optional(ATTR_STOP_AT): vol_help.time,
                vol.Optional(ATTR_ANGLE_SLOPE, default=DEFAULT_ANGLE_SLOPE): float,
                vol.Optional(
                    ATTR_ANGLE_INTERCEPT, default=DEFAULT_ANGLE_INTERCEPT
                ): float,
                vol.Optional(ATTR_MIN_AZIMITH, default=DEFAULT_MIN_AZIMUTH): int,
                vol.Optional(ATTR_MAX_AZIMUTH, default=DEFAULT_MAX_AZIMUTH): int,
                vol.Optional(ATTR_TILTING, default=DEFAULT_TILTING): bool,
            }
        )
        APP_SCHEMA = vol.Schema(
            {
                vol.Required("module"): str,
                vol.Required("class"): str,
                vol.Optional(ATTR_ENABLE_AUTOMATION): vol_help.existing_entity_id(self),
                vol.Optional(ATTR_OUTSIDE_TEMPERATURE): vol_help.existing_entity_id(
                    self
                ),
                vol.Optional(ATTR_HEATING_MODE): vol_help.existing_entity_id(self),
                vol.Optional(ATTR_WEARHER): vol_help.existing_entity_id(self),
                vol.Optional(
                    ATTR_WINTER_TRESHOLD, default=DEFAULT_WINTER_TRESHOLD
                ): float,
                vol.Optional(
                    ATTR_WINTER_MAX_TEMPERATURE, default=DEFAULT_WINTER_MAX_TEMPERATURE
                ): float,
                vol.Required(ATTR_COVERS): vol.All(
                    vol_help.ensure_list, [COVER_SCHEMA]
                ),
            },
            extra=vol.ALLOW_EXTRA,
        )

        __version__ = "0.0.1"
        try:
            config = APP_SCHEMA(self.args)
        except vol.Invalid as err:
            self.error(f"Invalid format: {err}", level="ERROR")
            return
        self.__outside_temperature = config.get(ATTR_OUTSIDE_TEMPERATURE)
        self.__weather = config.get(ATTR_WEARHER)
        self.__enable_automations = config.get(ATTR_ENABLE_AUTOMATION)
        self.__winter_treshold = config.get(ATTR_WINTER_TRESHOLD)
        self.__winter_max_temperature = config.get(ATTR_WINTER_MAX_TEMPERATURE)
        self.__heating_mode = config.get(ATTR_HEATING_MODE)
        self.__covers_config = config.get(ATTR_COVERS)
        self.__covers = []
        for cover in self.__covers_config:
            self.__covers.append(Cover(self, cover))

        if not self.entity_exists("sun.sun"):
            self.error("sun.sun entity not found")
            return
        runtime = datetime.now()
        addseconds = (round((runtime.minute * 60 + runtime.second) / 900) + 1) * 900
        runtime = runtime.replace(minute=0, second=0, microsecond=0) + timedelta(
            seconds=addseconds
        )
        self.run_every(self.update_covers, runtime, 900)

    def update_covers(self, kwargs):
        """This is run every 15 minutes to trigger cover update"""
        if (
            self.__enable_automations is not None
            and self.get_state(self.__enable_automations).lower() == "off"
        ):
            return
        if self.sun_down():
            return
        sun_elevation = float(self.get_state("sun.sun", attribute="elevation"))
        sun_azimuth = float(self.get_state("sun.sun", attribute="azimuth"))
        outside_temperature = float(self.get_state(self.__outside_temperature))
        is_suny = self.is_suny()
        is_winter = self.is_winter(outside_temperature)
        for cover in self.__covers:
            cover.update_cover(
                outside_temperature,
                sun_elevation,
                sun_azimuth,
                is_suny,
                is_winter,
                self.__winter_max_temperature,
            )

    def is_winter(self, outside_temperature) -> bool:
        if self.__heating_mode is None:
            return bool(outside_temperature < self.__winter_treshold)
        else:
            return bool(self.get_state(self.__heating_mode).lower() != "off")

    def is_suny(self) -> bool:
        if self.__weather is None:
            return True
        else:
            weather_icon = self.get_state(self.__weather)
            return next(
                (is_suny for weather, is_suny in CLEAR_SKY if weather_icon == weather),
                True,
            )
