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
        uint8_t stepper_line_num, uint8_t driver_mode,
        uint8_t clearPin, uint8_t enablePin)
{
    // steppers num
    this->nSteppers = nSteppers;

    // init steppers' step table full of 0
    // stepType: this is defined in head file Hundred_Steppers.h
    this->step_table = (stepType *)calloc(nSteppers, sizeof(stepType));

    // init pins directions.
    pinMode(dataPin, OUTPUT);
    pinMode(clockPin, OUTPUT);
    pinMode(latchPin, OUTPUT);
    pinMode(clearPin, OUTPUT);
    pinMode(enablePin, OUTPUT);

    // get pin registers pointer, these GPIO are frequently used.
    this->dataReg   = portOutputRegister(digitalPinToPort(dataPin));
    this->clockReg  = portOutputRegister(digitalPinToPort(clockPin));
    this->latchReg  = portOutputRegister(digitalPinToPort(latchPin));
    this->dataMask  = digitalPinToBitMask(dataPin);
    this->clockMask = digitalPinToBitMask(clockPin);
    this->latchMask = digitalPinToBitMask(latchPin);
    *dataReg &= ~dataMask;
    *clockReg &= ~clockMask;
    *latchReg &= ~latchMask;

    // not necessary pins, also not frequently used.
    this->clearPin = clearPin;
    this->enablePin = enablePin;
    digitalWrite(clearPin, HIGH);
    digitalWrite(enablePin, LOW);

    this->last_step_time = micros();
    this->stepper_line_num = stepper_line_num;
    this->nSteps = nSteps;

    // driver mode
    this->driver_mode = driver_mode;
    if (driver_mode == 4)
    {
        this->cmd_list = new uint8_t[4] { B1110, B1101, B1011, B0111 };
    }
    else if (driver_mode == 8)
    {
        this->cmd_list = new uint8_t[8] { B1110, B1100, B1101, B1001,
                                          B1011, B0011, B0111, B0110 };
    }

    // default speed 60r/min
    setSpeedRevPerMin(60);
}

Hundred_Steppers::Hundred_Steppers(
        uint16_t nSteppers, uint16_t nSteps,
        uint8_t dataPin, uint8_t clockPin, uint8_t latchPin,
        uint8_t stepper_line_num, uint8_t driver_mode)
{
    this->nSteppers = nSteppers;
    this->step_table = (stepType *)calloc(nSteppers, sizeof(stepType));
    pinMode(dataPin, OUTPUT);
    pinMode(clockPin, OUTPUT);
    pinMode(latchPin, OUTPUT);
    this->dataReg   = portOutputRegister(digitalPinToPort(dataPin));
    this->clockReg  = portOutputRegister(digitalPinToPort(clockPin));
    this->latchReg  = portOutputRegister(digitalPinToPort(latchPin));
    this->dataMask  = digitalPinToBitMask(dataPin);
    this->clockMask = digitalPinToBitMask(clockPin);
    this->latchMask = digitalPinToBitMask(latchPin);
    *dataReg &= ~dataMask;
    *clockReg &= ~clockMask;
    *latchReg &= ~latchMask;
    this->clearPin = 0;
    this->enablePin = 0;
    this->last_step_time = micros();
    this->stepper_line_num = stepper_line_num;
    this->nSteps = nSteps;
    this->driver_mode = driver_mode;
    if (driver_mode == 4)
    { this->cmd_list = new uint8_t[4] { B1110, B1101, B1011, B0111 }; }
    else if (driver_mode == 8)
    { this->cmd_list = new uint8_t[8] { B1110, B1100, B1101, B1001,
                                        B1011, B0011, B0111, B0110 }; }
    setSpeedRevPerMin(60);
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

bool Hundred_Steppers::clearSteppers(void)
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
    // step_delay_second * (steps * rev) = 1min
    // step_delay * steps * rev = 60000000 us
    step_delay = 60L * 1000L * 1000L / nSteps / n;
}

void Hundred_Steppers::setSpeedRadPerSec(uint16_t n)
{
    // step_delay_second * steps * rad/2Pi = 1s
    step_delay = TWO_PI * 1000L * 1000L / nSteps / n;
}

void Hundred_Steppers::setStepperStep(uint16_t n, int steps_to_move)
{
    // control single stepper with its index and steps to move
    if (n < nSteppers)
    {
        while (steps_to_move != 0)
        {
            if (steps_to_move > 0)
            {
                steps_to_move--;
                step_table[n]++;
            }
            else
            {
                steps_to_move++;
                step_table[n]--;
            }
            step(n + 1);
        }
    }
}

void Hundred_Steppers::setStepperStep(int * stepList, uint16_t length)
{
    // control a list of steppers
    uint16_t maxChanged;
    if (length > nSteppers) length = nSteppers;

    // loop until every steps_to_move return to zero
    // e.g. stepList finally become {0, 0, 0...}
    while (stepsToMove(stepList, length))
    {
        maxChanged = -1;
        for (uint16_t i = 0; i < length; i++)
        {
            if (stepList[i] > 0)
            {
                stepList[i]--;
                step_table[i]++;
                maxChanged = i;
            }
            else if (stepList[i] < 0)
            {
                stepList[i]++;
                step_table[i]--;
                maxChanged = i;
            }
        }
        if (maxChanged == -1) { break; }
        step(maxChanged);
    }
}

void Hundred_Steppers::home(void)
{
    // move each steppers to zero position
    uint16_t maxChanged;
    while (true)
    {
        maxChanged = -1;
        for (uint16_t i = 0; i < nSteppers; i++)
        {
            if (step_table[i] > 0)
            {
                step_table[i]--;
                maxChanged = i;
            }
            else if (step_table[i] < 0)
            {
                step_table[i]++;
                maxChanged = i;
            }
        }
        if (maxChanged == -1) {break;}
        step(maxChanged);
    }
}

bool Hundred_Steppers::setStepperNum(uint16_t n)
{
    /*
    uint8_t * tmp = (uint8_t *)realloc(step_table, n * sizeof(uint8_t));
    if (tmp)
    {
        step_table = tmp;
        memset(step_table, 0, n*sizeof(uint8_t));
        return true;
    }
    free(step_table);
    return false;
    */

    stepType * tmp = (stepType *)calloc(n, sizeof(stepType));
    if (tmp)
    {
        free(step_table);
        step_table = tmp;
        nSteppers = n;
        return true;
    }
    return false;
}

uint16_t Hundred_Steppers::getStepperNum(void)
{
    return nSteppers;
}

void Hundred_Steppers::step(uint16_t length)
{
    // If only 0~9 stepper will move, then set length to 10,
    // data for steppers after 10 will not be transferred out,
    // this can save some time.
    if (length == -1 || length > nSteppers)
        length = nSteppers;

    // control speed
    while ((micros() - last_step_time) < step_delay) {;}
    last_step_time = micros();

    //==========================================================================
    // the further stepper is, the earlier be set
    for (uint16_t i = 0; i < length; i++)
    {
        fastShiftOut( ~cmd_list[step_table[length - i - 1] % driver_mode] );
    }
    //==========================================================================

    *latchReg |= latchMask; // storage data at raising edge --> run steppers
    delayMicroseconds(1);
    *latchReg &= ~latchMask; // data is already registed, pull down
}


void Hundred_Steppers::fastShiftOut(uint8_t value)
{
    // LSB mode
    for (uint8_t i = 0; i < stepper_line_num; i++)
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
    uint16_t tmp = 0;
    for (uint16_t i=0; i < length; i++)
    {
        tmp += abs(array[i]);
    }
    return tmp;
}
