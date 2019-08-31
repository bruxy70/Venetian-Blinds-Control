/*
 Sonoff 4ch (ESP8285) MQTT sensor for shutters
 Generic 8285 module
 (or 8266 module,  FlashMode DOUT!!!!!)
 1M (no SPIFFS)
 Reset method: ck
 
 RX <-> RX !!!
 TX <-> TX !!!
Functionality:
 Push button for <1s - cover will go till the end. If I hold it longer, shutter will stop when I release the button.
 When I push the button during movement, it will stop
 
Roller shutters with horizontal vents:
 When I press button once, it will stop leaving the vents open. When I press it it again (or if the cover is past the open vents position), it will go till the end (both up and down)
 When I sent the cover to position 1, will will leave vents open
Tilt
 For blids, there is a separate function to control tilt. When I stop the movement, it will automatically tilt to the tilt tilt before it started moving
 For blinds with tilt, the behaviour pushing button for <1s and > 1s works differently than for roller shuuers (<1s pushes are meant for tilting)
The position is based on measuring the time for the shuuer to go fully up and down (and to the open vent position and for tilting blades)
Internally, the program works with position 0 - shutter up (open), 100 - shutter down (closed). But for mqtt, it maps the numbers 0=closed, 100=open (can be changed through commenting _reverse_position_mapping_)
It can control 1 or 2 shutters - controlled by _two_covers_ (works with Sonoff 4ch)
Sample configuration
cover:
  - platform: mqtt
    name: "MQTT Cover"
    command_topic: "blinds/cover1/set"
    state_topic: "blinds/cover1/state"
    set_position_topic:  "blinds/cover1/position"
    payload_open: "open"
    payload_close: "close"
    payload_stop: "stop"
    state_open: 0
    state_closed: 100
    tilt_command_topic: 'blinds/cover1/tilt'
    tilt_status_topic: 'blinds/cover1/tilt-state'
 
*/

//#define DEBUG 1
//#define DEBUG_updates 1

#include <ESP8266WiFi.h>  // ESP8266 WiFi module
//#include <ESP8266WiFiMulti.h>
#include <ESP8266mDNS.h>  // Sends host name in WiFi setup
#include <PubSubClient.h> // MQTT
#include <ArduinoOTA.h>   // (Over The Air) update

#include <ArduinoJson.h>
#include <math.h>
#include "shutter_class.h"
#include "config.h"
#include "crc.h"
#include "web.h"

unsigned long lastUpdate = 0; // timestamp - last MQTT update
unsigned long lastCallback = 0; // timestamp - last MQTT callback received
unsigned long lastWiFiDisconnect=0;
unsigned long lastWiFiConnect=0;
unsigned long lastMQTTDisconnect=0; // last time MQTT was disconnected
unsigned long WiFiLEDOn=0;
unsigned long k1_up_pushed=0;
unsigned long k1_down_pushed=0;
unsigned long k2_up_pushed=0;
unsigned long k2_down_pushed=0;

String lastCommand = "";
String crcStatus="";

configuration cfg,web_cfg;
Shutter r1;
Shutter r2;

// MQTT callback declaratiion (definition below)
void callback(char* topic, byte* payload, unsigned int length); 

WiFiClient espClient;         // WiFi
//ESP8266WiFiMulti wifiMulti;   // Primary and secondary WiFi

// Tady musim inicializovat mqtt_server po setup

PubSubClient mqqtClient(espClient);   // MQTT client
//PubSubClient mqqtClient(_mqtt_server_,1883,callback,espClient);   // MQTT client
#ifdef _WEB_
  ESP8266WebServer server(80);    // Web Server
  ESP8266HTTPUpdateServer httpUpdater;
#endif

