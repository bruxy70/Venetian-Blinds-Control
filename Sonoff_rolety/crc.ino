#include "config.h"
#include "crc.h"

struct shutter_position {
    int r1_position;
    int r2_position;
    int r1_tilt;
    int r2_tilt;
} p;

void openMemory() {
    EEPROM.begin(sizeof(configuration)+sizeof(unsigned long)+sizeof(shutter_position)+sizeof(unsigned long));
}

void saveConfig() {
  EEPROM.put(0,cfg);
  EEPROM.commit();
  unsigned long check=eeprom_crc(0,sizeof(configuration));
  EEPROM.put(sizeof(configuration),check);
  EEPROM.commit();  
}

void loadConfig() {
  unsigned long check1;
  unsigned long check2;
  
  EEPROM.get(0,cfg);
  EEPROM.get(sizeof(configuration),check2);
  check1=eeprom_crc(0,sizeof(configuration));
  if (check1!=check2) {  
    defaultConfig(&cfg);
    saveConfig();
    crcStatus+="CRC config failed. ";
  } else {
    crcStatus+="CRC config OK! ";
  }
}

void defaultConfig(configuration* c) {
  #if defined(_vents_)
    c->vents=true;
    c->tilt=false;
  #elif defined(_tilt_)
    c->vents=false;
    c->tilt=true;
  #else
    c->vents=false;
    c->tilt=false;    
  #endif
  #ifdef _two_covers_
    c->two_covers=true;
  #else
    c->two_covers=false;
  #endif
  #ifdef _auto_hold_buttons_
    c->auto_hold_buttons=true;
  #else
    c->auto_hold_buttons=false;
  #endif
  
  strncpy(c->host_name,_host_name_,24);
  strncpy(c->wifi_ssid1,_ssid1_,24);
  strncpy(c->wifi_password1,_password1_,24);
  strncpy(c->wifi_ssid2,_ssid1_,24);
  strncpy(c->wifi_password2,_password1_,24);
  c->wifi_multi=false;
  strncpy(c->mqtt_server,_mqtt_server_,24);
  strncpy(c->mqtt_user,_mqtt_user_,24);
  strncpy(c->mqtt_password,_mqtt_password_,24);
  strncpy(c->publish_position1,_publish_position1_,49);
  strncpy(c->publish_position2,_publish_position2_,49);
  strncpy(c->subscribe_command1,_subscribe_command1_,49);
  strncpy(c->subscribe_command2,_subscribe_command2_,49);
  strncpy(c->subscribe_position1,_subscribe_position1_,49);
  strncpy(c->subscribe_position2,_subscribe_position2_,49);
  strncpy(c->subscribe_calibrate,_subscribe_calibrate_,49);
  strncpy(c->subscribe_reset,_subscribe_reset_,49);
  strncpy(c->subscribe_reboot,_subscribe_reboot_,49);
  c->Shutter1_duration_down=_Shutter1_duration_down_;
  c->Shutter2_duration_down=_Shutter2_duration_down_;
  c->Shutter1_duration_up=_Shutter1_duration_up_;
  c->Shutter2_duration_up=_Shutter2_duration_up_;
  #if defined (_vents_)
    c->Shutter1_duration_vents_down=_Shutter1_duration_vents_down_;
    c->Shutter2_duration_vents_down=_Shutter2_duration_vents_down_;
  #else
    c->Shutter1_duration_vents_down=1;
    c->Shutter2_duration_vents_down=1;
  #endif
  #if defined(_tilt_)
    c->Shutter1_duration_tilt=_Shutter1_duration_tilt_;
    c->Shutter2_duration_tilt=_Shutter2_duration_tilt_;   
    strncpy(c->publish_tilt1,_publish_tilt1_,49);
    strncpy(c->publish_tilt2,_publish_tilt2_,49);
    strncpy(c->subscribe_tilt1,_subscribe_tilt1_,49);
    strncpy(c->subscribe_tilt2,_subscribe_tilt2_,49);
  #else
    c->Shutter1_duration_tilt=1;
    c->Shutter2_duration_tilt=1;
    strncpy(c->publish_tilt1,"",49);
    strncpy(c->publish_tilt2,"",49);
    strncpy(c->subscribe_tilt1,"",49);
    strncpy(c->subscribe_tilt2,"",49);
  #endif
  c->GPIO_KEY1=_GPIO_KEY1_;
  c->GPIO_KEY2=_GPIO_KEY2_;
  c->GPIO_KEY3=_GPIO_KEY3_;
  c->GPIO_KEY4=_GPIO_KEY4_;
}

