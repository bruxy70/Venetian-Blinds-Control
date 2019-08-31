#ifdef _WEB_
#include "web.h"
#include "shutter_class.h"
#include "config.h"

void handleRootPath() {            //Handler for the rooth path
   
  String html = MAIN_page;
  server.send(200, "text/html", html);
  
}
void handleConfigurePath() {            //Handler for the rooth path
   
  String html = CONFIGURE_page;
  server.send(200, "text/html", html);
  
}

void readMain() {
  String mqttResults[10] = { F("-4: server didn't respond within the keepalive time"), F("-3: network connection was broken"), F("-2: network connection failed"), F("-1: client is disconnected cleanly"), F("0: client is connected"), F("1: server doesn't support the requested version of MQTT"), F("2: server rejected the client identifier"), F("3: server was unable to accept the connection"), F("4: username/password were rejected"), F("5: client was not authorized to connect") };
  char buf1[25];
  char buf2[25];
  
  StaticJsonBuffer<700> jsonBuffer;   // normal 464
  JsonObject& root = jsonBuffer.createObject();

  JsonArray& keys = root.createNestedArray("keys");

  keys.add(r1.btnUp.pressed?"Pressed":String(r1.btnUp.counter));
  keys.add(r1.btnDown.pressed?"Pressed":String(r1.btnDown.counter));
  keys.add(r2.btnUp.pressed?"Pressed":String(r2.btnUp.counter));
  keys.add(r2.btnDown.pressed?"Pressed":String(r2.btnDown.counter));

  JsonArray& movement = root.createNestedArray("movement");
  movement.add(r1.Movement());
  movement.add(r2.Movement());

  JsonArray& position = root.createNestedArray("position");
  position.add(r1.getPosition());
  position.add(r2.getPosition());
  //position.add(map(r1.getPosition(),0,100,100,0));
  //position.add(map(r2.getPosition(),0,100,100,0));

  JsonArray& tilt= root.createNestedArray("tilt");
  tilt.add(r1.getTilt());
  tilt.add(r2.getTilt());
  
  root["device"]=String(cfg.host_name);
  root["two_covers"]=cfg.two_covers?"true":"false";
  root["tilting"]=cfg.tilt?"true":"false";
  root["wifi"]=String(WiFi.SSID());  
  if (r1.semafor && r2.semafor) {
    root["mqtt"]= mqttResults[mqqtClient.state()+4]+"(S1,S2)";
  } else if (r1.semafor) {
    root["mqtt"]= mqttResults[mqqtClient.state()+4]+"(S1)";
  } else if (r2.semafor) {
    root["mqtt"]= mqttResults[mqqtClient.state()+4]+"(S2)";
  } else {
    root["mqtt"]= mqttResults[mqqtClient.state()+4];
  }
  timeDiff(buf1,25,lastWiFiDisconnect);
  timeDiff(buf2,25,lastMQTTDisconnect);
  root["disconnect"]=((lastWiFiDisconnect==0)?"N/A":(String(buf1)+" ago"))+","+((lastMQTTDisconnect==0)?"N/A":(String(buf2)+" ago"));
  timeDiff(buf1,25,0);
  root["crc"]=crcStatus + " ("+String(buf1)+" ago)";
  root["mem"]="program: "+String(ESP.getFreeSketchSpace()/1024)+" kB | heap: "+String(ESP.getFreeHeap()/1024)+" kB";
  if(lastCommand=="")
    root["mqttmsg"]= "N/A";
  else
    timeDiff(buf1,25,lastCallback);
    root["mqttmsg"]= lastCommand + " ("+String(buf1)+" ago)";
  root["strength"]=String(WifiGetRssiAsQuality(WiFi.RSSI()));
  root["ip"]=WiFi.localIP().toString();
  
  if(lastUpdate==0) {
    root["update"]="N/A";
  } else {
    timeDiff(buf1,25,lastUpdate);
    root["update"]=String(buf1);
  }

  String out;
  root.prettyPrintTo(out);
#ifdef DEBUG_updates
  Serial.println(out);
#endif  
  
  jsonBuffer.clear(); 
  server.send(200, "text/plane", out); //Send values to client ajax request
}


