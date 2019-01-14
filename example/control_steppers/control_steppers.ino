#include <Hundred_Steppers.h>

#define steppers_num     10  // 10 steppers
#define stepper_line_num 4   // 4-wire 5-phase stepper
#define dataPin          2
#define clockPin         3
#define latchPin         4
#define enablePin        -1  // disabled
#define clearPin         -1  // disabled
#define steps_per_rev    int(16 * (360 / 3.75))
#define driveMode        8   // A-AB-B-BC-C-CD-D-DA
//#define driveMode        4   // A-B-C-D

Hundred_Steppers s = Hundred_Steppers(
    steppers_num, steps_per_rev,
    dataPin, clockPin, latchPin, enablePin, clearPin,
    stepper_line_num, driveMode, 20);


String s1 = String("\n\
===============================================\n\
set stepper step one by one\n\
e.g.\n\
    s.setStepperStep(0, 2);\n\
    stepper No.1 will move forward two steps\n\
===============================================");

String s2 = String("\n\
===============================================\n\
set stepper steps by a plus and minus step list\n\
e.g.\n\
    int step_list[5] = {-2, -1, 0, 1, 2};\n\
    s.setStepperStep(step_list);\n\
    +5 means 5 steps forward\n\
    -3 means 3 steps backwar\n\
===============================================");

String s3 = String("\n\
===============================================\n\
enableSteppers\n\
disableSteppers\n\
clearShiftStorage\n\
setStepperNum\n\
getStepperNum\n\
setSpeedRevPerMin\n\
setSpeedRadPerSec\n\
home\n\
===============================================");


void setup() {
    pinMode(13, OUTPUT);
    Serial.begin(115200);
    Serial.println(s1);
    Serial.print("start time(us): ");
    Serial.println(micros());
    for (uint8_t i = 0; i < steppers_num; i++)
    {
       s.setStepperStep(i, 20*i);
       Serial.print("set stepper ");
       Serial.print(i);
       Serial.print(" to step ");
       Serial.println(20*i);
    }
    Serial.print("finish time(us): ");
    Serial.println(micros());
    Serial.println("hit return to continue");
    Serial.readStringUntil('\n');
    while (!Serial.available()) {;}
    Serial.readStringUntil('\n');


    Serial.println(s2);
    Serial.print("start time(us): ");
    Serial.println(micros());
    int step_list[4] = {1000, 800, -800, -1000};
    s.setStepperStep(step_list, 4);
    Serial.print("finish time(us): ");
    Serial.println(micros());
    Serial.println("hit return to continue");
    Serial.readStringUntil('\n');
    while (!Serial.available()) {;}
    Serial.readStringUntil('\n');


    Serial.println(s3);
    Serial.print("enable steppers: ");
    Serial.println(s.enableSteppers() ? "true" : "false");
    Serial.print("clear registers: ");
    Serial.println(s.clearShiftStorage() ? "true" : "false");
    Serial.print("stepper num: ");
    Serial.println(s.getStepperNum());
    Serial.print("set stepper num: ");
    Serial.println(s.setStepperNum(10) ? "true" : "false");
    Serial.print("stepper num: ");
    Serial.println(s.getStepperNum());
    Serial.println("done!");

    s.home();
}

void loop() {
   // put your main code here, to run repeatedly:
   Serial.println("input stepper index and steps to move(e.g. `5, 5` to move No.5 stepper forward 5 steps.): ");
   while (!Serial.available()) {;}
   digitalWrite(13, HIGH);
   uint16_t index = Serial.parseInt();
   int steps_to_move = Serial.parseInt();
   Serial.readStringUntil('\n');
   String tmp = "moving stepper " + String(index) + " " + String(steps_to_move) + " steps";
   Serial.println(tmp);
   s.setStepperStep(index, steps_to_move);
   digitalWrite(13, LOW);
    // int step_list[4] = {1000, 1000, 1000, 1000};
    // s.setStepperStep(step_list, 4);
}
