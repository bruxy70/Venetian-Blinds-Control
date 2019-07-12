#ifndef Shutter_CLASS_H
#define Shutter_CLASS_H

#include "config.h"

#define vent_tolerance  2 // Tollerance in percent, when the shutter is detected to be in "vent" position
float upper_stop_offset = 1000;
#define lower_stop_offset  10000.0 // how far (ms) to go beyond bottom stop (make sure it is stopped, and sort of callibrate)
#define hold_button_delay 1000 // if button is held longer than this, the shutter will stop after the button is rekleased. Otherwise it will keep going till the end (or till the "vent" position
#define max_hold_time 60000 // if I hold button longer then this, release it

enum movement_type { up = -2, up_vent = -1, stopped = 0, down_vent = 1, down = 2};

class Button { 
  public:
    boolean pressed;
    boolean web_key_pressed;
    unsigned long pressed_timestamp;
    unsigned int counter;
    Button();
};

class Shutter {
  private:
    unsigned long duration_vents_down;
    int vent_position; // position sterbiny
    unsigned long duration_tilt;
    boolean tilting;
    int tilt_start;
    unsigned long duration_down,duration_up;
    int start_position; // position of the shutter when the movement started
    unsigned long timestamp_start;
    unsigned long auto_stop; // time when the shutter should stop (calculated when movement starts, from time, start position, direction and lenght of the movement
    unsigned long full_stop; // the time till the very end (in case it only goes to the "vent position", so the movement can be extended)
    byte pin_up;  // Pin for movement up
    byte pin_down; // Pin for movement down
    boolean calibrating;
    int Position;
    int Tilt; // 0 (otevreno), 90 (zavreno)
  public:
    Shutter();
    void setup(String j,unsigned long d_down,unsigned long d_up,unsigned long s_down,unsigned long t,byte p_up,byte p_down);
    int getPosition();
    void setPosition(int p);
    boolean semafor; // Making sure I do not call function when other function is running (and therefore combination of variables is not defined)    
 
    int getTilt();
    void setTilt(int tlt);
    void tilt_it(int tlt);
    
    Button btnUp;
    Button btnDown;
    String Name;
    boolean force_update;
    movement_type movement;
    void Start_up();
    void Start_down();
    void Calibrate();
    void Go_to_position(int p);
    void Stop();
    const char *Movement();
    void Process_key(boolean key_up,boolean key_down);
    void Update_position();
};

#endif