void pressButton() {
  String t_state = server.arg("button"); //Refer  request.open("GET", "pressButton?button="+button, true);
  int btn=t_state.toInt();
  #ifdef DEBUG
    Serial.print("Web button: ");
    Serial.println(t_state);
  #endif

 //1,2,3,4 - pushed
 //11,12,13,14 - released
 switch(btn) {
  case 1:
    r1.btnUp.web_key_pressed=true; break;
  case 2:
    r1.btnDown.web_key_pressed=true; break;
  case 3:
    r2.btnUp.web_key_pressed=true; break;
  case 4:
    r2.btnDown.web_key_pressed=true; break;
  case 11:
    r1.btnUp.web_key_pressed=false; break;
  case 12:
    r1.btnDown.web_key_pressed=false; break;
  case 13:
    r2.btnUp.web_key_pressed=false; break;
  case 14:
    r2.btnDown.web_key_pressed=false; break;
  case 55:
    r1.Calibrate();
    r2.Calibrate();
    break;
  case 66:
    Restart();
    break;
  case 77:
    defaultConfig(&web_cfg);
    break;
  case 88:
    copyConfig(&web_cfg,&cfg);
    saveConfig();    
    Restart();
    break;
 }
 server.send(200, "text/plane", t_state); //Send web page
}

void updateField() {
 String t_field = server.arg("field");
 String t_value = server.arg("value");
 if (t_field.equals("host_name")) {
    strncpy(web_cfg.host_name,t_value.c_str(),24);
 } else if (t_field.equals("vents")) {
    web_cfg.vents = t_value.equals("true");
 } else if (t_field.equals("auto_hold_buttons")) {
    web_cfg.auto_hold_buttons = t_value.equals("true");  
 } else if (t_field.equals("tilt")) {
    web_cfg.tilt = t_value.equals("true");
 } else if (t_field.equals("two_covers")) {
    web_cfg.two_covers = t_value.equals("true");
 } else if (t_field.equals("wifi_multi")) {
    web_cfg.wifi_multi = t_value.equals("true");
 } else if (t_field.equals("wifi_ssid1")) {
    strncpy(web_cfg.wifi_ssid1,t_value.c_str(),24);
 } else if (t_field.equals("wifi_password1")) {
    strncpy(web_cfg.wifi_password1,t_value.c_str(),24);
 } else if (t_field.equals("wifi_ssid2")) {
    strncpy(web_cfg.wifi_ssid2,t_value.c_str(),24);
 } else if (t_field.equals("wifi_password2")) {
    strncpy(web_cfg.wifi_password2,t_value.c_str(),24);
 } else if (t_field.equals("mqtt_server")) {
    strncpy(web_cfg.mqtt_server,t_value.c_str(),24);
 } else if (t_field.equals("mqtt_user")) {
    strncpy(web_cfg.mqtt_user,t_value.c_str(),24);
 } else if (t_field.equals("mqtt_password")) {
    strncpy(web_cfg.mqtt_password,t_value.c_str(),24);
 } else if (t_field.equals("publish_position1")) {
    strncpy(web_cfg.publish_position1,t_value.c_str(),49);
 } else if (t_field.equals("publish_position2")) {
    strncpy(web_cfg.publish_position2,t_value.c_str(),49);
 } else if (t_field.equals("publish_tilt1")) {
    strncpy(web_cfg.publish_tilt1,t_value.c_str(),49);
 } else if (t_field.equals("publish_tilt2")) {
    strncpy(web_cfg.publish_tilt2,t_value.c_str(),49);
 } else if (t_field.equals("subscribe_command1")) {
    strncpy(web_cfg.subscribe_command1,t_value.c_str(),49);
 } else if (t_field.equals("subscribe_command2")) {
    strncpy(web_cfg.subscribe_command2,t_value.c_str(),49);
 } else if (t_field.equals("subscribe_position1")) {
    strncpy(web_cfg.subscribe_position1,t_value.c_str(),49);
 } else if (t_field.equals("subscribe_position2")) {
    strncpy(web_cfg.subscribe_position2,t_value.c_str(),49);
 } else if (t_field.equals("subscribe_tilt1")) {
    strncpy(web_cfg.subscribe_tilt1,t_value.c_str(),49);
 } else if (t_field.equals("subscribe_tilt2")) {
    strncpy(web_cfg.subscribe_tilt2,t_value.c_str(),49);
 } else if (t_field.equals("subscribe_calibrate")) {
    strncpy(web_cfg.subscribe_calibrate,t_value.c_str(),49);
 } else if (t_field.equals("subscribe_reboot")) {
    strncpy(web_cfg.subscribe_reboot,t_value.c_str(),49);
 } else if (t_field.equals("subscribe_reset")) {
    strncpy(web_cfg.subscribe_reset,t_value.c_str(),49);   
 } else if (t_field.equals("Shutter1_duration_down")) {
    web_cfg.Shutter1_duration_down=constrain(t_value.toInt(),0,120000);
 } else if (t_field.equals("Shutter2_duration_down")) {
    web_cfg.Shutter2_duration_down=constrain(t_value.toInt(),0,120000);
 } else if (t_field.equals("Shutter1_duration_up")) {
    web_cfg.Shutter1_duration_up=constrain(t_value.toInt(),0,120000);
 } else if (t_field.equals("Shutter2_duration_up")) {
    web_cfg.Shutter2_duration_up=constrain(t_value.toInt(),0,120000);
 } else if (t_field.equals("Shutter1_duration_vents_down")) {
    web_cfg.Shutter1_duration_vents_down=constrain(t_value.toInt(),0,120000);
 } else if (t_field.equals("Shutter2_duration_vents_down")) {
    web_cfg.Shutter2_duration_vents_down=constrain(t_value.toInt(),0,120000);
 } else if (t_field.equals("Shutter1_duration_tilt")) {
    web_cfg.Shutter1_duration_tilt=constrain(t_value.toInt(),0,120000);
 } else if (t_field.equals("Shutter2_duration_tilt")) {
    web_cfg.Shutter2_duration_tilt=constrain(t_value.toInt(),0,120000);
 } else if (t_field.equals("GPIO_KEY1")) {
    web_cfg.GPIO_KEY1=constrain(t_value.toInt(),0,16);
 } else if (t_field.equals("GPIO_KEY2")) {
    web_cfg.GPIO_KEY2=constrain(t_value.toInt(),0,16);
 } else if (t_field.equals("GPIO_KEY3")) {
    web_cfg.GPIO_KEY3=constrain(t_value.toInt(),0,16);
 } else if (t_field.equals("GPIO_KEY4")) {
    web_cfg.GPIO_KEY4=constrain(t_value.toInt(),0,16);
 }
 server.send(200, "text/plane", t_field); //Send web page
 server.send(200, "text/plane", t_value); //Send web page  
}

