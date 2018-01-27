//  TT2.1 -- Same as TT2, use this for development 
 
 #include <AccelStepper.h>   // AccelStepper Library
 #include <Keypad.h>   // Keypad Library
 #include "U8glib.h"  // U8glib for Nokia LCD
 #include <Wire.h>

 // Variables to hold entered number on Keypad
 int enteredTrack = 0;          // used for track entry
 bool reversed = false;

 boolean plus180;
 long stepsfor180 = 600;  // These 2 for turning the bridge 180

 //  0 is furthest CCW exit and is always 0, for 1, 2, 3, etc substitute the actual 
 //  number of full steps to that exit from the furthest CCW exit
 #define NUMTRACKS 16
 long steps[NUMTRACKS] = {0,30,60,90,150,210,240,270,300,345,375,480,510,600,765,975};
 const int microStepsPerFullStep = 8;

 // Variables to hold Distance, target, and CurrentPosition
 String sCurrentPosition = "";  // Used for display on Nokia LCD
 int targetTrack = 0;           // goal of a move
 String sTargetTrack="";        // Used for display
 
 // Stepper Travel Variables
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
 #define StepperEnable 0  // Arduino pin 0 to EasyStepper Enable
 #define MS1 1            // Arduino pin 1 to EasyDriver MS1
 #define MS2 2            // Arduino pin 2 to EasyDriver MS2
 #define home_switch 9    // Arduino pin 9 to Home Switch (IR Beam Break)
 #define EmergStop 10     // Arduino pin 10 to Emergency stop switch
 int home_led = 8;        // When lit indicates stepper is home


 void setup(void) {

  //  Light up the LCD backlight LEDS
  analogWrite(backlight_pin, 200);  // Set the Backlight intensity (0=Bright, 255=Dim)
    
  //  AccelStepper speed and acceleration setup
  stepper.setMaxSpeed(100);  // Not too fast or you will have missed steps
  stepper.setAcceleration(25);  //  Same here
  
  // Draw starting screen on Nokia LCD
    drawnokiascreen(String(""));

   pinMode(StepperEnable, OUTPUT);
   digitalWrite(StepperEnable, HIGH);  //dph
   pinMode(MS1, OUTPUT);
   pinMode(MS2, OUTPUT);   
   pinMode(home_switch, INPUT_PULLUP);
   pinMode(home_led, OUTPUT);
   pinMode(EmergStop, INPUT_PULLUP);


 /* Configure type of Steps on EasyDriver:
 // MS1 MS2
 //
 // LOW LOW = Full Step //
 // HIGH LOW = Half Step //
 // LOW HIGH = A quarter of Step //
 // HIGH HIGH = An eighth of Step //
 */
 
  digitalWrite(MS1, HIGH);  // HIGH HIGH = 1/8 Step
  digitalWrite(MS2, HIGH);
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

  stepper.setCurrentPosition(steps[0]* microStepsPerFullStep);  // n = # of steps in array that is home position

  digitalWrite(home_led, HIGH);                  //Turn on the LED
  sCurrentPosition = String(targetTrack);        // change to string for display
  drawnokiascreen( "" );                         // refresh the screen

  stepper.setMaxSpeed(100.0);      // Set Max Speed of Stepper (Faster for regular movements)
  stepper.setAcceleration(25.0);  // Set Acceleration of Stepper
 
 }


void loop(){
  stepper.run();
  static bool estop = false;                           // Emergency stop
  //if( !digitalRead(EmergStop) && !estop ) {
  //     digitalWrite(StepperEnable, LOW);             //turn off motor physically
  //     stepper.stop();                               //tell software to stop motor
  //     estop = true;   
  //}

 // At a destination?
  bool once=true;                   // dph
  if(stepper.distanceToGo()==0 && once) {            // update the display only once when reach target
      sCurrentPosition = String(targetTrack);        // change to string for display
      drawnokiascreen( "" );                         // refresh the screen
      reversed = false;                              // reset reversed
      once = false;
  }
  static char keypressed;
  keypressed = keypad.getKey();                      // Get value of keypad button if pressed
  if (keypressed != NO_KEY){                         // If keypad button pressed check which key
    
    if( keypressed == 'A' ) {
      reversed = !reversed;                          // remember reversal state
      sTargetTrack = String(enteredTrack);           // change to string for display
      if( reversed ) 
         sTargetTrack = String("-" + sTargetTrack);  // indicate reversed for display
      drawnokiascreen( sTargetTrack );               // draw the screen
      
    } else if( keypressed == '*' ) {
      enteredTrack = 0;                              // zero chosen track Number 
      drawnokiascreen("");
      
    } else if( keypressed == '#' ) {                  // execute the move
      long posn = steps[enteredTrack];               // calculate the position
      if( reversed ) posn = posn + stepsfor180;      // reversed?
      long microPosn = posn * microStepsPerFullStep;  // convert to microsteps
      estop = false;                                 // allow movement
      stepper.moveTo( microPosn );                   // go to new position
      sCurrentPosition = String("moving");           // indicate a move is in progress
      targetTrack = enteredTrack;                    // remember targetTrack
      if(reversed) sTargetTrack = String(-targetTrack);
      else sTargetTrack = String(targetTrack);
      enteredTrack = 0;                              // prepare for next track input
      drawnokiascreen( sTargetTrack );               // draw the screen
      once = true;                                   // dph allow display at end of travel
      
    } else {                                          // deal with number keys
      int ikey = keypressed - '0';                   // convert to an integer
      if( ikey>=0 && ikey<=9 ) {                     // is 0-9, then
        enteredTrack = enteredTrack*10 + ikey;       // for tracks > 10, allow multiple digits
        if(enteredTrack>NUMTRACKS) 
            enteredTrack = NUMTRACKS;                // not past the last track
        sTargetTrack = String(enteredTrack);
        if( reversed ) 
           sTargetTrack = String("-" + sTargetTrack); // indicate reversed
        drawnokiascreen( sTargetTrack );              // update the screen
      }    
    }
  }
}

void drawnokiascreen(String sEnteredTrack) {
    u8g.firstPage();
    do {
      u8g.drawHLine(0, 15, 84);
      u8g.drawVLine(50, 16, 38);
      u8g.drawHLine(0, 35, 84); 
      u8g.setFont(u8g_font_profont11);
      u8g.drawStr(0, 10, "ENTER TRACK #");
      u8g.setPrintPos(0,29);
      u8g.print(sEnteredTrack);                // Put entered number on Nokia lcd    
      u8g.drawStr(62, 29, "#");
      u8g.drawStr(4, 46, "cur-pos");
      u8g.setPrintPos(57,47); 
      u8g.print(sCurrentPosition);             //  Display current position of stepper
    }
      while( u8g.nextPage() );
}