/************************ 
* S E T U P   W I F I  
*************************/
void setup_wifi() {
  delay(10);
  // We start by connecting to a WiFi network
  #ifdef DEBUG
    Serial.println();
    Serial.print("Connecting to WiFi");
  #endif

  WiFi.hostname(cfg.host_name);
  WiFi.mode(WIFI_STA); // Pouze WiFi client!!
  WiFi.begin(cfg.wifi_ssid1, cfg.wifi_password1);
//  wifiMulti.addAP(cfg.wifi_ssid1, cfg.wifi_password1);
//  wifiMulti.addAP(cfg.wifi_ssid2, cfg.wifi_password2);
  int i=0;
//  while (wifiMulti.run() != WL_CONNECTED && i<20) {
  while (WiFi.status() != WL_CONNECTED && i<60) {
    i++;
    delay(500);
    #ifdef DEBUG
      Serial.print(".");
    #endif
  }
  if (WiFi.status() != WL_CONNECTED && (strcmp(cfg.wifi_ssid1,_ssid1_)!=0 || strcmp(cfg.wifi_password1,_password1_)!=0))  {
    #ifdef DEBUG
       Serial.println();
       Serial.print("Loading defaults and restarting...");
    #endif
    defaultConfig(&cfg);
    saveConfig();
    Restart();
    delay(10000);
  }
  MDNS.begin(cfg.host_name);

  #ifdef DEBUG
    Serial.println("");
    Serial.printf("Connected to %s\n", WiFi.SSID().c_str());
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
  #endif
}


