#include <Arduino.h>
#include <Servo.h>
#include <SCMD.h>
#include <SCMD_config.h>
#include <SparkFun_Qwiic_OLED.h>
#include <Buzzer.h>

// Define buzzer object
int buzzerPin = 12;
Buzzer buzzer(buzzerPin);
int tim = 100;


// Define servo object
Servo swingArm;
int pos = 0;
int Servopin = 13;

// Define wheel and brush motors
SCMD MotorDriver;

// Define buttons
int leftButtonPin = 5;
int rightButtonPin = 9;
int startButtonPin = 6;

int fanPin = 10;
int pumpPin = 11;

// Create wash variables
int cycleTime = 20;
float timeRemaining = 0.0;

// Define OLED display
QwiicMicroOLED myOLED;
char pout[30];

// Set up machine states
enum Machinestates {
  WAITING,   // 0
  DRY_SCRUB, // 1
  WET_SCRUB, // 2
  DRYING,    // 3
  COMPLETE   // 4
};

enum WashCycles{
  DRYSCRUB,  // 0
  QUICKWASH, // 1
  DEEPCLEAN  // 2
};
int WashTimes[3] = {21, 42, 63};

Machinestates currentState = WAITING;

WashCycles currentCycle = DRYSCRUB;

void setup() {
  Serial.begin(9600);

  delay(1000); // Wait for Serial to initialize

  buzzer.begin(10);

  pinMode(leftButtonPin, INPUT_PULLUP);
  pinMode(rightButtonPin, INPUT_PULLUP);
  pinMode(startButtonPin, INPUT_PULLUP);
  pinMode(fanPin, OUTPUT);
  pinMode(pumpPin, OUTPUT);

  swingArm.attach(Servopin);

  MotorDriver.settings.commInterface = I2C_MODE;
  MotorDriver.settings.I2CAddress = 0x5D;
   while (MotorDriver.begin() != 0xA9 ) //Wait until a valid ID word is returned
  {
    Serial.println( "ID mismatch, trying again" );
    delay(500);
  }
  Serial.println( "ID matches 0xA9" );
  Serial.print("Waiting for enumeration...");
  while (MotorDriver.ready() == false );
  Serial.println("Done.");
  Serial.println();
  while (MotorDriver.busy() );
  MotorDriver.enable();

   while(!myOLED.begin()){
    delay(1000);
  }
  Serial.println("Everything started!!!!!");
}

