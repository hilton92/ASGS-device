
/* Author: Benjamin Hilton
   Date: June 2018
   Developed for the BYU Voice Lab
   Copyright 2018, BYU Voice Lab, All rights reserved
*/


#include <Wire.h> // For IC2 communication
#include <hd44780.h> // For LCD screen                       
#include <hd44780ioClass/hd44780_I2Cexp.h> 

hd44780_I2Cexp lcd(0x27); //create LCD object, with I2C address

int val = 0;
int in_byte = 0;

String string;
char value;

int current_constriction = 19; // ranges from 19 to 100
int desired_constriction = 19; // ranges from 19 to 100
int BootMode = 0; // set to determine if the device is in solo mode or labview mode

// define the pins for the stepper motors
int StepperA_PUL = 47;
int StepperA_DIR = 49;
int StepperA_ENA = 51;
int StepperB_PUL = 46;
int StepperB_DIR = 48;
int StepperB_ENA = 50;
int StepperC_PUL = 39;
int StepperC_DIR = 41;
int StepperC_ENA = 43;
int StepperD_PUL = 38;
int StepperD_DIR = 40;
int StepperD_ENA = 42;


// define the pins for the limit switches (Limit switch A goes with Stepper A, etc.)
int LimitA_signal = 29;
int LimitB_signal = 27;
int LimitC_signal = 25;
int LimitD_signal = 23;

// the pin for the switch that is used to select the mode
int ModeSwitchPin = 22;

// the pins for the push buttons
int UpButton = 26;
int DownButton = 24;


// class definition
class StepperMotor {
    int PULpin;
    int DIRpin;
    int ENApin;
    int CurrentPosition;
    int Steps_from_zero_to_starting;
    int LimitSwitchPin;
  public:
    StepperMotor(int pul, int dir, int ena, int steps, int limit_switch) {
      PULpin = pul;
      DIRpin = dir;
      ENApin = ena;
      Steps_from_zero_to_starting = steps;
      LimitSwitchPin = limit_switch;
      CurrentPosition = 5000; //something sufficiently large so that the position never becomes negative
    }


    void ZeroStepper() {
      //turn the stepper motor back one by 1 until the limit switch is reached.
      int counter = 0;
      while (digitalRead(LimitSwitchPin) == HIGH) {
        TakeSteps(-1);
        counter = counter + 1;
      }
      Serial.print("Number of Steps: ");
      Serial.print(counter);
      //set the CurrentPosition to zero
      
      CurrentPosition = 0;

      //automatically move forward to zero percent stenosis
      TakeSteps(Steps_from_zero_to_starting);
      CurrentPosition = CurrentPosition + Steps_from_zero_to_starting;
    }


    void TakeSteps(int steps) {
      //when this function is called, the code will move the stepper motor the selected number of steps. PWM happens here.
      //reverse the sign so that positive steps are positive direction
      CurrentPosition = CurrentPosition + steps;
      steps = -steps;
      if (steps > 0) {
        //move in the positive direction
        for (int i = 0; i < steps; i++) {
          digitalWrite(DIRpin, LOW);
          digitalWrite(ENApin, HIGH);
          digitalWrite(PULpin, HIGH);
          delayMicroseconds(50);
          digitalWrite(PULpin, LOW);
          delay(1);
        }
        
      }
      else if (steps < 0) {
        //move in the negative direction
        steps = abs(steps);
        for (int i = 0; i < steps; i++) {
          digitalWrite(DIRpin, HIGH);
          digitalWrite(ENApin, HIGH);
          digitalWrite(PULpin, HIGH);
          delayMicroseconds(50);
          digitalWrite(PULpin, LOW);
          delay(1);
        }
      }
    }


    void Update() {
      //this function tells the stepper to move from current constriction to desired constriction
      int steps = calculateSteps();
      //move the amount of steps
      TakeSteps(steps);
      
    }


    int calculateSteps() {
      // given the current and desired constriction, do math to get the number of steps
      // There will be a mathematical function, determined by testing. Something along the lines of:
      int steps = (desired_constriction - current_constriction) * 46;;
      return steps;
    }
};


//initialize all stepper motors with pin values
  StepperMotor StepperA(StepperA_PUL, StepperA_DIR, StepperA_ENA, 488, LimitA_signal);
  StepperMotor StepperB(StepperB_PUL, StepperB_DIR, StepperB_ENA, 336, LimitB_signal);
  StepperMotor StepperC(StepperC_PUL, StepperC_DIR, StepperC_ENA, 512, LimitC_signal);
  StepperMotor StepperD(StepperD_PUL, StepperD_DIR, StepperD_ENA, 527, LimitD_signal);


