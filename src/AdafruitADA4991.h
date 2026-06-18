#pragma once

/////////////////////////////////////////////////////////////////////
// Adafruit ADA4991 I2C Rotary Encoder QT Module
// with 'neopixel' RGB LED
// Copyright (C) mumanchu, muman.ch & Adafruit, 2026.06.16
// All Rights Reversed Inversely
/*
Instead of using the full Adafruit Seesaw package, this code 
supports only the ADA4991 module with a rotary encoder, encoder 
pushbutton and an RGB led. The code has been optimized for only
this module. It can also be used with the quad rotary encoder.

https://www.adafruit.com/product/4991
https://learn.adafruit.com/adafruit-i2c-qt-rotary-encoder

Poll the encoder with hasInterrupt(), then read the position and
button states. It does not use actual interrupts - you cannot call
I2C methods from an interrupt handler. The interrupt state is not 
cleared until the position and button states are read. The red 
LED turns on when the INT pin is active (low).

Where possible, the methods return 'false' on a communications
or other error, so the program won't continue to run blindy if 
there's a hardware or software problem. But the Adafruit Seesaw 
compatible methods can't do that. Check the source code below 
for details.

SCHEMATIC
https://learn.adafruit.com/assets/102081

ADAFRUIT SEESAW REFERENCES
https://learn.adafruit.com/adafruit-seesaw-atsamd09-breakout/reading-and-writing-data
https://github.com/adafruit/Adafruit_Seesaw/tree/master

I2C ADDRESS
Set by A0 A1 A2 jumpers
0x36..0x3d
https://learn.adafruit.com/adafruit-i2c-qt-rotary-encoder/pinouts

ROTARY ENCODER
https://learn.adafruit.com/adafruit-seesaw-atsamd09-breakout/encoder
Base register	0x11

Register	Name				Size		R/W
0x10		Interrupt Enable	1 byte		W
0x20		Interrupt Disable	1 byte		W
0x30		Position			4 bytes		R/W
0x40		Delta				4 bytes		R
Lower 4 bits are the encoder number

NEOPIXEL
https://learn.adafruit.com/adafruit-seesaw-atsamd09-breakout/neopixel
Base register	0x0E

Register	Name				Size
0x01		PIN					8 bits		Pin number PORTA 
0x02		SPEED				8 bits		0=400kHz, 1=800kHz (default) 
0x03		BUF_LENGTH			16 bits		No. of bytes for pixel array
0x04		BUF					32 bytes	1st 2 bytes are start address + data bytes 
0x05		SHOW				none		Causes output to update

RGB LED					Seesaw Pin 6
Encode Push Button		Seesaw Pin 24

QUAD ROTARY ENCODER
The quad rotary encoder version is not fully supported.
The 'encoder' parameter (0..3) is only for the quad rotary encoder 
https://learn.adafruit.com/adafruit-i2c-quad-rotary-encoder-breakout
*/


class AdafruitADA4991
{
	TwoWire* wire;
	byte i2cAdds;
	byte intPin;
	bool lastEncoderButtonState;

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

protected:
	bool beginNeopixel();
	bool adaPinMode(byte pin, byte mode);
	bool adaSetGPIOInterrupt(byte pin, bool enabled);
	bool adaDigitalRead(byte pin);

	bool read(byte adds, byte reg, byte* buf, byte len, uint delayMicros = 250);
	bool read32(byte base, byte reg, ulong* data, uint delayMicros = 250);
	bool write(byte adds, byte reg, byte* buf, byte len);
	ulong swap32(byte* buf);
};


// i2cAddress   = 0x36 .. 0x3d
// interruptPin = the INT output pin from the module,
//                poll this with hasInterrupt()
bool AdafruitADA4991::begin(TwoWire* twoWire, byte i2cAddress, 
	byte interruptPin /*=255*/)
{
	wire = twoWire;
	i2cAdds = i2cAddress;
	intPin = interruptPin;
	lastEncoderButtonState = false;

	// the interrupt pin does not generate an interrupt,
	// it is polled by calling hasInterrupt()
	if (interruptPin != 255)
		pinMode(interruptPin, INPUT_PULLUP);

	ulong version;
	if (!read32(0x00, 0x02, &version))
		return false;
	if (version == 0 || version == 0xffffffff) {
		LOGERROR("ADA4991 not responding")
		return false;
	}
	if ((version >> 16) != 4991) {
		LOGERROR("not an ADA4991")
		return false;
	}

	// configure neopixel
	if (!beginNeopixel())
		return false;

	// configure encoder push button input
	adaPinMode(24, INPUT_PULLUP);
	if (interruptPin != 255) {
		adaSetGPIOInterrupt(24, true);
		if (!enableEncoderInterrupt())
			return false;
	}
	return setEncoderPosition(0);
}

// Poll for a rotary encoder event, encoder turned or push button pressed
bool AdafruitADA4991::hasInterrupt()
{
	if (intPin == 255)
		return false;
	// INT is active low
	return digitalRead(intPin) == 0;
}

// https://learn.adafruit.com/adafruit-seesaw-atsamd09-breakout/neopixel
bool AdafruitADA4991::setNeopixelColor(ulong rgb)
{
	byte buf[5];
	// buffer address 0
	buf[0] = 0;
	buf[1] = 0;
	// GRB data
	byte* p = (byte*)&rgb;
	buf[2] = p[1];		// G
	buf[3] = p[2];		// R
	buf[4] = p[0];		// B
	if (!write(0x0E, 0x04, buf, 5))
		return false;
	// show
	return write(0x0E, 0x05, NULL, 0);
}

