#ifndef CONFIG_H
#define CONFIG_H

#define _WEB_ 1

// Constants (default values, stored to EEPROM during the first run, can be configured from the web interface)
char _host_name_[] = "Blinds_Sonoff4ch";  // Has to be unique for each device

//comment for covers with no tilting blades
#define _tilt_ 1

//comment for single cover, uncomment for two covers (sonoff 4ch)
#define _two_covers_ 1  

// filters out noise (100 ms)
boolean button_press_delay = true;   
#define _button_delay_ 100  

//MQTT parameters (has to be unique for each device)
char _publish_position1_[] = "blinds/cover1/state";
char _publish_position2_[] = "blinds/cover2/state";
char _publish_tilt1_[] = "blinds/cover1/tilt-state";
char _publish_tilt2_[] = "blinds/cover2/tilt-state";
char _subscribe_command1_[] = "blinds/cover1/set";
char _subscribe_command2_[] = "blinds/cover2/set";
char _subscribe_position1_[] = "blinds/cover1/position";
char _subscribe_position2_[] = "blinds/cover2/position";
char _subscribe_tilt1_[] = "blinds/cover1/tilt";
char _subscribe_tilt2_[] = "blinds/cover2/tilt";

char _subscribe_calibrate_[] = "blinds/cover/calibrate";
char _subscribe_reset_[] = "blinds/cover/reset";
char _subscribe_reboot_[] = "blinds/cover/reboot";

// Time for each rolling shutter to go down and up - you need to measure this and configure - default values - can be changed via web (if enabled)
#define _Shutter1_duration_down_ 51200
#define _Shutter2_duration_down_ 51200
#define _Shutter1_duration_up_ 51770
#define _Shutter2_duration_up_ 51770
#define _Shutter1_duration_tilt_ 1650
#define _Shutter2_duration_tilt_ 1650

// if enabled, 0 is rolled in (open), 100 is fully rolled out (closed) - by default, Hassio considers 0=closed, 100=open - I think it does not make sense for covers
#define _reverse_position_mapping_ 1

char payload_open[] = "open";
char payload_close[] = "close";
char payload_stop[] = "stop";


// Changge these for your WIFI, IP and MQTT
char _ssid_[] = "wifi_ssid";
char _password1_[] = "wifi_password";
char OTA_password[] = "OTApassword"; // Only accepts [a-z][A-Z][0-9]
char _mqtt_server_[] = "xxx.xxx.xxx.xxx";
char _mqtt_user_[] = "mqtt_user";
char _mqtt_password_[] = "mqtt_password";


char movementUp[] ="up";
char movementDown[] = "down";
char movementUpVent[] = "up (vent)";
char movementDownVent[] = "down (vent)";
char movementStopped[] = "stopped";

#define update_interval_loop 50
#define update_interval_active 1000
#define update_interval_passive 300000


// This is for Sonoff 4ch. Change if using a different device
// Relay GPIO ports
#define GPIO_REL1 12  // r1 up wire
#define GPIO_REL2 5   // r1 down wire
#define GPIO_REL3 4   // r2 up wire
#define GPIO_REL4 15  // r2 down wire

// Buttons GPIO ports
#define _GPIO_KEY1_ 0  // r1 up button
#define _GPIO_KEY2_ 9  // r1 down button
#define _GPIO_KEY3_ 10 // r2 up button
#define _GPIO_KEY4_ 14 // r2 down button

#define SLED 13  // Blue light (inverted)
#define RX_PIN 1 // for external sensors - not used
#define TX_PIN 3 // for external sensors - not used

#define KEY_PRESSED  LOW

struct configuration {
  boolean tilt;
  boolean two_covers;
  boolean vents;
  boolean reverse_position_mapping; // currently not used
  char host_name[25];
  char wifi_ssid1[25];
  char wifi_password1[25];
  char wifi_ssid2[25];
  char wifi_password2[25];
  boolean wifi_multi;
  char mqtt_server[25];
  char mqtt_user[25];
  char mqtt_password[25];
  char publish_position1[50];
  char publish_position2[50];
  char publish_tilt1[50];
  char publish_tilt2[50];
  char subscribe_command1[50];
  char subscribe_command2[50];
  char subscribe_position1[50];
  char subscribe_position2[50];
  char subscribe_tilt1[50];
  char subscribe_tilt2[50];  
  char subscribe_calibrate[50];
  char subscribe_reboot[50];
  char subscribe_reset[50]; // currently not used
  unsigned long Shutter1_duration_down;
  unsigned long Shutter2_duration_down;
  unsigned long Shutter1_duration_up;
  unsigned long Shutter2_duration_up;
  unsigned long Shutter1_duration_vents_down;
  unsigned long Shutter2_duration_vents_down;
  unsigned long Shutter1_duration_tilt;
  unsigned long Shutter2_duration_tilt;
  byte GPIO_KEY1;
  byte GPIO_KEY2;
  byte GPIO_KEY3;
  byte GPIO_KEY4;
};

#endif