/********************************************
* M A I N   A R D U I N O   S E T U P 
********************************************/
void setup() {
// HW initialization
  pinMode(GPIO_REL1, OUTPUT);
  pinMode(GPIO_REL2, OUTPUT);
  pinMode(GPIO_REL3, OUTPUT);
  pinMode(GPIO_REL4, OUTPUT);
  pinMode(SLED, OUTPUT);
  digitalWrite(SLED, HIGH);   // Turn the Status Led off

  digitalWrite(GPIO_REL1, LOW);  // Off
  digitalWrite(GPIO_REL2, LOW);  // Off
  digitalWrite(GPIO_REL3, LOW);  // Off
  digitalWrite(GPIO_REL4, LOW);  // Off

// Open EEPROM
  openMemory();
  loadConfig();
  copyConfig(&cfg,&web_cfg);
  loadStatus();

  pinMode(cfg.GPIO_KEY1, INPUT_PULLUP);
  pinMode(cfg.GPIO_KEY2, INPUT_PULLUP);
  pinMode(cfg.GPIO_KEY3, INPUT_PULLUP);
  pinMode(cfg.GPIO_KEY4, INPUT_PULLUP);
  
  r1.setup("Shutter1",cfg.Shutter1_duration_down,cfg.Shutter1_duration_up,cfg.Shutter1_duration_vents_down,cfg.Shutter1_duration_tilt,GPIO_REL1,GPIO_REL2);
  r2.setup("Shutter2",cfg.Shutter2_duration_down,cfg.Shutter2_duration_up,cfg.Shutter2_duration_vents_down,cfg.Shutter2_duration_tilt,GPIO_REL3,GPIO_REL4);

  #ifdef DEBUG
    Serial.begin(115200);
  #endif

  setup_wifi();
  
  mqqtClient.setServer(cfg.mqtt_server,1883);
  mqqtClient.setCallback(callback);
  
// Over The Air Update
  ArduinoOTA.setHostname(cfg.host_name);
  ArduinoOTA.setPassword(OTA_password);
  ArduinoOTA.onStart([]() {
  #ifdef DEBUG
    Serial.println("OTA Start");
  #endif
  });
  ArduinoOTA.onEnd([]() {
  #ifdef DEBUG
    Serial.println("\nOTA End");
  #endif
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    #ifdef DEBUG
      Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    #endif
  });
  ArduinoOTA.onError([](ota_error_t error) {
    #ifdef DEBUG
      Serial.printf("Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
      else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
      else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
      else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
      else if (error == OTA_END_ERROR) Serial.println("End Failed");
    #endif
  });
  ArduinoOTA.begin();  

#ifdef _WEB_
// Zapnout Web Server
  server.on("/", handleRootPath);    //Associate the handler function to the path
  server.on("/configure",handleConfigurePath);
  server.on("/readMain", readMain);
  server.on("/readConfig",readConfig);
  server.on("/pressButton",pressButton);
  server.on("/updateField",updateField);
  httpUpdater.setup(&server,"/upgrade","admin","J1kubJeN1sKluk");
  server.begin();                    //Start the server
#endif

  #ifdef DEBUG
    Serial.println("Server listening");
  #endif



  // Nastaveni hotovo  
  #ifdef DEBUG
    Serial.println("Ready");
  #endif
}

String macToStr(const uint8_t* mac)
{
  String result;
  for (int i = 0; i < 6; ++i) {
    result += String(mac[i], 16);
    if (i < 5)
      result += ':';
  }
  return result;
}

/********************************
* R E C O N N E C T   M Q T T 
********************************/
void reconnect() {
  #ifdef DEBUG
    Serial.print("Attempting MQTT connection...");
  #endif

  unsigned long now = millis();
  if (lastMQTTDisconnect!=0 && lastMQTTDisconnect<now && abs(now-lastMQTTDisconnect)<10000) return;
  lastMQTTDisconnect=now;
  // Attempt to connect
  
  uint8_t mac[6];
  WiFi.macAddress(mac);
  String clientName(cfg.host_name);
  clientName += "-";
  clientName += macToStr(mac);
  clientName += "-";
  clientName += String(micros() & 0xff, 16);
  
  if (mqqtClient.connect((char*)clientName.c_str(),cfg.mqtt_user,cfg.mqtt_password)) {
    #ifdef DEBUG
      Serial.println("connected");
    #endif

    // Once connected, publish an announcement...
    digitalWrite(SLED, LOW);   // Turn the Status Led on
    // lastUpdate=0;
    // checkSensors(); // send current sensors
    // publishSensor();

    // resubscribe
    mqqtClient.subscribe(cfg.subscribe_command1);  // listen to control for cover 1
    mqqtClient.subscribe(cfg.subscribe_position1);  // listen to cover 1 postion set
    mqqtClient.subscribe(cfg.subscribe_reboot);  // listen for reboot command
    mqqtClient.subscribe(cfg.subscribe_calibrate);  // listen for calibration command
    if (cfg.two_covers) {
      mqqtClient.subscribe(cfg.subscribe_command2);  // listen to control for cover 2
      mqqtClient.subscribe(cfg.subscribe_position2);  // listen to cover 2 position set
    }
    if (cfg.tilt) {
      mqqtClient.subscribe(cfg.subscribe_tilt1);  // listen for cover 1 tilt position set
      if (cfg.two_covers) {
        mqqtClient.subscribe(cfg.subscribe_tilt2);  // listen for cover 2 tilt position set
      }
    }
  } else {
    digitalWrite(SLED, HIGH);   // Turn the Status Led off
    #ifdef DEBUG
      Serial.print("failed, rc=");
      Serial.print(mqqtClient.state());
    #endif
  }
}

void Restart() {
    mqqtClient.publish(cfg.subscribe_command1 , "" , true);
    mqqtClient.publish(cfg.subscribe_position1 , "" , true);
    mqqtClient.publish(cfg.publish_position1 , "" , true);
    if (cfg.two_covers) {
      mqqtClient.publish(cfg.subscribe_command2 , "" , true);
      mqqtClient.publish(cfg.subscribe_position2 , "" , true);
      mqqtClient.publish(cfg.publish_position2 , "" , true);
    }
    if (cfg.tilt) {
      mqqtClient.publish(cfg.subscribe_tilt1 , "" , true);
      mqqtClient.publish(cfg.publish_tilt1 , "" , true);
      if (cfg.two_covers) {
        mqqtClient.publish(cfg.subscribe_tilt2 , "" , true);
        mqqtClient.publish(cfg.publish_tilt2 , "" , true);        
      }
    }       
    ESP.restart();  
}

// Callback for processing MQTT message
void callback(char* topic, byte* payload, unsigned int length) {

  char* payload_copy = (char*)malloc(length+1);
  memcpy(payload_copy,payload,length);
  payload_copy[length] = '\0';

  #ifdef DEBUG
    Serial.print("Message arrived [");
    Serial.print(topic);
    Serial.print("] ");
    Serial.println(payload_copy);
  #endif
  lastCommand="Topic:";
  lastCommand+=topic;
  lastCommand+=",  Payload:";
  lastCommand+=payload_copy;
  
  lastCallback= millis();

  if (strcmp(topic, cfg.subscribe_command1) == 0) {

    if (strcmp(payload_copy,payload_open) == 0) {
      r1.Start_up();
    } else if (strcmp(payload_copy,payload_close) == 0) {
      r1.Start_down();
    } else if (strcmp(payload_copy,payload_stop) == 0) {
      r1.Stop(); 
    }
  } else if (cfg.two_covers && strcmp(topic, cfg.subscribe_command2) == 0) {
    if (strcmp(payload_copy,payload_open) == 0) {
      r2.Start_up();
    } else if (strcmp(payload_copy,payload_close) == 0) {
      r2.Start_down();
    } else if (strcmp(payload_copy,payload_stop) == 0) {
      r2.Stop();
    }
  } else if (strcmp(topic, cfg.subscribe_position1) == 0) {
      #ifdef _reverse_position_mapping_
        int p = map(constrain(atoi(payload_copy),0,100),0,100,100,0);
      #else
        int p = constrain(atoi(payload_copy),0,100);
      #endif
      if (p!=r1.getPosition()) {
        r1.Go_to_position(p);   
      } else {
        r1.force_update=true;
      }    
  } else if (cfg.two_covers && strcmp(topic, cfg.subscribe_position2) == 0) {
    
      #ifdef _reverse_position_mapping_
        int position = map(constrain(atoi(payload_copy),0,100),0,100,100,0);
      #else
        int position = constrain(atoi(payload_copy),0,100);
      #endif
      if (position!=r2.getPosition()) {
        r2.Go_to_position(position);
      } else {
        r2.force_update=true;
      }      
  } else if (cfg.tilt && strcmp(topic, cfg.subscribe_tilt1) == 0) {
      int tilt = constrain(atoi(payload_copy),0,100);
      if (tilt!=r1.getTilt()) {      
        r1.tilt_it(tilt);
      } else {
        r1.force_update=true;
      }
  } else if (cfg.tilt && cfg.two_covers && strcmp(topic, cfg.subscribe_tilt2) == 0) {
      int tilt = constrain(atoi(payload_copy),0,100);
      if (tilt!=r2.getTilt()) {      
        r2.tilt_it(tilt);
      } else {
        r2.force_update=true;
      }
  } else if (strcmp(topic, cfg.subscribe_calibrate) == 0) {
    r1.Calibrate();
    r2.Calibrate();
  } else if (strcmp(topic, cfg.subscribe_reboot) == 0) {    
     Restart();
  }
  free(payload_copy);
}

/*****************************************************************
* S E N D   S E N S O R S   M Q Q T  ( J S O N ) 
******************************************************************/
void publishSensor() {
  char message1[10];
  char message2[10];
  char message3[10];
  char message4[10];
  unsigned long now = millis();

  unsigned long interval = ((r1.movement==stopped) && (r2.movement==stopped))?update_interval_passive:update_interval_active;
  
  if ( lastUpdate==0 || lastUpdate>now || abs(now-lastUpdate)>interval || r1.force_update || r2.force_update ) {  
    // INFO: the data must be converted into a string; a problem occurs when using floats...
    #ifdef _reverse_position_mapping_    
      snprintf(message1,10,"%d",map(r1.getPosition(),0,100,100,0));
    #else
      snprintf(message1,10,"%d",r1.getPosition());
    #endif
    mqqtClient.publish(cfg.publish_position1, message1 , false);
    //yield();

    if (cfg.two_covers) {
      #ifdef _reverse_position_mapping_    
        snprintf(message2,10,"%d",map(r2.getPosition(),0,100,100,0));
      #else
        snprintf(message2,10,"%d",r2.getPosition());
      #endif
      mqqtClient.publish(cfg.publish_position2, message2 , false);
    }
    //yield();
    if (cfg.tilt) {
      snprintf(message3,10, "%d", r1.getTilt());
      mqqtClient.publish(cfg.publish_tilt1, message3 , false);
      //yield();
      if (cfg.two_covers) {
        snprintf(message4,10, "%d", r2.getTilt());
        mqqtClient.publish(cfg.publish_tilt2, message4 , false);
        //yield();
      }
    }
    saveStatus();   
  #ifdef DEBUG_updates
    Serial.printf("position: %d/%d\n",r1.getPosition(),r2.getPosition());
  #endif
    
    r1.force_update=false;
    r2.force_update=false;
    lastUpdate=now;
  }
}




/***********************************************************************************************
* L O O P  -  K O N T R O L A   S E N S O R U    A    P O S L A N I   M Q T T   U P D A T E 
************************************************************************************************/
void checkSensors() {

  unsigned long now = millis();

  boolean k1_up= (digitalRead(cfg.GPIO_KEY1)==KEY_PRESSED);
  boolean k1_down= (digitalRead(cfg.GPIO_KEY2)==KEY_PRESSED);
  boolean k2_up= (digitalRead(cfg.GPIO_KEY3)==KEY_PRESSED);
  boolean k2_down= (digitalRead(cfg.GPIO_KEY4)==KEY_PRESSED);

  if (button_press_delay) { // Ignorovat stisknuti kratsi nez 100 ms
    if(k1_up) {
      if (k1_up_pushed==0 || (abs(now - k1_up_pushed) < _button_delay_ )) {
        k1_up=false;
        if (k1_up_pushed==0)
          k1_up_pushed=now;
      }
    } else {
      k1_up_pushed=0;
    }
    if(k1_down) {
      if (k1_down_pushed==0 || (abs(now - k1_down_pushed) < _button_delay_ )) {
        k1_down=false;
        if (k1_down_pushed==0)
          k1_down_pushed=now;
      }
    } else {
      k1_down_pushed=0;
    }
    if(k2_up) {
      if (k2_up_pushed==0 || (abs(now - k2_up_pushed) < _button_delay_ )) {
        k2_up=false;
        if (k2_up_pushed==0)
          k2_up_pushed=now;
      }
    } else {
      k2_up_pushed=0;
    }
    if(k2_down) {
      if (k2_down_pushed==0 || (abs(now - k2_down_pushed) < _button_delay_ )) {
        k2_down=false;
        if (k2_down_pushed==0)
          k2_down_pushed=now;
      }
    } else {
      k2_down_pushed=0;
    }
  }
  
  r1.Process_key(k1_up,k1_down);
  r2.Process_key(k2_up,k2_down);
}  

/****************************************************
* L O O P    -   K O N T R O L A   C A S O V A C U 
****************************************************/
void checkTimers() {
  r1.Update_position();
  r2.Update_position();
}

/********************************************************
K O N V E R T U J E   S I G N A L   Z    d B   N A   % 
*********************************************************/
int WifiGetRssiAsQuality(int rssi)
{
  int quality = 0;

  if (rssi <= -100) {
    quality = 0;
  } else if (rssi >= -50) {
    quality = 100;
  } else {
    quality = 2 * (rssi + 100);
  }
  return quality;
}

void timeDiff(char *buf,size_t len,unsigned long lastUpdate){
    //####d, ##:##:##0
    unsigned long t = millis();
    if(lastUpdate>t) {
      snprintf(buf,len,"N/A");
      return;
    }
    t=(t-lastUpdate)/1000;  // Converted to difference in seconds

    int d=t/(60*60*24);
    t=t%(60*60*24);
    int h=t/(60*60);
    t=t%(60*60);
    int m=t/60;
    t=t%60;
    if(d>0) {
      snprintf(buf,len,"%dd, %02d:%02d:%02d",d,h,m,t);
    } else if (h>0) {
      snprintf(buf,len,"%02d:%02d:%02d",h,m,t); 
    } else {
      snprintf(buf,len,"%02d:%02d",m,t); 
    }
}


/***************************************
* M A I N   A R D U I N O   L O O P  
***************************************/
void loop() {
  unsigned long now = millis();
  checkTimers();
  checkSensors();
  if (WiFi.status() == WL_CONNECTED) {
    lastWiFiConnect=now;  // Not used at the moment
    ArduinoOTA.handle(); // OTA first
    if (mqqtClient.loop()) {
       publishSensor();
    } else {
      reconnect();      
    }
    #ifdef _WEB_    
      server.handleClient();         // Web handling
    #endif
  } else {
    lastWiFiDisconnect=now;
    if (abs(WiFiLEDOn-lastWiFiDisconnect)>2000) {
      digitalWrite(SLED, LOW);   // Turn the Status Led on
      WiFiLEDOn=lastWiFiDisconnect;
    }
    if (abs(WiFiLEDOn-lastWiFiDisconnect)>1000) {
      digitalWrite(SLED, HIGH);   // Turn the Status Led off
    }
  } 
  delay(update_interval_loop); // 25 ms (short because of tiltu) (1.5 degrees in in 25 ms)
}
