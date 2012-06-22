/*****************************************
*   Simple-Solenoid-Sequencer-Shield ver 0.1
*   SMD Pre-Built version
*   share and share alike
*   cornwallhackerspace
******************************************/

#include <Adafruit_GFX.h>
#include <Adafruit_PCD8544.h>
#include <EEPROM.h>

// pin a - Serial clock out (SCLK)
// pin b - Serial data out (DIN)
// pin c - Data/Command select (D/C)
// pin d - LCD chip select (CS)
// pin e - LCD reset (RST)
// Adafruit_PCD8544(a,b,c,d,e);

// LCD instance
Adafruit_PCD8544 display = Adafruit_PCD8544(15, 16, 17, 19, 18);

// button pins
#define b1 7  // up
#define b2 5  // right
#define b3 4  // down
#define b4 14 // left
#define b5 3  // function
#define b6 2  // select

// led pin
#define led 6

// solenoid pins
#define s1 8
#define s2 9
#define s3 10
#define s4 11
#define s5 12
#define s6 13

#define SolOnTime 50

#define BPMmax 700
#define BPMmin 40

#define Rows 6
#define Cols 16

boolean b1_state_new = 0;
boolean b1_state_old = 0;
boolean b2_state_new = 0;
boolean b2_state_old = 0;
boolean b3_state_new = 0;
boolean b3_state_old = 0;
boolean b4_state_new = 0;
boolean b4_state_old = 0;
boolean b5_state_new = 0;
boolean b5_state_old = 0;
boolean b6_state_new = 0;
boolean b6_state_old = 0;

int16_t solenoid[Rows] = {s1, s2, s3, s4, s5, s6};

boolean  drum_array[Rows][Cols];
boolean slot1_array[Rows][Cols];
boolean slot2_array[Rows][Cols];

byte slot_1_col[Cols];
byte slot_2_col[Cols];

int16_t cursor_x = 0;
int16_t cursor_y = 0;
int16_t cursor_old_x = 0;
int16_t cursor_old_y = 0;

float bpm = 130;
float bpm_microsecs = 0;
int16_t bpm_display = 0;
int16_t count = 0;

float time_now = 0;
float time_then = 0;

float solTimer[Rows];
boolean solOn[Rows] = {0, 0, 0, 0, 0, 0};

boolean should_increment = 0;
boolean should_update = 0;
boolean leave_settings = 0;
boolean unselect_settings = 0;

int16_t brightness = 30;
int16_t contrast = 50;

/**
 *	Set up some vars and runs some init's.
 */
void setup() {
  // buttons    
  pinMode(b1, INPUT);
  pinMode(b2, INPUT);      
  pinMode(b3, INPUT);
  pinMode(b4, INPUT);
  pinMode(b5, INPUT);
  pinMode(b6, INPUT);
  // fets
  pinMode(s1, OUTPUT);
  pinMode(s2, OUTPUT);      
  pinMode(s3, OUTPUT);
  pinMode(s4, OUTPUT);
  pinMode(s5, OUTPUT);
  pinMode(s6, OUTPUT);
  // other
  pinMode(led, OUTPUT);
  // startup
  // Serial.begin(9600);
  // Serial.println("start");
  display.begin();
  display.setContrast(contrast);
  analogWrite(led, brightness);
  delay(5000);
  display.clearDisplay();
  reset_drum_array();
  enter_settings();
}

/**
 *  Load the saved drum patterns from eeprom to arrays.
 */
void load_slot_from_rom(int16_t slot){
  int16_t pos = 0;
  if (slot == 2) pos = 100;
  for (int16_t e = 0; e < Rows; e++) {
    for (int16_t i = 0; i < Cols; i++) {
      drum_array[e][i] = EEPROM.read(pos);
      pos++;
    }
  }
}

/**
 *  Save the current pattern to eeprom
 */
void save_slot_to_rom(int16_t slot){
  int16_t pos = 0;
  if (slot == 2) pos = 100;
  for (int16_t e = 0; e < Rows; e++) {
    for (int16_t i = 0; i < Cols; i++) {
      if (drum_array[e][i] == 1) {
        EEPROM.write(pos, 1);
      } else {
        EEPROM.write(pos, 0);
      }
      pos++;
    }
  }
}