void setup() {

  pinMode(StepperA_PUL, OUTPUT);
  pinMode(StepperA_DIR, OUTPUT);
  pinMode(StepperA_ENA, OUTPUT);
  pinMode(StepperB_PUL, OUTPUT);
  pinMode(StepperB_DIR, OUTPUT);
  pinMode(StepperB_ENA, OUTPUT);
  pinMode(StepperC_PUL, OUTPUT);
  pinMode(StepperC_DIR, OUTPUT);
  pinMode(StepperC_ENA, OUTPUT);
  pinMode(StepperD_PUL, OUTPUT);
  pinMode(StepperD_DIR, OUTPUT);
  pinMode(StepperD_ENA, OUTPUT);
  pinMode(UpButton, INPUT);
  pinMode(DownButton, INPUT);

  Serial.begin(9600);
 
  
  // zero stepper motors
  StepperA.ZeroStepper();
  StepperB.ZeroStepper();
  StepperC.ZeroStepper();
  StepperD.ZeroStepper();

  // initialize the LCD screen
  lcd.begin(20,4);

  // hide the cursor
  lcd.noCursor();

  // check which mode to boot up in
  BootMode = checkMode();
  if (BootMode == 0) { // solo mode
    //do not activate serial communication
    lcd.clear();
    lcd.print("The device has been ");
    lcd.setCursor(0,1);
    lcd.print("booted in solo mode.");
    lcd.setCursor(0,2);
    lcd.print("Restart the device ");
    lcd.setCursor(0,3);
    lcd.print("to change.");
    delay(1000);
  }
  else if (BootMode == 1) { //  serial mode
    //activate serial communication
    lcd.clear();
    lcd.print("The device has been ");
    lcd.setCursor(0,1);
    lcd.print("booted in Labview ");
    lcd.setCursor(0,2);
    lcd.print("mode. Restart the ");
    lcd.setCursor(0,3);
    lcd.print("device to change.");
    delay(1000);
  }

  lcd.clear();
  lcd.print("Desired       Actual");
  lcd.setCursor(0, 1);
  lcd.print("Percent      Percent");
}

void loop() {

  if (BootMode == 0){//this is for solo mode
   
    int changed1 = 0;
    int changed2 = 0;
    int changed3 = 0;
  
    changed1 = getDesiredConstriction();
    if (changed1 == 1){
      printUpdate();
      delay(100);
    }
 
    changed2 = getDesiredConstriction();
    if (changed2 == 1){
      printUpdate();
      delay(100);
    }

    changed3 = getDesiredConstriction();
    if (changed3 == 1){
      printUpdate();
      delay(100);
    }

    printUpdate();
  
    if (changed1 == 1 && changed2 == 1 && changed3 == 1){
      //repeat the loop without moving stepper motors
    }
    else{
      if (desired_constriction > current_constriction){
        MoveSteppersPositive();
      }
      if (desired_constriction < current_constriction){
        MoveSteppersNegative();
      }
    }
  } 

  else{//this is for labview mode
    //read the value from labview if available

    if (Serial.available() > 0){
      in_byte = Serial.read();
      if (in_byte > 18 && in_byte < 101){
        desired_constriction = in_byte;
      }
      Serial.println(current_constriction);
    }
    
    if (desired_constriction > current_constriction){
        MoveSteppersPositive();
    }
    if (desired_constriction < current_constriction){
        MoveSteppersNegative();
    }
    
    printUpdate();  
  }
}

void MoveSteppersPositive(){
  double buffer = 0;
  if(current_constriction >= 20 && current_constriction < 24){
    buffer = -0.18;
  }
  if(current_constriction >= 24 && current_constriction < 28){
    buffer = -0.04;
  }
  if(current_constriction >= 28 && current_constriction < 32){
    buffer = -0.12;
  }
  if(current_constriction >= 32 && current_constriction < 36){
    buffer = 0.016;
  }
  if(current_constriction >= 36 && current_constriction < 40){
    buffer = -0.01;
  }
  if(current_constriction >= 40 && current_constriction < 44){
    buffer = -0.05;
  }
  if(current_constriction >= 44 && current_constriction < 48){
    buffer = 0.04;
  }
  if(current_constriction >= 48 && current_constriction < 52){
    buffer = -0.04;
  }
  if(current_constriction >= 52 && current_constriction < 56){
    buffer = -0.05;
  }
  if(current_constriction >= 56 && current_constriction < 60){
    buffer = -0.05;
  }
  if(current_constriction >= 60 && current_constriction < 64){
    buffer = 0.07;
  }
  if(current_constriction >= 64 && current_constriction < 68){
    buffer = 0.14;
  }
  if(current_constriction >= 68 && current_constriction < 72){
    buffer = -0.04;
  }
  if(current_constriction >= 72 && current_constriction < 76){
    buffer = -0.25;
  }
  if(current_constriction >= 76 && current_constriction < 80){
    buffer = -0.15;
  }
  if(current_constriction >= 80 && current_constriction < 84){
    buffer = -0.25;
  }
  if(current_constriction >= 84 && current_constriction < 88){
    buffer = -0.18;
  }
  if(current_constriction >= 88 && current_constriction < 92){
    buffer = 0.2;
  }
  if(current_constriction >= 92 && current_constriction < 96){
    buffer = 0.22;
  }
  if(current_constriction >= 96 && current_constriction < 100){
    buffer = 0.32;
  }

  int factor = buffer * -30;
  
  if(current_constriction > 69){
    StepperA.TakeSteps(10 + factor);
    StepperB.TakeSteps(10 + factor);
    StepperC.TakeSteps(10 + factor);
    StepperD.TakeSteps(10 + factor);
    StepperA.TakeSteps(12);
    StepperB.TakeSteps(12);
    StepperC.TakeSteps(12);
    StepperD.TakeSteps(12);
    current_constriction = current_constriction + 1; 
  }
  else{
    StepperA.TakeSteps(23 + factor);
    StepperB.TakeSteps(23 + factor);
    StepperC.TakeSteps(23 + factor);
    StepperD.TakeSteps(23 + factor);
    StepperA.TakeSteps(23);
    StepperB.TakeSteps(23);
    StepperC.TakeSteps(23);
    StepperD.TakeSteps(23);
    current_constriction = current_constriction + 1;
  }
  
}

