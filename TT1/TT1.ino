/* Turntable_Build
   Mods by dph
*/

 #include <AccelStepper.h>   // AccelStepper Library
 #include <Keypad.h>   // Keypad Library
 #include "U8glib.h"  // U8glib for Nokia LCD
 #include <Wire.h>

 // Variables to hold entered number on Keypad
 int trackNum = 0;  // calculated from keypresses
 bool reversed = false;

 boolean plus180;
 long stepsfor180 = 1200;  // These 2 for turning the bridge 180

 //  0 is furthest CCW exit and is always 0, for 1, 2, 3, etc substitute the actual 
 //  number of full steps to that exit from the furthest CCW exit
 #define NUMTRACKS 16
 long steps[NUMTRACKS] = {0,30,60,90,150,210,240,270,300,345,375,480,510,600,765,975};
 const int microSteppingPerStep = 40;

 // Variables to hold Distance and CurrentPosition
 String currentposition = "";  // Used for display on Nokia LCD

 // Stepper Travel Variables
 long TravelX;  // Used to store the X value entered in the Serial Monitor
 int move_finished=1;  // Used to check if move is completed
 long initial_homing=-1;  // Used to Home Stepper at startup


 // ==== Keypad Setup =====================================
 const byte ROWS = 4; // Four Rows
 const byte COLS = 4; // Four Columns
 char keys[ROWS][COLS] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
 };
 byte rowPins[ROWS] = {22, 24, 26, 28}; // Arduino pins connected to the row pins of the keypad
 byte colPins[COLS] = {31, 33, 35, 37}; // Arduino pins connected to the column pins of the keypad
 Keypad keypad = Keypad( makeKeymap(keys), rowPins, colPins, ROWS, COLS );  // Keypad Library definition


 // ==== U8glib Setup for Nokia LCD =======================
 #define backlight_pin 11
 U8GLIB_PCD8544 u8g(3, 4, 6, 5, 7);  // Arduino pins connected to Nokia pins:
                                    // CLK=3, DIN=4, CE=6, DC=5, RST=7
                                    
 // ==== AccelStepper Setup ===============================
 AccelStepper stepper(1, A0, A1);  // 1 = Easy Driver interface
                                  // Arduino A0 connected to STEP pin of Easy Driver
                                  // Arduino A1 connected to DIR pin of Easy Driver
                                                                   
 // Define the Pins used
 #define MS1 1  // Arduino pin 1 to EasyDriver MS1
 #define MS2 2  // Arduino pin 2 to EasyDriver MS2
 #define home_switch 9  // Pin 9 connected to Home Switch (IR Beam Break)
 int home_led = 8;  // when lit indicates stepper is home


 void setup(void) {
  //  Light up the LCD backlight LEDS
  analogWrite(backlight_pin, 0);  // Set the Backlight intensity (0=Bright, 255=Dim)
    
  //  AccelStepper speed and acceleration setup
  stepper.setMaxSpeed(200);  // Not too fast or you will have missed steps
  stepper.setAcceleration(50);  //  Same here
  
  // Draw starting screen on Nokia LCD
  


   pinMode(MS1, OUTPUT);
   pinMode(MS2, OUTPUT);   
   pinMode(home_switch, INPUT_PULLUP);
   pinMode(home_led, OUTPUT);


 /* Configure type of Steps on EasyDriver:
 // MS1 MS2
 //
 // LOW LOW = Full Step //
 // HIGH LOW = Half Step //
 // LOW HIGH = A quarter of Step //
 // HIGH HIGH = An eighth of Step //
 */
  
  digitalWrite(MS1, LOW);  // LOW LOW = Full Step, HIGH HIGH = 1/8 Step
  digitalWrite(MS2, LOW); 
   
  delay(5);  // Wait for EasyDriver wake up

 // Start Homing procedure of Stepper Motor at startup

  while (digitalRead(home_switch)) {  // Make the Stepper move CCW until the switch is activated   
    stepper.moveTo(initial_homing);  // Set the position to move to
    initial_homing--;  // Decrease by 1 for next move if needed
    stepper.run();  // Start moving the stepper
    delay(5);
 }

  initial_homing=1;

  while (!digitalRead(home_switch)) { // Make the Stepper move CW until the switch is deactivated
    stepper.moveTo(initial_homing);  
    stepper.run();
    initial_homing++;
    delay(5);
  }

  stepper.setCurrentPosition(steps[0]*microSteppingPerStep);  // n = # of steps in array that is home position

  digitalWrite(home_led, HIGH);  //Turn on the LED

  stepper.setMaxSpeed(50.0);      // Set Max Speed of Stepper (Faster for regular movements)
  stepper.setAcceleration(25.0);  // Set Acceleration of Stepper

  drawnokiascreen(String(""));  // initial screen
  
 }


void loop(){
  
  char keypressed = keypad.getKey();  // Get value of keypad button if pressed
  if (keypressed != NO_KEY){  // If keypad button pressed check which key
    if( keypressed == 'A' ) reversed = !reversed;           // remember reversal state
    else if( keypressed == '*' ) trackNum = 0;              // zero chosen trackNumber
    else if( keypressed == '#' ) {                          // execute the move
      long posn = steps[trackNum];                          // calculate the position
      if( reversed ) posn = posn + stepsfor180;             // reversed?
      long microPosn = posn * microSteppingPerStep;         // convert to microsteps
      stepper.moveTo( microPosn );                          // make move
      if( reversed ) currentposition = String(-trackNum);   // change to string for display
      else currentposition = String(trackNum);
    } else {                                                // deal with number keys
      int ikey = keypressed - '0';                          // convert to an integer
      if( ikey>=0 && ikey<=9 ) {                            // is 0-9, then
        trackNum = trackNum*10 + ikey;  // for tracks > 10  // allow multiple digits
        if(trackNum>NUMTRACKS) trackNum = NUMTRACKS;        // not past the last track
        String sTrack;
        if( reversed ) sTrack = String(-trackNum);          // change to string for display
        else sTrack = String(-trackNum);
        drawnokiascreen( sTrack );                          // draw the screen
      }
    }
  }
}
                
void drawnokiascreen(String y) {
    u8g.firstPage();
    do {
      u8g.drawHLine(0, 15, 84);
      u8g.drawVLine(50, 16, 38);
      u8g.drawHLine(0, 35, 84); 
      u8g.setFont(u8g_font_profont11);
      u8g.drawStr(0, 10, "ENTER TRACK #");
      u8g.setPrintPos(0,29);
      u8g.print(y);  // Put entered number on Nokia lcd    
      u8g.drawStr(62, 29, "#");
      u8g.drawStr(4, 46, "cur-pos");
      u8g.setPrintPos(57,47); 
      u8g.print(currentposition);  //  Display current position of stepper
    }
      while( u8g.nextPage() );
}