/**
 *	Settings mode for loading patches and adjusting brightness.
 */
void enter_settings() {
  turn_all_off();
  delay(500);
  cursor_x = 0;
  cursor_y = 0;
  should_update = 1;
  unselect_settings = 1;
  leave_settings = 0;
  while(!leave_settings){
    get_move_buttons(0,1,0,4);
    get_settings_buttons();
    move_highligher();
    draw_settings();
    update_screen();
  }
}

/**
 *	Poll function buttons presses for adjusting the highlighted settings.
 */
void get_settings_buttons() {
  // increment brightness and contrast.
  if (digitalRead(b6)) {
    if (cursor_y == 1) {
      if (contrast < 60) {
        contrast++;
        display.setContrast(contrast);
        unselect_settings = 1;
        delay(200);
      }
    }
    if (cursor_y == 2) {
      brightness+=5;
      analogWrite(led, brightness);
      unselect_settings = 1;
      delay(200);
    }
  }
  // decrement brightness and contrast.
  if (digitalRead(b5)) {
    if (cursor_y == 1) {
      if (contrast > 0) {
        contrast--;
        display.setContrast(contrast);
        unselect_settings = 1;
        delay(200);
      }
    }
    if (cursor_y == 2) {
      brightness-=5;
      analogWrite(led, brightness);
      unselect_settings = 1;
      delay(200);
    }
  }
  // exit and load or save using either buttons.
  if (digitalRead(b5) || digitalRead(b6)){
    if (cursor_y == 0) {
      exit_settings();
    }
    if (cursor_x == 0) {
      if (cursor_y == 3) {
        // load game from slot 1 in eeprom.
        load_slot_from_rom(1);
        display.clearDisplay();
        display.setTextSize(1);
        display.setTextColor(BLACK);
        display.setCursor(0,20);
        display.println(" Loaded Slot 1");
        exit_settings();
      }
      if (cursor_y == 4) {
        // load game from slot 2 in eeprom.
        display.clearDisplay();
        load_slot_from_rom(2);
        display.setTextSize(1);
        display.setTextColor(BLACK);
        display.setCursor(0,20);
        display.println(" Loaded Slot 2");
        exit_settings();
      }
    }
    if (cursor_x == 1) {
      if (cursor_y == 3) {
        // save game to slot 1 in eeprom.
        save_slot_to_rom(1);
        display.clearDisplay();
        display.setTextSize(1);
        display.setTextColor(BLACK);
        display.setCursor(0,20);
        display.println(" Saved Slot 1");
        exit_settings();
      }
      if (cursor_y == 4) {
        // save game to slot 2 in eeprom.
        save_slot_to_rom(2);
        display.clearDisplay();
        display.setTextSize(1);
        display.setTextColor(BLACK);
        display.setCursor(0,20);
        display.println(" Saved Slot 2");
        exit_settings();
      }
    }
  }
}

/**
 *  Load the correct drum array to the screen.
 */
void load_drum(){
  for (int16_t e = 0; e < Rows; e++) {
    for (int16_t i = 0; i < Cols; i++) {
      if (drum_array[e][i] == 1) {
        display.fillRect((i * 5) + 2, (e * 5) + 2, 2, 2, BLACK);
      }
    }
  }
}

/**
 *  Leave the settings page from various points.
 */
void exit_settings(){
  display.display();
  delay(1000);
  display.clearDisplay();
  count = 0;
  cursor_x = 0;
  cursor_y = 0;
  leave_settings = 1;
  draw_grid();
  load_drum();
}

/**
 *	Draw the settings screen.
 */
void draw_settings() {
  if(unselect_settings){
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(BLACK);
    display.setCursor(0,0);
    display.println("Settings  Exit");
    display.setCursor(0,10);
    display.println("Contrast  :");
    display.setCursor(65,10);
    display.println(contrast);
    display.setCursor(0,20);
    display.println("Brightness:");
    display.setCursor(65,20);
    display.println(brightness);
    display.setCursor(0,30);
    display.println("1 : load save");
    display.setCursor(0,40);
    display.println("2 : load save");
    should_update = 1;
  }
  unselect_settings = 0;
}