void readConfig() {
  
  StaticJsonBuffer<1500> jsonBuffer;   // normal 1279 (but can configure longer strings, so leave it)
  JsonObject& root = jsonBuffer.createObject();
  root["host_name"] = web_cfg.host_name;
  root["two_covers"] = web_cfg.two_covers?"true":"false";
  root["tilt"] = web_cfg.tilt?"true":"false";
  root["vents"] = web_cfg.vents?"true":"false";
  root["auto_hold_buttons"] = web_cfg.auto_hold_buttons?"true":"false";
  root["wifi_ssid1"] = web_cfg.wifi_ssid1;
  root["wifi_password1"] = web_cfg.wifi_password1;
  root["wifi_ssid2"] = web_cfg.wifi_ssid2;
  root["wifi_password2"] = web_cfg.wifi_password2;
  root["wifi_multi"] = web_cfg.wifi_multi?"true":"false";
  root["mqtt_server"] = web_cfg.mqtt_server;
  root["mqtt_user"] = web_cfg.mqtt_user;
  root["mqtt_password"] = web_cfg.mqtt_password;
  root["publish_position1"] = web_cfg.publish_position1;
  root["publish_position2"] = web_cfg.publish_position2;
  root["publish_tilt1"] = web_cfg.publish_tilt1;
  root["publish_tilt2"] = web_cfg.publish_tilt2;
  root["subscribe_command1"] = web_cfg.subscribe_command1;
  root["subscribe_command2"] = web_cfg.subscribe_command2;
  root["subscribe_position1"] = web_cfg.subscribe_position1;
  root["subscribe_position2"] = web_cfg.subscribe_position2;
  root["subscribe_tilt1"] = web_cfg.subscribe_tilt1;
  root["subscribe_tilt2"] = web_cfg.subscribe_tilt2;
  root["subscribe_calibrate"] = web_cfg.subscribe_calibrate;
  root["subscribe_reboot"] = web_cfg.subscribe_reboot;
  root["subscribe_reset"] = web_cfg.subscribe_reset;
  root["Shutter1_duration_down"] = web_cfg.Shutter1_duration_down;
  root["Shutter2_duration_down"] = web_cfg.Shutter2_duration_down;
  root["Shutter1_duration_up"] = web_cfg.Shutter1_duration_up;
  root["Shutter2_duration_up"] = web_cfg.Shutter2_duration_up;
  root["Shutter1_duration_vents_down"] = web_cfg.Shutter1_duration_vents_down;
  root["Shutter2_duration_vents_down"] = web_cfg.Shutter2_duration_vents_down;
  root["Shutter1_duration_tilt"] = web_cfg.Shutter1_duration_tilt;
  root["Shutter2_duration_tilt"] = web_cfg.Shutter2_duration_tilt;
  root["GPIO_KEY1"] = web_cfg.GPIO_KEY1;
  root["GPIO_KEY2"] = web_cfg.GPIO_KEY2;
  root["GPIO_KEY3"] = web_cfg.GPIO_KEY3;
  root["GPIO_KEY4"] = web_cfg.GPIO_KEY4;
  
  String out;
  root.prettyPrintTo(out); 
#ifdef DEBUG_updates
  Serial.println(out);
#endif  
  jsonBuffer.clear(); 
  server.send(200, "text/plane", out); //Send values to client ajax request
}
#endif
