








/*
  Name:       ccoop controller take 2.ino
  Created:  12/9/2019 11:33:56 AM
  Author:     DESKTOP-IIR28C4\chris
*/


#include <FiniteStateMachine.h>
#include <MyMetro.h>
#include <RGBLed.h>
//#include <C:\Users\chris\OneDrive\Documents\Arduino\libraries\RGBLed-master\src\RGBLed.h>
#include <Average.h>  // library for rolling average of light level



#define ledRPin 11
#define ledGPin 10
#define ledBPin 9
#define remPin 4
#define LDRPin  A1

#define OutA 2
#define OutB 3
#define CSensPin A0
#define remPin 4


RGBLed led(ledRPin, ledGPin, ledBPin, COMMON_CATHODE);

Average<float> averageLDR(10);

boolean remCurState;
boolean remPrevState;
boolean remChange =false;
boolean remOpen = false;
boolean remClose = false;




bool override = false;

float current;
bool dir;
bool prevDir;

int const LDRDusk = 550;  //will be 20 for production
int const LDRDawn = 50;
int const Bright = 1; int const Dark = 2; int const Unknown = 0;
float lightLevel;
int light;

int alertDirection;
const int alertClose = 0;
const int alertOpen = 1;

int initDir;
const int initClose = 0;
const int initOpen = 1;

//#define DEBUG 1  //1 for debugging 0 for normal

#if !DEBUG
//Normal timers
MyMetro openCloseTime = MyMetro(120000); //2 minutes
MyMetro ButtonDebounce = MyMetro(300);
MyMetro OneHourDelay = MyMetro(3600000); //3600000 ** one hour
MyMetro DisplayUpdate = MyMetro(2000); //2 seconds
MyMetro RetryTimer = MyMetro(1800000); //1800000 **30 minutes
MyMetro StartTimer = MyMetro(10000); //20 seconds
MyMetro LedUpdate = MyMetro(5000); //5 seconds
MyMetro OverrideTimer = MyMetro(7200000); //1hr


#else // DEBUG mode timers
MyMetro openCloseTime = MyMetro(120000); //2 minutes
MyMetro ButtonDebounce = MyMetro(300);
MyMetro OneHourDelay = MyMetro(120000); // 2 minutes
MyMetro DisplayUpdate = MyMetro(2000); //2 seconds
MyMetro RetryTimer = MyMetro(60000); // 2 minutes
MyMetro StartTimer = MyMetro(10000); //20 seconds
MyMetro LedUpdate = MyMetro(5000); //4 seconds
MyMetro OverrideTimer = MyMetro(160000); //10 seconds
#endif



//Door Controller State Machine
void enterOpen();
void updateOpen();
void exitOpen();

void enterOpening();
void updateOpening();
void exitOpening();

void enterClosing();
void updateClosing();
void exitClosing();

void enterClosed();
void updateClosed();
void exitClosed();
void enterAlert();
void updateAlert();
void exitAlert();
void enterInitialize();
void updateInitialize();

State Initialize = State(enterInitialize,updateInitialize,NULL);
State Opening = State(enterOpening, updateOpening, exitOpening);
State Open = State(enterOpen, updateOpen, exitOpen);
State Closing = State(enterClosing, updateClosing, exitClosing);
State Closed = State(enterClosed, updateClosed, exitClosed);
State Alert = State(enterAlert, updateAlert, exitAlert);

FiniteStateMachine CoopController = FiniteStateMachine(Initialize);
int n = 0;
float actuatorCurrent() {
  float c = 0;
  for (n = 1; n <= 5; n++) {
    c = c + float((float(analogRead(CSensPin)) - 508) * 20 / 508);
    delay(100);
  }
  if (abs(c/5) < 0.13)  return 0;
  else return c / 5;
}

void openDoor() {
  digitalWrite(OutA, false);
  digitalWrite(OutB, false);
  digitalWrite(OutA, true);
}

void closeDoor() {
  digitalWrite(OutA, false);
  digitalWrite(OutB, false);
  digitalWrite(OutB, true);
}

void stopDoor() {
  digitalWrite(OutA, false);
  digitalWrite(OutB, false);
}


//int const LDRDusk = 680;  //will be 20 for production
//int const LDRDawn = 40;
//int const Bright = 1; int const Dark = 2; int const Unknown = 0;

void checkLight() {
  lightLevel = analogRead(LDRPin);
  averageLDR.push(lightLevel);
  if (averageLDR.mean() > LDRDusk) { light = Dark; }
  else if (averageLDR.mean() < LDRDawn) { light = Bright; }
  else { light = Unknown; }
}// end checkLight


void enterOpen() {}