// Returns 'true' if the encoder button has been pressed
// it returns 'true' only once, then the pushbutton must be
// released and pressed before it will return 'true' again
bool AdafruitADA4991::encoderButtonPressed()
{
	bool currentState = !adaDigitalRead(24);

	if (lastEncoderButtonState == currentState)
		return false;
	if (!lastEncoderButtonState && currentState) {
		lastEncoderButtonState = currentState;
		return true;
	}
	lastEncoderButtonState = currentState;
	return false;
}

long AdafruitADA4991::getEncoderPosition(byte encoder)
{
	ulong data;
	read32(0x11, 0x30 + encoder, &data);
	return (long)data;
}

long AdafruitADA4991::getEncoderDelta(byte encoder)
{
	ulong data;
	read32(0x11, 0x40 + encoder, &data);
	return (long)data;
}

bool AdafruitADA4991::enableEncoderInterrupt(byte encoder)
{
	byte b = 0x01;
	return write(0x11, 0x10 + encoder, &b, 1);
}

bool AdafruitADA4991::disableEncoderInterrupt(byte encoder)
{
	byte b = 0x01;
	return write(0x11, 0x20 + encoder, &b, 1);
}

bool AdafruitADA4991::setEncoderPosition(long pos, byte encoder)
{
	long posr = swap32((byte*)&pos);
	return write(0x11, 0x30 + encoder, (byte*)&posr, 4);
}

// Internal Methods

// The ADA4991 has one RGB LED
bool AdafruitADA4991::beginNeopixel()
{
	// set neopixel pin 6
	byte b = 6;
	if (!write(0x0E, 0x01, &b, 1))
		return false;

	// set buffer length to 3 bytes (24-bit GRB)
	byte buf[2] = { 0, 3 };
	if (!write(0x0E, 0x03, buf, 2))
		return false;

	// (speed is already 800KHz by default)
	return true;
}

//https://learn.adafruit.com/adafruit-seesaw-atsamd09-breakout/gpio
//https://github.com/adafruit/Adafruit_Seesaw/blob/985b41efae3d9a8cba12a7b4d9ff0d226f9e0759/Adafruit_seesaw.cpp#L206

bool AdafruitADA4991::adaPinMode(byte pin, byte mode)
{
	ulong pinMask = 1L << pin;
	ulong pinMaskr = swap32((byte*)&pinMask);
	byte* p = (byte*)&pinMaskr;

	switch (mode) {
	// we only need this one, for now
	case INPUT_PULLUP:
		// 0x03=DIRCLR, 0x0B=PULLENSET, 0x05=SET 
		if (write(0x01, 0x03, p, 4) &&
			write(0x01, 0x0B, p, 4) &&
			write(0x01, 0x05, p, 4))
			return true;
		break;
	/*these are not used for the ADA4991
	case OUTPUT:
		// 0x02=DIRSET
		write(0x01, 0x02, p, 4);
		break;
	case INPUT:
		// 0x03=DIRCLR
		write(0x01, 0x03, p, 4);
		break;
	case INPUT_PULLDOWN:
		// 0x03=DIRCLR, 0x0B=PULLENSET, 0x06=CLR 
		write(0x01, 0x03, p, 4);
		write(0x01, 0x0B, p, 4);
		write(0x01, 0x06, p, 4);
		break;
	*/
	default:
		LOGERROR("mode not supported");
	}
	return false;
}

bool AdafruitADA4991::adaSetGPIOInterrupt(byte pin, bool enabled)
{
	ulong pinMask = 1L << pin;
	ulong pinMaskr = swap32((byte*)&pinMask);

	// 0x08=INTENSET, 0x09=INTENCLR
	return write(0x01, enabled ? 0x08 : 0x09, (byte*)&pinMaskr, 4);
}

bool AdafruitADA4991::adaDigitalRead(byte pin)
{
	byte buf[4];
	read(0x01, 0x04, buf, 4);
	return swap32(buf) & (1L << pin);
}

// Read bytes
bool AdafruitADA4991::read(byte base, byte reg, byte* buf, byte length, 
	uint delayMicros/*=250*/)
{
	memset(buf, 0, length);		// returns 0s if it fails

	wire->beginTransmission(i2cAdds);
	wire->write(&base, 1);
	wire->write(&reg, 1);
	if (wire->endTransmission() != 0) {
		LOGERROR("endTransmission() failed 1");
		return false;
	}
	if (delayMicros != 0)
		delayMicroseconds(delayMicros);

	wire->requestFrom(i2cAdds, (size_t)length);
	if (wire->readBytes(buf, length) != length) {
		LOGERROR("readBytes() failed");
		return false;
	}
	return true;
}

// Read a 32-bit value
bool AdafruitADA4991::read32(byte base, byte reg, ulong* data, 
	uint delayMicros/*=250*/)
{
	byte buf[4];
	if (!read(base, reg, buf, 4, delayMicros)) {
		*data = 0;
		return false;
	}
	*data = swap32(buf);
	return true;
}

// Write bytes
bool AdafruitADA4991::write(byte adds, byte reg, byte* buf, byte len)
{
	wire->beginTransmission(i2cAdds);
	wire->write(&adds, 1);
	wire->write(&reg, 1);
	if (buf != NULL && len != 0)
		wire->write(buf, len);
	if (wire->endTransmission() != 0) {
		LOGERROR("endTransmission() failed 2");
		return false;
	}
	return true;
}

// Reverse the 4 bytes of a 32-bit value
// (little to big-endian conversion, or is it big to little-startian?)
ulong AdafruitADA4991::swap32(byte* buf)
{
	return ((ulong)buf[0] << 24) + ((ulong)buf[1] << 16) + ((ulong)buf[2] << 8) + buf[3];
}