void copyConfig(configuration* from,configuration* to) {
  to->vents=from->vents;
  to->auto_hold_buttons=from->auto_hold_buttons;
  to->tilt=from->tilt;
  to->two_covers=from->two_covers;
  strncpy(to->host_name,from->host_name,24);
  strncpy(to->wifi_ssid1,from->wifi_ssid1,24);
  strncpy(to->wifi_password1,from->wifi_password1,24);
  strncpy(to->wifi_ssid2,from->wifi_ssid2,24);
  strncpy(to->wifi_password2,from->wifi_password2,24);
  to->wifi_multi=from->wifi_multi;
  strncpy(to->mqtt_server,from->mqtt_server,24);
  strncpy(to->mqtt_user,from->mqtt_user,24);
  strncpy(to->mqtt_password,from->mqtt_password,24);
  strncpy(to->publish_position1,from->publish_position1,49);
  strncpy(to->publish_position2,from->publish_position2,49);
  strncpy(to->subscribe_command1,from->subscribe_command1,49);
  strncpy(to->subscribe_command2,from->subscribe_command2,49);
  strncpy(to->subscribe_position1,from->subscribe_position1,49);
  strncpy(to->subscribe_position2,from->subscribe_position2,49);
  strncpy(to->subscribe_calibrate,from->subscribe_calibrate,49);
  strncpy(to->subscribe_reset,from->subscribe_reset,49);
  strncpy(to->subscribe_reboot,from->subscribe_reboot,49);
  to->Shutter1_duration_down=from->Shutter1_duration_down;
  to->Shutter2_duration_down=from->Shutter2_duration_down;
  to->Shutter1_duration_up=from->Shutter1_duration_up;
  to->Shutter2_duration_up=from->Shutter2_duration_up;
  to->Shutter1_duration_vents_down=from->Shutter1_duration_vents_down;
  to->Shutter2_duration_vents_down=from->Shutter2_duration_vents_down;
  to->Shutter1_duration_tilt=from->Shutter1_duration_tilt;
  to->Shutter2_duration_tilt=from->Shutter2_duration_tilt;   
  strncpy(to->publish_tilt1,from->publish_tilt1,49);
  strncpy(to->publish_tilt2,from->publish_tilt2,49);
  strncpy(to->subscribe_tilt1,from->subscribe_tilt1,49);
  strncpy(to->subscribe_tilt2,from->subscribe_tilt2,49);
  to->GPIO_KEY1=from->GPIO_KEY1;
  to->GPIO_KEY2=from->GPIO_KEY2;
  to->GPIO_KEY3=from->GPIO_KEY3;
  to->GPIO_KEY4=from->GPIO_KEY4;  
}

void saveStatus() {
  if (p.r1_position==r1.getPosition() and p.r2_position==r2.getPosition() and p.r1_tilt==r1.getTilt() and p.r2_tilt==r2.getTilt())
    return;
  p.r1_position=r1.getPosition();
  p.r2_position=r2.getPosition();
  p.r1_tilt=r1.getTilt();
  p.r2_tilt=r2.getTilt();
  EEPROM.put(sizeof(configuration)+sizeof(unsigned long),p);
  EEPROM.commit();
  unsigned long check=eeprom_crc(sizeof(configuration)+sizeof(unsigned long),sizeof(shutter_position));
  EEPROM.put(sizeof(configuration)+sizeof(unsigned long)+sizeof(shutter_position),check);
  EEPROM.commit();
}

void loadStatus() {
  unsigned long check1;
  unsigned long check2;
  
  EEPROM.get(sizeof(configuration)+sizeof(unsigned long),p);
  EEPROM.get(sizeof(configuration)+sizeof(unsigned long)+sizeof(shutter_position),check2);

  check1=eeprom_crc(sizeof(configuration)+sizeof(unsigned long),sizeof(shutter_position));
  if (check1==check2) {
    #ifdef DEBUG
      Serial.println("EEPROM CRC check OK, reading stored values.");
    #endif
    r1.setPosition(p.r1_position);
    r2.setPosition(p.r2_position);
    if (cfg.tilt) {
      r1.setTilt(p.r1_tilt);
      r2.setTilt(p.r2_tilt);
      crcStatus += "CRC status OK: [("+String(p.r1_position)+","+String(p.r1_tilt)+"),("+String(p.r2_position)+","+String(p.r2_tilt)+")] loaded. ";
    } else {
      crcStatus += "CRC status OK: ["+String(p.r1_position)+","+String(p.r2_position)+"] loaded. ";
    }
    #ifdef DEBUG
      Serial.printf("shutter position %d/%d\n",p.r1_position,p.r2_position);
    #endif
  } else {
    #ifdef DEBUG
      Serial.println("EEPROM CRC check failed, using defaults.");
    #endif
    r1.setPosition(0);
    r2.setPosition(0);
    if (cfg.tilt) {
      r1.setTilt(0);
      r2.setTilt(0);
    }
    crcStatus += "CRC status failed! ";
//    r1.Calibrate();   Ideally, I woudl need to calibrate here. But what if it restarts at night? Disable for now!
//    r2.Calibrate();
  }
}

/**** Vypocita CRC ulozenych hodnot ****/ 

unsigned long eeprom_crc(int o,int s) {
  const unsigned long crc_table[16] = {
    0x00000000, 0x1db71064, 0x3b6e20c8, 0x26d930ac,
    0x76dc4190, 0x6b6b51f4, 0x4db26158, 0x5005713c,
    0xedb88320, 0xf00f9344, 0xd6d6a3e8, 0xcb61b38c,
    0x9b64c2b0, 0x86d3d2d4, 0xa00ae278, 0xbdbdf21c
  };

  unsigned long crc = ~0L;
  byte value;

  for (int index = 0 ; index < s  ; ++index) {
    value = EEPROM.read(o+index);
    crc = crc_table[(crc ^ value) & 0x0f] ^ (crc >> 4);
    crc = crc_table[(crc ^ (value >> 4)) & 0x0f] ^ (crc >> 4);
    crc = ~crc;
  }
  return crc;
}
