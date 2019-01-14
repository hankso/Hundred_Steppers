/*
 * This is a library for controlling hundreds of steppers with a single
 * arduino. 74HC595 chip is used to extend arduino's GPIO number, which
 * accept serial data and transfer it into parallel output.
 *
 * 74HC595 pin-outs:    |     UNL2803(8 NPN darlington, 50V@500mA max):
 *       ____           |            ____
 *  Q1 -|o   |- VCC     | Q1 -- I1 -|o   |- O1
 *  Q2 -|    |- Q0      | Q2 -- I2 -|    |- O2
 *  Q3 -|    |- DS      | Q3 -- I3 -|    |- O3
 *  Q4 -|    |- OE      | Q4 -- I4 -|    |- O4
 *  Q5 -|    |- STCP    | Q5 -- I5 -|    |- O5
 *  Q6 -|    |- SHCP    | Q6 -- I6 -|    |- O6
 *  Q7 -|    |- MR      | Q7 -- I7 -|    |- O7
 * GND -|____|- Q7'     | Q8 -- I8 -|    |- O8
 *                      | GND--GND -|____|- COMMON
 *
 * Q0-Q7 -- parallel output
 * DS -- dataPin, serial input
 * OE -- enablePin, set to LOW to enable parallel output
 * SHCP -- clockPin, SHift register Clock Pin
 * STCP -- latchPin, STorage register Clock Pin
 * MR -- clearPin, Master Reclear, set Q0-Q7 to LOW
 * Q7' -- serial output to next cascaded DS
 *
 * You may need extra driver chip such as UNL2803 to offer enough
 * current for stepper. Simply connect pins as showed above.
 *
 * Writtern by Hank @page https://github.com/hankso
 *
 * Stepper `24BYJ48` pin value table
 * +---------+-------------------------------+
 * |color pin| 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 |
 * +---------+-------------------------------+
 * |red     5| H | H | H | H | H | H | H | H |
 * |orange  4| L | L |   |   |   |   |   | L |
 * |yellow  3|   | L | L | L |   |   |   |   |
 * |pink    2|   |   |   | L | L | L |   |   |
 * |blue    1|   |   |   |   |   | L | L | L |
 * +---------+-------------------------------+
 */

#ifndef HUNDRED_STEPPERS_H
#define HUNDRED_STEPPERS_H

// Default it is uint8_t(only can be 0, 1, 2 or 3), you may want
// to set int16_t to storage current step since you can use HOME
// command to zero all motors.
// #define stepType uint8_t
#define stepType int

// #define arrayLen(array) sizeof(array)/sizeof(array[0])

class Hundred_Steppers
{
    public:
        // You can connect MR to VCC and OE to GND
        // to simplify your hardeware and circuits.
        Hundred_Steppers(uint16_t nSteppers, uint16_t nSteps,
                         uint8_t dataPin, uint8_t clockPin, uint8_t latchPin,
                         uint8_t clearPin = -1, uint8_t enablePin = -1,
                         uint8_t nStepperLines = 4, uint8_t driveMode = 4,
                         uint8_t speed = 10);

        void
            // control single stepper
            setStepperStep(uint16_t, int),
            // control a list of steppers
            setStepperStep(int *, uint16_t),
            setSpeedRevPerMin(uint16_t),
            setSpeedRadPerSec(uint16_t),
            // move all steppers towards zero
            home(void);
        uint16_t
            getStepperNum(void);
        bool
            // steppers are default enabled if enablePin is offered
            enableSteppers(void),
            disableSteppers(void),
            // this will clear all data in 74HC595 shift registers
            clearShiftStorage(void),
            setStepperNum(uint16_t);

    private:
        volatile uint8_t
            *dataReg,
            *clockReg,
            *latchReg;
        uint8_t
            nStepperLines,
            driveMode,
            dataMask,
            clockMask,
            latchMask,
            clearPin,
            enablePin,
            *cmdList;
        uint16_t
            nSteppers,
            nSteps,
            speedDelay;
        stepType
            *stepTable;
        uint32_t
            lastTime;

        void
            doStep(uint16_t length = 0),
            fastShiftOut(uint8_t);
        uint16_t
            stepsToMove(int *, uint16_t);
};

#endif