void loop() {
  switch(currentState) {

    case WAITING:
    swingArm.write(0);
    delay(1000);

    while(digitalRead(startButtonPin)){
      if(!digitalRead(rightButtonPin)){
        currentCycle = (WashCycles)((currentCycle + 1) % 3);
        delay(300);
      }
      if(!digitalRead(leftButtonPin)){
        currentCycle = (WashCycles)((currentCycle - 1));
        if (currentCycle < 0) currentCycle = DEEPCLEAN;
        delay(300);
      }
      myOLED.erase();
      sprintf(pout, "Wash Cycle");
      myOLED.text(0,0,pout);
      sprintf(pout, currentCycle == DRYSCRUB ? "Dry Scrub" : currentCycle == QUICKWASH ? "Quick Wash" : "Deep Clean");
      myOLED.text(0,12,pout);
      myOLED.display();
    }

    delay(1000);
    currentState = DRY_SCRUB;

      break;

    case DRY_SCRUB: {
    
    swingArm.detach();
    delay(100);
    buzzer.sound(NOTE_C6, tim);
    buzzer.sound(NOTE_E6, tim);
    buzzer.sound(NOTE_G6, tim);
    buzzer.sound(NOTE_C7, tim*3);
    delay(100);
    swingArm.attach(Servopin);
    delay(100);
    swingArm.write(103);
    delay(1000);
    MotorDriver.setDrive(0, 0, 70); // Wheel motor forward
    MotorDriver.setDrive(1, 0, 80); // Brush motor forward

    unsigned long start = millis();

    while (millis() - start < cycleTime * 1000) {
    timeRemaining = WashTimes[currentCycle] - (millis() - start) / 1000;
    myOLED.erase();
    sprintf(pout, "Scrubbing...");
    myOLED.text(0,0,pout);
    sprintf(pout, "%.1f", timeRemaining);
    myOLED.text(0,12,pout);
    myOLED.display();
    }

    MotorDriver.setDrive(0, 0, 0); // Wheel motor stop
    MotorDriver.setDrive(1, 0, 0); // Brush motor stop
    swingArm.write(0);
    delay(1000);

  }
  timeRemaining = WashTimes[currentCycle] - cycleTime;
   myOLED.erase();
   sprintf(pout, "Scrubbing...");
   myOLED.text(0,0,pout);
   sprintf(pout, "%.1f", timeRemaining);
   myOLED.text(0,12,pout);
   myOLED.display();

   delay(1000);
   
   currentState = WET_SCRUB;

      break;

    case WET_SCRUB: {
    
    if(currentCycle != DRYSCRUB){
    swingArm.write(103);
    delay(1000);
    MotorDriver.setDrive(0, 0, 45); // Wheel motor forward
    MotorDriver.setDrive(1, 0, 100); // Brush motor forward
    digitalWrite(pumpPin, HIGH);// Turn on pump

    unsigned long start = millis();

    while (millis() - start < cycleTime * 1000) {
    timeRemaining = WashTimes[currentCycle-1] - (millis() - start) / 1000;
    myOLED.erase();
    sprintf(pout, "Washing...");
    myOLED.text(0,0,pout);
    sprintf(pout, "%.1f", timeRemaining);
    myOLED.text(0,12,pout);
    myOLED.display();
    }

    swingArm.write(0);
    delay(1000);
    MotorDriver.setDrive(0, 0, 0); // Wheel motor stop
    MotorDriver.setDrive(1, 0, 0); // Brush motor stop
    digitalWrite(pumpPin, LOW);// Turn off pump
  }
  timeRemaining = WashTimes[currentCycle-1] - cycleTime;
   myOLED.erase();
   sprintf(pout, "Washing...");
   myOLED.text(0,0,pout);
   sprintf(pout, "%.1f", timeRemaining);
   myOLED.text(0,12,pout);
   myOLED.display();
    delay(1000);}
    currentState = DRYING;

      break;
    
    case DRYING: {

    if(currentCycle == DEEPCLEAN) {
    MotorDriver.setDrive(0, 1, 200);
    digitalWrite(fanPin, HIGH); // Turn on fan
  

    unsigned long start = millis();

    while (millis() - start < cycleTime * 1000) {
    timeRemaining = WashTimes[currentCycle-2] - (millis() - start) / 1000;
    myOLED.erase();
    sprintf(pout, "Drying...");
    myOLED.text(0,0,pout);
    sprintf(pout, "%.1f", timeRemaining);
    myOLED.text(0,12,pout);
    myOLED.display();
    }

    MotorDriver.setDrive(0, 1, 0); // Wheel motor stop
    digitalWrite(fanPin, LOW); // Turn off fan

    timeRemaining = WashTimes[currentCycle-2] - cycleTime;
   myOLED.erase();
   sprintf(pout, "Drying...");
   myOLED.text(0,0,pout);
   sprintf(pout, "%.1f", timeRemaining);
   myOLED.text(0,12,pout);
   myOLED.display();
    delay(1000);}
    currentState = COMPLETE;
  }
      break;

    case COMPLETE:
    swingArm.detach();
    delay(100);
    buzzer.sound(NOTE_C6, tim);
    buzzer.sound(NOTE_E6, tim);
    buzzer.sound(NOTE_G6, tim);
    buzzer.sound(NOTE_C7, tim*3);
    delay(100);
    swingArm.attach(Servopin);
    delay(100);

    while(digitalRead(startButtonPin)){
    myOLED.erase();
    sprintf(pout, "Complete!");
    myOLED.text(0,0,pout);
    sprintf(pout, "Good Luck");
    myOLED.text(0,12,pout);
    sprintf(pout, "Driver!");
    myOLED.text(0,22,pout);
    myOLED.display();
    }

    delay(1000);
    currentState = WAITING;

      break;

    default:
    
      break;
  }
}