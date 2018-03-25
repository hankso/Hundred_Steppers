## Hundred Steppers
An arduino library to control hundreds of steppers with as less pins or
arduinos as possible.

[74HC595](https://www.arduino.cc/en/Tutorial/ShiftOut) is needed to extend the
GPIO number of arduino.

To offer enough current for steppers, you may need UNL2803(8 channel NPN
darlington triodes, 500mA@50V max)

If you have used arduino library
[`Stepper`](https://www.arduino.cc/en/Reference/Stepper) and
[`Adafruit_NeoPixel`](https://github.com/adafruit/Adafruit_NeoPixel) before,
you may find this API familiar because I name these functions similar with them.

## How to use
- Download zip file
- Unzip in your arduino libraries directory. It can be `~/arduino/libraries`
or something like `C:\Program Files\Arduino\libraries`.
- open your arduino IDE and try include library, also there is an example
available.

## Attention
- you need to define how steppers are driver with
`#define driver_mode_4`(A-B-C-D) or `#define driver_mode_8`(A-AB-B-BC-C-CD-D-DA)

- 74HC595 pins
    - ST_CP --> latchPin
    - SH_CP --> clockPin
    - DS --> dataPin
    - OE --> enablePin
    - MR --> clearPin

![74HC595]()

## API
![similar API]()