/**
 *	Move the highlighter around the settings screen.
 */
void move_highligher() {
  if(should_update){
    if (cursor_y == 0) {
      unselect_settings = 1;
      draw_settings();
      display.setCursor(60,0);
      display.setTextColor(WHITE, BLACK);
      display.println("Exit");
    }
    if (cursor_y == 1) {
      unselect_settings = 1;
      draw_settings();
      display.setCursor(65,10);
      display.setTextColor(WHITE, BLACK);
      display.println(contrast);
    }
    if (cursor_y == 2) {
      unselect_settings = 1;
      draw_settings();
      display.setCursor(65,20);
      display.setTextColor(WHITE, BLACK);
      display.println(brightness);
    }
    if (cursor_y == 3) {
      if (cursor_x == 0) {
        unselect_settings = 1;
        draw_settings();
        display.setCursor(24,30);
        display.setTextColor(WHITE, BLACK);
        display.println("load");
      }
      if (cursor_x == 1) {
        unselect_settings = 1;
        draw_settings();
        display.setCursor(54,30);
        display.setTextColor(WHITE, BLACK);
        display.println("save");
      }
    }
    if (cursor_y == 4) {
      if (cursor_x == 0) {
        unselect_settings = 1;
        draw_settings();
        display.setCursor(24,40);
        display.setTextColor(WHITE, BLACK);
        display.println("load");
      }
      if (cursor_x == 1) {
        unselect_settings = 1;
        draw_settings();
        display.setCursor(54,40);
        display.setTextColor(WHITE, BLACK);
        display.println("save");
      }
    }
  }
}

/**
 *	Draw the 6 by 16 grid.
 */
void draw_grid() {
  for (int16_t e = 0; e < Rows; e++) {
    for (int16_t i = 0; i < Cols; i++) {
      display.drawRect(i*5, e*5, 6, 6, BLACK);
    }
  }
}

/**
 *	Loop through rows and columns in the array and set to 0.
 */
void reset_drum_array(){
  for (int16_t e = 0; e < Rows; e++) {
    for (int16_t i = 0; i < Cols; i++) {
      drum_array[e][i] = 0;
    }
  }
}

/**
 *	Turn all fets off.
 */
void turn_all_off(){
  for (int16_t e = 0; e < Rows; e++) {
    digitalWrite(solenoid[e], LOW);
  }
}

/**
 *	Main Loop.
 *  Step though main processes.
 */
void loop() {
  get_move_buttons(0,15,0,5);
  move_cursor();
  move_counter();
  get_cursor_button();
  get_function_button();
  show_bpm();
  update_screen();
  do_bangs();
  master_increment();
  master_delay();
}

/**
 *	Check the array at point count to see if solenoids should bang.
 */
void do_bangs(){
  if (should_increment) {
    for (int16_t e = 0; e < Rows; e++) {
      if (drum_array[e][count] == 1) {
        digitalWrite(solenoid[e], HIGH);
        solTimer[e] = millis();
        solOn[e] = 1;
      }
    }
  }
  for (int16_t e = 0; e < Rows; e++) {
    if (solOn[e]) {
      if ((solTimer[e] + SolOnTime) < millis()) {
        digitalWrite(solenoid[e], LOW);
        solOn[e] = 0;
      }
    }
  }
}

/**
 *	Print the temp to screen.
 */
void show_bpm(){
  if(should_update){
    display.fillRect(0, 37, 100, 8, WHITE);
    display.setTextSize(1);
    display.setTextColor(BLACK);
    display.setCursor(0,37);
    bpm_display = bpm; 
    display.println(bpm_display);
  }
}

/**
 *	Update the screen with all the changes.
 */
void update_screen(){
  if(should_update){
    display.display();
  }
  should_update = 0;
}

/**
 *	Draw the cursor at new position and erase old cursor.
 */
