/*
 * This is a library for controlling hundreds of steppers with a single
 * arduino. 74HC595 chip is used to extend arduino's GPIO number, which
 * accept serial data and transfer it into parallel output.
 *
 * 74HC595 pin-outs:    |     UNL2803(8 NPN darlington, 50V@500mA max):
 *       ____           |            ____
 *  Q1 -|o   |- VCC     | Q0 -- I1 -|o   |- O1
 *  Q2 -|    |- Q0      | Q1 -- I2 -|    |- O2
 *  Q3 -|    |- DS      | Q2 -- I3 -|    |- O3
 *  Q4 -|    |- OE      | Q3 -- I4 -|    |- O4
 *  Q5 -|    |- STCP    | Q4 -- I5 -|    |- O5
 *  Q6 -|    |- SHCP    | Q5 -- I6 -|    |- O6
 *  Q7 -|    |- MR      | Q6 -- I7 -|    |- O7
 * GND -|____|- Q7'     | Q7 -- I8 -|    |- O8
 *                      | GND--GND -|____|- COMMON -- VCC
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

#include <Arduino.h>
#include "Hundred_Steppers.h"

Hundred_Steppers::Hundred_Steppers(
        uint16_t nSteppers, uint16_t nSteps,
        uint8_t dataPin, uint8_t clockPin, uint8_t latchPin,
        uint8_t clearPin, uint8_t enablePin,
        uint8_t nStepperLines, uint8_t driveMode, uint8_t speed)
{
    // steppers num
    this->nSteppers = nSteppers;

    // init steppers' step table full of 0
    // stepType: this is defined in head file Hundred_Steppers.h
    this->stepTable = (stepType *)calloc(nSteppers, sizeof(stepType));

    // get pin registers pointer, these GPIO are frequently used.
    this->dataReg   = portOutputRegister(digitalPinToPort(dataPin));
    this->clockReg  = portOutputRegister(digitalPinToPort(clockPin));
    this->latchReg  = portOutputRegister(digitalPinToPort(latchPin));
    this->dataMask  = digitalPinToBitMask(dataPin);
    this->clockMask = digitalPinToBitMask(clockPin);
    this->latchMask = digitalPinToBitMask(latchPin);
    pinMode(dataPin, OUTPUT);
    pinMode(clockPin, OUTPUT);
    pinMode(latchPin, OUTPUT);
    *dataReg &= ~dataMask;
    *clockReg &= ~clockMask;
    *latchReg &= ~latchMask;

    // not necessary pins, also not frequently used.
    if (clearPin != -1)
    {
        this->clearPin = clearPin;
        pinMode(clearPin, OUTPUT);
        digitalWrite(clearPin, HIGH);
    } else this->clearPin = 0;
    if (enablePin != -1)
    {
        this->enablePin = enablePin;
        pinMode(enablePin, OUTPUT);
        digitalWrite(enablePin, LOW);
    } else this->enablePin = 0;

    this->lastTime = micros();
    this->nStepperLines = nStepperLines;
    this->nSteps = nSteps;

    // driver mode
    this->driveMode = driveMode;
    if (driveMode == 4)
    {
        this->cmdList = new uint8_t[4] { B1110, B1101, B1011, B0111 };
    }
    else if (driveMode == 8)
    {
        this->cmdList = new uint8_t[8] { B1110, B1100, B1101, B1001,
                                         B1011, B0011, B0111, B0110 };
    }

    setSpeedRevPerMin(speed);
}

bool Hundred_Steppers::enableSteppers(void)
{
    if (enablePin)
    {
        digitalWrite(enablePin, LOW);
        return true;
    }
    return false;
}

bool Hundred_Steppers::disableSteppers(void)
{
    if (enablePin)
    {
        digitalWrite(enablePin, HIGH);
        return true;
    }
    return false;
}

bool Hundred_Steppers::clearShiftStorage(void)
{
    if (clearPin)
    {
        digitalWrite(clearPin, LOW);
        delay(10);
        digitalWrite(clearPin, HIGH);
        return true;
    }
    return false;
}

void Hundred_Steppers::setSpeedRevPerMin(uint16_t n)
{
    // speedDelay_second * (steps * rev) = 1min
    // speedDelay * steps * rev = 60000000 us
    speedDelay = 60L * 1000L * 1000L / nSteps / n;
}

void Hundred_Steppers::setSpeedRadPerSec(uint16_t n)
{
    // speedDelay_second * steps * rad/2Pi = 1s
    speedDelay = TWO_PI * 1000L * 1000L / nSteps / n;
}

void Hundred_Steppers::setStepperStep(uint16_t n, int steps)
{
    // control single stepper with its index and steps to move
    if (n > nSteppers) return;
    if (steps > 0)
    {
        while (steps--)
        {
            stepTable[n]++;
            doStep(n);
        }
    }
    else if (steps < 0)
    {
        while (steps++)
        {
            stepTable[n]--;
            doStep(n);
        }
    }
}

void Hundred_Steppers::setStepperStep(int * stepsList, uint16_t length)
{
    // control a list of steppers
    length = min(length, nSteppers);
    uint16_t maxChanged;

    // loop until every steps_to_move return to zero
    // e.g. stepsList finally become {0, 0, 0...}
    while (stepsToMove(stepsList, length))
    {
        maxChanged = -1;
        for (uint16_t i = 0; i < length; i++)
        {
            if (stepsList[i] > 0)
            {
                stepsList[i]--;
                stepTable[i]++;
                maxChanged = i;
            }
            else if (stepsList[i] < 0)
            {
                stepsList[i]++;
                stepTable[i]--;
                maxChanged = i;
            }
        }
        if (maxChanged == -1) break;
        doStep(maxChanged);
    }
}

void Hundred_Steppers::home(void)
{
    // Move each steppers to zero position
    // This do exactly setStepperStep(-stepTable)
    stepType stepsList[nSteppers];
    for (uint16_t i = 0; i < nSteppers; i++)
    {
        stepsList[i] = -stepTable[i];
    }
    setStepperStep(stepsList, nSteppers);
}

bool Hundred_Steppers::setStepperNum(uint16_t n)
{
    if (n <= stepTable)
    {
        nSteppers = n;
        return true;
    }
    // resize internal steps array `stepTable`
    stepType * tmp = (stepType *)realloc(stepTable, n * sizeof(stepType));
    if (tmp)
    {
        stepTable = tmp;
        nSteppers = n;
        return true;
    }
    return false;
}

uint16_t Hundred_Steppers::getStepperNum(void)
{
    return nSteppers;
}

void Hundred_Steppers::doStep(uint16_t maxChanged)
{
    // If only 0~9 stepper will move and clear pin is offered,
    // then you can just set length to 9,
    // data for steppers after 10 will not be transferred out,
    // this can save some time.
    maxChanged = min(maxChanged, nSteppers);
    // If clear pin is not available, mannually clear shift
    // storage  registers by shifting zeros.
    if (!clearShiftStorage())
    {
        for (uint16_t i = 0; i < (nSteppers - maxChanged); i++)
            fastShiftOut(0);
    }
    int index;

    //==========================================================================
    // the further stepper is, the earlier be set
    for (uint16_t i = 0; i <= maxChanged; i++)
    {
        index = stepTable[maxChanged - i] % driveMode;
        if (index < 0) { index += driveMode; }
        fastShiftOut( ~cmdList[index] );
    }
    //==========================================================================

    // control speed
    while ((micros() - lastTime) < speedDelay) {;}
    lastTime = micros();
    *latchReg |= latchMask; // storage data at raising edge --> run steppers
    delayMicroseconds(1);
    *latchReg &= ~latchMask; // data is already registed, pull down
}


void Hundred_Steppers::fastShiftOut(uint8_t value)
{
    // LSB mode
    for (uint8_t i = 0; i < nStepperLines; i++)
    {
        // clock pin set LOW
        *clockReg &= ~clockMask;

        // write data on data pin
        if ( value & (1 << i) )
            *dataReg |= dataMask;
        else
            *dataReg &= ~dataMask;

        // clock pin set HIGH
        *clockReg |= clockMask;
    }
}

uint16_t Hundred_Steppers::stepsToMove(int * array, uint16_t length)
{
    // summary of values in an array --> sum(abs(array))
    uint16_t tmp = 0;
    for (uint16_t i=0; i < length; i++)
    {
        tmp += abs(array[i]);
    }
    return tmp;
}
