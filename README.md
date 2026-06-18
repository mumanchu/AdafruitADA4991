# AdafruitADA4991

The ADA4991 is an I2C module with a rotary encoder and an RGB LED. It has an onboard microprocessor to do most of the work. This removes all the complexity of interrupt handling for the encoder, and nanosecond timing for the RGB LED.

<img src="https://github.com/mumanchu/mumanchu/blob/main/assets/ada4991/ada4991.jpg" alt="Picture of ADA4991 board" width="300">

This board is part of Adafruit's "seesaw" framework which comprises a set of boards with I2C interfaces and a comprehensive seesaw library for programming them. It has Stemma QT connectors (for chaining I2C devices), but you don't need to use those because it also has a 2.54mm pin header. It will run on 3.3V or 5V, with an onboard 3.3V regulator so 5V can be used.

I wanted to use this board _without_ the seesaw library, so this small-footprint AdafruitADA4991 library `Adafruit4991.h` was developed. It handles the rotary encoder and RGB LED in the same class.

To use it, see the example sketch.

Poll the encoder with `hasInterrupt()` from `loop()` to check for events, then read the position and button states. It does not use actual interrupts because you cannot call the I2C methods from an interrupt handler. The interrupt state is not cleared until the position and button states are read. The red LED on the borad turns on when the INT pin is active (low).

This code was developed and tested on an STM32. It may need modifications for other MCUs, further tests will be done later.

> [!NOTE]
> The Quad Rotary Encoder version is not fully supported yet, but the code will be updated when I get one...

## Class Reference

Read the commented source code for more details.

```cpp
class AdafruitADA4991
{
public:
	// Convert 3 separate 8-bit RGB values to a single 24-bit RGB value
	#define RGB(r, g, b) (ulong)((ulong)(byte)r << 16) + ((ulong)(byte)g << 8) + (byte)b)

	bool begin(TwoWire* twoWire, byte i2cAddress, byte interruptPin = 255);
	bool hasInterrupt();
	bool setNeopixelColor(ulong rgb);
	bool encoderButtonPressed();

	//>>> Adafruit Seesaw compatible methods
	// note: the 'encoder' value (0..3) is only for the quad rotary encoder
	long getEncoderPosition(byte encoder = 0);
	long getEncoderDelta(byte encoder = 0);
	bool enableEncoderInterrupt(byte encoder = 0);
	bool disableEncoderInterrupt(byte encoder = 0);
	bool setEncoderPosition(long pos, byte encoder = 0);
	//<<<
};
```
## Useful Links

**Adafruit Wiki** \
https://learn.adafruit.com/adafruit-i2c-qt-rotary-encoder

**Adafruit seesaw Reference** \
https://learn.adafruit.com/adafruit-seesaw-atsamd09-breakout/reading-and-writing-data

<br/>

## Revision History

| Date       | Version  | Details |
|:---------- |:---------|:----------- |
| 2026.06.17 | 0.0.0	| Preliminary |

<br/>

## Joke of the Week

_Of course I'm in a BAD MOOD! I haven't had a cigarette for 20 years._




