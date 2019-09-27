import appdaemon.plugins.hass.hassapi as hass
from enum import Enum
import voluptuous as vol
import voluptuous_helper as vol_help
from datetime import datetime, time, timedelta
"""
Automatically adjust the gallery shutter position and tilt_position 
based on the sun elevation, sun azimuth, temperature instide and outside
Arguments:
- covers                    - (required) list of cover entities
- outside_temperature       - outside temperature sensor
- inside_temperature        - inside temperature sensor
- weather                   - weather entity (check state for clouds)
- enable_automation         - input boolean to disable the automation
- start_at                  - do not run before this time (reflects sun azimuth)
- stop_at                   - do not run after this time (reflects sun azimuth)
- winter_treshold           - outside temperature theshold to switch to the winter_mode
- winter_max_temperature    - in winter suny day, let sun in if internal temperature is below
- angle_intercept           - blade angle = angle_intercept + angle_slope*sun_elevation
- angle_slope               - dtto

configuration example (in AppDaemon's apps.yaml):

auto_adjust_gallery_cover:
    module: auto-adjust-gallery-cover
    class: AutoAdjustGalleryCover
    covers:
        - cover.galerie_vlevo
        - cover.galerie_vpravo
    outside_temperature: sensor.teplota_venku
    inside_temperature: sensor.teplota_uvnitr
    weather: weather.dark_sky
    enable_automation: input_boolean.automation_cover_galerie
    start_at: '8:00'
    stop_at: '15:00'

To run this script, you need to install the AppDaemon Add-on
And inclide these packages in its configuration:
  "python_packages": [
    "datetime",
    "voluptuous"
  ]
"""

ATTR_COVERS = 'covers'
ATTR_OUTSIDE_TEMPERATURE = 'outside_temperature'
ATTR_INSIDE_TEMPERATURE = 'inside_temperature'
ATTR_WEARHER = 'weather'
ATTR_ENABLE_AUTOMATION = 'enable_automation'
ATTR_START_AT = 'start_at'
ATTR_STOP_AT = 'stop_at'
ATTR_WINTER_TRESHOLD = 'winter_treshold'
ATTR_WINTER_MAX_TEMPERATURE = 'winter_max_temperature'
ATTR_ANGLE_INTERCEPT = "angle_intercept"
ATTR_ANGLE_SLOPE = "angle_slope"
DEFAULT_WINTER_TRESHOLD = 5.0
DEFAULT_WINTER_MAX_TEMPERATURE = 26.0
DEFAULT_ANGLE_INTERCEPT = 60.0
DEFAULT_ANGLE_SLOPE = 1.3
LOG_LEVEL = 'DEBUG'
ADJUST_THRESHOLD_POSITION = 5
ADJUST_THRESHOLD_TILT = 10


CLEAR_SKY = [['clear-night',False],
            ['cloudy',False],
            ['fog',False],
            ['hail',False],
            ['lightning',False],
            ['lightning-rainy',False],
            ['partlycloudy',True],
            ['pouring',False],
            ['rainy',False],
            ['snowy',False],
            ['snowy-rainy',False],
            ['sunny',True],
            ['windy',True],
            ['windy-variant',True],
            ['exceptional',True]]


class Mode(Enum):
    SHUT = 1
    VARIABLE_BLOCKING = 2
    FIXED_OPEN = 3
    OPEN = 4


def isTime(text):
    try:
        datetime.strptime(text, '%H:%M')
        return True
    except ValueError:
        return False