void updateOpen() {

  if (LedUpdate.check()) {
    if (override) led.multiFlash(RGBLed::BLUE, 25, 100, 5);
    else led.flash(RGBLed::BLUE, 100);
    LedUpdate.reset();
  }
  if (remChange) {
    CoopController.transitionTo(Closing);
    OverrideTimer.reset();
    remChange = false;
  }
  if (OverrideTimer.check()) override = false;
  if (!override && light == Dark) CoopController.transitionTo(Closing);
}

void exitOpen() {}

void enterOpening() {
  openCloseTime.reset();
  openDoor();
  delay(1000);//allow it to start moving
}

void updateOpening() {

  if (LedUpdate.check()) {
    led.multiFlash(RGBLed::BLUE, 25, 100, 10);
  LedUpdate.reset();
  }
  if (openCloseTime.check()) {
    CoopController.transitionTo(Alert);
    alertDirection = alertOpen;
  }
  else if (actuatorCurrent() == 0) { CoopController.transitionTo(Open); }
  else if (remChange) {
    remChange = false;
    CoopController.transitionTo(Closing);
    }
}

void exitOpening() {}

void enterClosing() { 
  closeDoor();
  openCloseTime.reset();
  delay(1000);//allow it to start movingrem
}

void updateClosing() {

  if (LedUpdate.check()) {
    led.multiFlash(RGBLed::GREEN, 50, 100, 10);
  LedUpdate.reset();
  }
  if (openCloseTime.check()) {
    CoopController.transitionTo(Alert);
    alertDirection = alertClose;
  }
  else if (actuatorCurrent() == 0) { CoopController.transitionTo(Closed); }
  else if (remChange) {
    CoopController.transitionTo(Opening);
    remChange = false;
  }
   
}

void exitClosing() {}

void enterClosed() {}

void updateClosed() {
  if (LedUpdate.check()) {
    if (override) led.multiFlash(RGBLed::GREEN, 25, 100, 5);
    else led.flash(RGBLed::GREEN, 100);//Closed
  
  LedUpdate.reset();
  }
  if (remChange) {
    CoopController.transitionTo(Opening);
    OverrideTimer.reset();
    remChange = false;
  }
  if (OverrideTimer.check()) override = false;
  if (!override && light == Bright) CoopController.transitionTo(Opening);

}

void exitClosed() {}

void enterAlert() {
  stopDoor();
  LedUpdate.interval(200);
  RetryTimer.reset(); 
}

void updateAlert() {
  
  if (LedUpdate.check()) {
  led.flash(RGBLed::RED, 1000);//Alert
  LedUpdate.reset();
  }
  if (RetryTimer.check())
    if (alertDirection == alertOpen) CoopController.transitionTo(Opening);
    else CoopController.transitionTo(Closing); 
}
void exitAlert() { LedUpdate.interval(5000); }



void enterInitialize() {
  if (light == Bright) {
    initDir = initOpen;
    openDoor();
  }
  else {
    initDir = initClose;
    closeDoor();
  }
  StartTimer.reset();
  openCloseTime.reset();
  delay(1000);//allow it to start moving
}
void updateInitialize() {
  led.flash(RGBLed::GREEN, 100);
  if (StartTimer.check()) {
    if (!openCloseTime.check() && actuatorCurrent() == 0) {
      if (initDir == initOpen) CoopController.transitionTo(Open);
      else CoopController.transitionTo(Closed);
    }
  if (openCloseTime.check()) CoopController.transitionTo(Alert);
  }
}

void checkRem() {
  int s = digitalRead(remPin);
  if (s != remCurState) {
    delay(30);
    s = digitalRead(remPin);
    if (s != remCurState) { 
      remChange = true; 
      remCurState = !remCurState;
      override = true;
      OverrideTimer.reset();
    }
    else remChange = false;
    led.multiFlash(RGBLed::WHITE, 50, 100,5);
  }
}

String currentStateName() {
  if (CoopController.isInState(Initialize)) return "Initialize";
  if (CoopController.isInState(Opening)) return "Opening";
  if (CoopController.isInState(Open)) return "Open";
  if (CoopController.isInState(Closing)) return "Closing";
  if (CoopController.isInState(Closed)) return "Closed";
  if (CoopController.isInState(Alert)) return "Alert";
}//end currentStateName

void setup()
{
  pinMode(OutA, OUTPUT);
  pinMode(OutB, OUTPUT);
  digitalWrite(OutA, false);
  digitalWrite(OutB, false);

  pinMode(LDRPin, INPUT);
  
  pinMode(CSensPin, INPUT);

  Serial.begin(115200);

  pinMode(remPin, INPUT_PULLUP);
  remCurState = digitalRead(remPin);
  remPrevState = remCurState;
  remChange = false;

  override = false;

}


void loop()
{
  led.off();
  checkRem();
  checkLight();
  float ACTUATOR = actuatorCurrent();
  CoopController.update();
  //Serial.println(currentStateName());
  delay(500);
}