void MoveSteppersNegative(){
  double buffer = 0;
  if(current_constriction >= 20 && current_constriction < 24){
    buffer = -0.18;
  }
  if(current_constriction >= 24 && current_constriction < 28){
    buffer = -0.04;
  }
  if(current_constriction >= 28 && current_constriction < 32){
    buffer = -0.12;
  }
  if(current_constriction >= 32 && current_constriction < 36){
    buffer = 0.016;
  }
  if(current_constriction >= 36 && current_constriction < 40){
    buffer = -0.01;
  }
  if(current_constriction >= 40 && current_constriction < 44){
    buffer = -0.05;
  }
  if(current_constriction >= 44 && current_constriction < 48){
    buffer = 0.04;
  }
  if(current_constriction >= 48 && current_constriction < 52){
    buffer = -0.04;
  }
  if(current_constriction >= 52 && current_constriction < 56){
    buffer = -0.05;
  }
  if(current_constriction >= 56 && current_constriction < 60){
    buffer = -0.05;
  }
  if(current_constriction >= 60 && current_constriction < 64){
    buffer = 0.07;
  }
  if(current_constriction >= 64 && current_constriction < 68){
    buffer = 0.14;
  }
  if(current_constriction >= 68 && current_constriction < 72){
    buffer = -0.04;
  }
  if(current_constriction >= 72 && current_constriction < 76){
    buffer = -0.25;
  }
  if(current_constriction >= 76 && current_constriction < 80){
    buffer = -0.15;
  }
  if(current_constriction >= 80 && current_constriction < 84){
    buffer = -0.25;
  }
  if(current_constriction >= 84 && current_constriction < 88){
    buffer = -0.18;
  }
  if(current_constriction >= 88 && current_constriction < 92){
    buffer = 0.2;
  }
  if(current_constriction >= 92 && current_constriction < 96){
    buffer = 0.22;
  }
  if(current_constriction >= 96 && current_constriction < 100){
    buffer = 0.32;
  }

  int factor = buffer * -30;

  
  if(current_constriction > 70){
    StepperA.TakeSteps(-10 - factor);
    StepperB.TakeSteps(-10 - factor);
    StepperC.TakeSteps(-10 - factor);
    StepperD.TakeSteps(-10 - factor);
    StepperA.TakeSteps(-12);
    StepperB.TakeSteps(-12);
    StepperC.TakeSteps(-12);
    StepperD.TakeSteps(-12);
    current_constriction = current_constriction - 1; 
  }
  else{
    StepperA.TakeSteps(-23 - factor);
    StepperB.TakeSteps(-23 - factor);
    StepperC.TakeSteps(-23 - factor);
    StepperD.TakeSteps(-23 - factor);
    StepperA.TakeSteps(-23);
    StepperB.TakeSteps(-23);
    StepperC.TakeSteps(-23);
    StepperD.TakeSteps(-23);
    current_constriction = current_constriction - 1;
  }
}


void printUpdate() {
  
  String currentString = String(current_constriction);
  String desiredString = String(desired_constriction);
  lcd.setCursor(0, 3);
  //format the output, cuz this ain't python and there's no function that does this...
  String output1 = "";
  String output2 = "";
  String output3 = "";
  String output4 = "";
  if(desired_constriction < 100){
    output1 = "0";
  }
  if(desired_constriction < 10){
    output2 = "0";
  }
  if(current_constriction < 100){
    output3 = "0";
  }
  if(current_constriction < 10){
    output4 = "0";
  }
  lcd.print("  " + output1 + output2 + desiredString + "%         " + output3 + output4 + currentString + "% ");

}


int getDesiredConstriction() {
  int currentDesired = desired_constriction;
  int changed = 0;
  //check the up button
  if (digitalRead(UpButton) == LOW && currentDesired < 100) {
    currentDesired = currentDesired + 1;
    changed = 1;
  }
  if (digitalRead(DownButton) == LOW && currentDesired > 19) {
    currentDesired = currentDesired - 1;
    changed = 1;
  }
  desired_constriction = currentDesired;
  return changed;
}



int checkMode() {
  lcd.clear();
  lcd.print("Selecting mode...");
  delay(1500);
  int val = digitalRead(ModeSwitchPin);
  if (val == LOW) {
    return 0; //0 for solo mode
  }
  if (val == HIGH) {
    return 1; //1 for serial mode
  }
}