class AutoAdjustGalleryCover(hass.Hass):
    def initialize(self):
        APP_SCHEMA = vol.Schema({
            vol.Required('module'): str,
            vol.Required('class'): str,
            vol.Required(ATTR_COVERS): vol.All(
                vol_help.ensure_list,
                [vol_help.existing_entity_id(self)]
            ),
            vol.Optional(ATTR_OUTSIDE_TEMPERATURE): vol_help.existing_entity_id(self),
            vol.Optional(ATTR_INSIDE_TEMPERATURE): vol_help.existing_entity_id(self),
            vol.Optional(ATTR_WEARHER): vol_help.existing_entity_id(self),
            vol.Optional(ATTR_ENABLE_AUTOMATION): vol_help.existing_entity_id(self),
            vol.Optional(ATTR_START_AT): vol_help.time,
            vol.Optional(ATTR_STOP_AT): vol_help.time,
            vol.Optional(
                ATTR_WINTER_TRESHOLD,
                default=DEFAULT_WINTER_TRESHOLD
                ): float,
            vol.Optional(
                ATTR_WINTER_MAX_TEMPERATURE,
                default=DEFAULT_WINTER_MAX_TEMPERATURE): float,
            vol.Optional(
                ATTR_ANGLE_SLOPE,
                default=DEFAULT_ANGLE_SLOPE): float,
            vol.Optional(
                ATTR_ANGLE_INTERCEPT,
                default=DEFAULT_ANGLE_INTERCEPT): float,
        }, extra=vol.ALLOW_EXTRA)

        __version__ = '0.0.1'
        try:
            config = APP_SCHEMA(self.args)
        except vol.Invalid as err:
            self.error(f"Invalid format: {err}", level='ERROR')
            return
        self.log(f'Initialized entity_id={config.get("entity_id")}', 'DEBUG')
        self._inside_temperature = config.get(ATTR_INSIDE_TEMPERATURE)
        self._outside_temperature = config.get(ATTR_OUTSIDE_TEMPERATURE)
        self._weather = config.get(ATTR_WEARHER)
        self._enable_automations = config.get(ATTR_ENABLE_AUTOMATION)
        self._start_at = config.get(ATTR_START_AT)
        self._stop_at = config.get(ATTR_STOP_AT)
        self._covers = config.get(ATTR_COVERS)
        self._winter_treshold = config.get(ATTR_WINTER_TRESHOLD)
        self._winter_max_temperature = config.get(ATTR_WINTER_MAX_TEMPERATURE)
        self._angle_intercept = config.get(ATTR_ANGLE_INTERCEPT)
        self._angle_slope = config.get(ATTR_ANGLE_SLOPE)
        if not self.entity_exists('sun.sun'):
            self.error('sun.sun entity not found')
            return
        runtime = datetime.now()
        addseconds = (round((runtime.minute*60 + runtime.second)/900) + 1) * 900
        runtime = runtime.replace(minute=0, second=0, microsecond=0) \
            + timedelta(seconds=addseconds)
        self.run_every(self.update_covers, runtime, 900)

    def update_covers(self, kwargs):
        """This is run every 15 minutes to trigger cover update"""
        if (self._enable_automations is not None and
                not self.get_state(self._enable_automations)):
            return
        if self.sun_down():
            return
        now = datetime.now().time()
        if self._start_at is not None and now < self._start_at:
            return
        if self._stop_at is not None and now >= self._stop_at:
            return

        sun_elevation = float(self.get_state('sun.sun', attribute='elevation'))
        sun_azimuth = float(self.get_state('sun.sun', attribute='azimuth'))
        outside_temperature = float(self.get_state(self._outside_temperature))
        inside_temperature = float(self.get_state(self._inside_temperature))
        if self._weather is None:
            is_suny = True
        else:
            weather_icon = self.get_state(self._weather)
            is_suny = next((is_suny for weather, is_suny in CLEAR_SKY if weather_icon == weather), True)
        is_winter = bool(outside_temperature < self._winter_treshold)
        self.log(
            f'temperature_inside=[{inside_temperature}],'
            f'temperature_outside=[{outside_temperature}],'
            f'sun_elevation=[{sun_elevation}],'
            f'sun_azimuth=[{sun_azimuth}],'
            f'is_suny=[{is_suny}],'
            f'is_winter=[{is_winter}]',
            LOG_LEVEL)
        mode = self.get_mode(
            sun_elevation,
            sun_azimuth,
            is_winter,
            is_suny,
            inside_temperature)
        self.calculate_positions(mode, sun_elevation)
        self.log(
            f'{mode}, new position/tilt: '
            f'[{self._position}/{self._tilt_position}]',
            LOG_LEVEL)
        all_position_adjusted = all_tilt_position_adjusted = False
        for cover in self._covers:
            position_adjusted = tilt_position_adjusted = False
            self.set_state(cover, attributes={'mode': f'{mode}'})
            try:
                current_position = self.get_state(
                    cover,
                    attribute='current_position')
                current_tilt_position = self.get_state(
                    cover,
                    attribute='current_tilt_position')
            except:
                self.error(f'Cannot find current_position of {cover}')
                current_position = current_tilt_position = None
            if (current_position is None or
                    current_tilt_position is None):
                continue
            self.log(
                f'Current {cover} position/tilt: '
                f'[{current_position}/{current_tilt_position}]',
                LOG_LEVEL)
            if abs(self._position - current_position) > ADJUST_THRESHOLD_POSITION:
                self.set_position(cover, self._position)
                position_adjusted = all_position_adjusted = True
            if abs(self._tilt_position - current_tilt_position) > ADJUST_THRESHOLD_TILT:
                self.run_in(
                    self.set_tilt_position,
                    90 if position_adjusted else 0,
                    cover=cover,
                    tilt_position=self._tilt_position)
                tilt_position_adjusted = all_tilt_position_adjusted = True
        if all_position_adjusted and all_tilt_position_adjusted:
            adjustments_made = f'position set to {self._position} and tilted {self._tilt_position}°'
        elif all_position_adjusted:
            adjustments_made = f'position set to {self._position}'
        elif all_tilt_position_adjusted:
            adjustments_made = f'tilted {self._tilt_position}°'
        else:
            adjustments_made = ''
        if (all_position_adjusted or all_tilt_position_adjusted):
            self.call_service(
                'logbook/log',
                name='Covers galerie',
                message=f'{adjustments_made}, ({mode})')

    def set_position(self, cover, position):
        self.call_service(
            'cover/set_cover_position',
            entity_id=cover,
            position=position)
        return

    def set_tilt_position(self, kwargs):
        cover = kwargs["cover"]
        tilt_position = kwargs["tilt_position"]
        self.call_service(
            'cover/set_cover_tilt_position',
            entity_id=cover,
            tilt_position=tilt_position)
        return

    def get_mode(
        self,
        sun_elevation,
        sun_azimuth,
        is_winter,
        is_suny,
        inside_temperature
    ):
        if sun_elevation < 0:
            return Mode.SHUT
        if (sun_azimuth < 25) or (sun_azimuth > 210) or not is_suny:
            return Mode.FIXED_OPEN
        if is_winter:
            if inside_temperature < self._winter_max_temperature:
                return Mode.OPEN
            else:
                return Mode.VARIABLE_BLOCKING
        return Mode.VARIABLE_BLOCKING

    def calculate_positions(self, mode, sun_elevation):
        if mode == Mode.SHUT:
            self._position = 0
            self._tilt_position = 90
        elif mode == Mode.OPEN:
            self._position = 100
            self._tilt_position = 0
        elif mode == Mode.FIXED_OPEN:
            self._position = 0
            self._tilt_position = 0
        elif mode == Mode.VARIABLE_BLOCKING:
            self._position = 0
            if (sun_elevation > 46):
                self._tilt_position = 0
            else:
                self._tilt_position = int(
                    self._angle_intercept - self._angle_slope*sun_elevation)