void move_cursor(){
  if(should_update){
    display.drawRoundRect((cursor_old_x * 5) + 1, (cursor_old_y * 5) + 1, 4, 4, 2, WHITE);
    display.drawRoundRect((cursor_x * 5) + 1, (cursor_y * 5 )+1, 4, 4, 2, BLACK);
  }
  cursor_old_x = cursor_x;
  cursor_old_y = cursor_y;
}

/**
 *	Poll function button and combo buttons and perform extra functions.
 */
void get_function_button(){
  if (digitalRead(b6)) {
    if (digitalRead(b1)) {
      if (bpm < BPMmax) {
        bpm++;
        should_update = 1;
      }
    }
    else if (digitalRead(b2)) {
      if (bpm > BPMmin) {
        bpm--;
        should_update = 1;
      }
    }
    else if (digitalRead(b3)) {

    }
    else if (digitalRead(b4)) {

    }
    else if (digitalRead(b5)) {
      enter_settings();
    }
  }
}

/**
 *	Poll cursor button and switch drum array location on/off.
 */
void get_cursor_button(){
  if (!digitalRead(b6)) {
    b5_state_new = digitalRead(b5);
    if (b5_state_new != b5_state_old) {
      if (b5_state_new == HIGH) {
        if (drum_array[cursor_y][cursor_x] == 1) {
          drum_array[cursor_y][cursor_x] = 0;
          display.fillRect((cursor_x * 5) + 2, (cursor_y * 5) + 2, 2, 2, WHITE);
        } else {
          drum_array[cursor_y][cursor_x] = 1;
          display.fillRect((cursor_x * 5) + 2, (cursor_y * 5) + 2, 2, 2, BLACK);
        }
        should_update = 1;
      }
    }
    b5_state_old = b5_state_new; 
  }
}

/**
 *	Poll move buttons and increment or decrement drum array location accordingly.
 */
void get_move_buttons(int16_t x_min, int16_t x_max, int16_t y_min, int16_t y_max){

  b1_state_new = digitalRead(b1);
  b2_state_new = digitalRead(b2);
  b3_state_new = digitalRead(b3);
  b4_state_new = digitalRead(b4);
  
  if (!digitalRead(b6)) {
  
    if (b3_state_new != b3_state_old) {
      if (b3_state_new == HIGH) {
        if (cursor_y < y_max) {
          cursor_y++;
          should_update = 1;
        }
      }
    }
    if (b2_state_new != b2_state_old) {
      if (b2_state_new == HIGH) {
        if (cursor_x < x_max) {
          cursor_x++;
          should_update = 1;
        }
      }
    }
    if (b4_state_new != b4_state_old) {
      if (b4_state_new == HIGH) {
        if (cursor_x > x_min) {
          cursor_x--;
          should_update = 1;
        }
      }
    }
    if (b1_state_new != b1_state_old) {
      if (b1_state_new == HIGH) {
        if (cursor_y > y_min) {
          cursor_y--;
          should_update = 1;
        }
      }
    } 
  }
  b1_state_old = b1_state_new;
  b2_state_old = b2_state_new;
  b3_state_old = b3_state_new;
  b4_state_old = b4_state_new;
}

/**
 *	Draw a beat counter at the bottom of the grid at position 'count'.
 */
void move_counter(){
  if(should_increment){
    if (count == 0) {
      display.fillRoundRect(((15 * 5) + 1), 33, 4, 4, 2, WHITE);
    }
    display.fillRoundRect(((count * 5) + 1), 33, 4, 4, 2, BLACK);
    display.fillRoundRect((((count - 1) * 5) + 1), 33, 4, 4, 2, WHITE);
  }
}

/**
 *	Increment the master counter called 'count'.
 */
void master_increment(){
  if(should_increment){
    if (count < (Cols-1)) {
      count++;
    } else {
      count = 0;
    }
  }
  should_increment = 0;
}

/**
 *	Set the bpm_microsecs by delay a certain amount.
 */
void master_delay(){
  time_now = millis();
  bpm_microsecs = (60 / bpm) * 1000;
  // inverse of > bpm = (1000 / microseconds) * 60;
  if((time_now - bpm_microsecs) > time_then ){
    should_increment = 1;
    should_update = 1;
    time_then = millis();
  }
}

