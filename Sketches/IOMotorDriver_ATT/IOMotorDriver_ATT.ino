#include<Arduino.h>
#include <Buzzer.h>
#include <AccelStepper.h>

//ins
#define DI01 10
#define DI02 2
#define AI01 6
//#define AI02 5

//outs
//#define DO01 1
#define buzz 4
#define DIR 8
#define STEP 7
#define ENA 3
float timeStamp = 0;
bool runToggle = true;
int historyLength = 5;
int anaRead[5] = {0, 0, 0, 0, 0};
int tempVal = 0;
double motorSpeed = 50;
int buzzCounter = 0;
double speedCutOff = 15;

AccelStepper stepper(AccelStepper::DRIVER, STEP, DIR);
Buzzer buzzer(buzz);

void doBuzz(long buzzTime, long buzzDelay) {
  buzzer.begin(0);
  buzzer.sound(NOTE_C4, buzzTime);
  buzzer.end(buzzDelay);
}

void doBuzzStartup() {
  buzzer.begin(0);
  buzzer.sound(NOTE_C3, 200);
  buzzer.sound(NOTE_C4, 55);
  buzzer.end(100);
}

void smoothAnaRead() {
  tempVal = 0;
  for (int i = 0; i < historyLength - 1; i++) {
    anaRead[i] = anaRead[i + 1];
    tempVal += anaRead[i + 1];
  }
  anaRead[4] = analogRead(AI01);
  tempVal += anaRead[4];
  motorSpeed = map((tempVal / historyLength), 0, 1023, 5, 4000);
}

void setup() {
  Serial.begin(9600);
  pinMode(AI01, INPUT);
  pinMode(DI01, INPUT);
  pinMode(DI02, INPUT);
  //pinMode(DO01, OUTPUT);
  pinMode(buzz, OUTPUT);
  pinMode(DIR, OUTPUT);
  pinMode(STEP, OUTPUT);
  pinMode(ENA, OUTPUT);

  digitalWrite(ENA, HIGH);
  stepper.setMaxSpeed(1000);
  stepper.setAcceleration(50.0);
  stepper.setSpeed(200);

  delay(1000);
  doBuzzStartup();
}


void loop() {
  if (runToggle == true) {
    stepper.setSpeed(motorSpeed);
  } else {
    stepper.setSpeed(0);
  }

  if ((millis() - timeStamp) > 100) {
    smoothAnaRead();

    if (digitalRead(DI01) == LOW ||  digitalRead(DI02) == LOW) {

      runToggle = true;
      digitalWrite(ENA, LOW);

      if (digitalRead(DI02) == LOW) {
        buzzCounter ++;
        if (buzzCounter == 20) {
          doBuzz(25, 0);
          buzzCounter = 0;
        }
      }
    } else {
      runToggle = false;
      digitalWrite(ENA, HIGH);
    }
    timeStamp = millis();
  }
  stepper.runSpeed();
}
