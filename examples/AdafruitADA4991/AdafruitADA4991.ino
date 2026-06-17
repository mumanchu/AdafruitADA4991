/////////////////////////////////////////////////////////////////////
// Example sketch for Adafruit ADA4991 I2C Rotary Encoder QT Module
// 

#include <Wire.h>

#define LOGERROR(s) { Serial.println(s); Serial.flush(); }

#include "AdafruitADA4991.h"
AdafruitADA4991 ada4991;


void setup() 
{
	Serial.setTx(PC_10);
	Serial.setRx(PC_11);

	Serial.begin(115200);
	delay(3000);
	Serial.println("\n\rStarted...\n\r");
	Serial.flush();

	pinMode(LED_BUILTIN, OUTPUT);

	Wire.begin();
	Wire.setClock(100000);
	Wire.setTimeout(100);

	if (!ada4991.begin(&Wire, 0x36, D8)) {
		LOGERROR("ada4991.begin() failed");
		while (1)
			yield();
	}
}

void loop() 
{
	// scheduler
	ulong t = millis();

	// poll the encoder every 50ms
	static ulong t1 = 0;
	if (t - t1 >= 50) {
		t1 = t;

		// toggle the led so we know it's running
		digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));

		// if there's been an encoder event
		if (ada4991.hasInterrupt()) {

			// get the encoder position
			static long lastPos = __LONG_MAX__;
			long pos = ada4991.getEncoderPosition();
			if (pos != lastPos) {
				lastPos = pos;
				Serial.println(pos);
			}

			// was the encoder button pressed?
			if (ada4991.encoderButtonPressed())
				Serial.println("pushbutton pressed");
			Serial.flush();
		}
	}

	// change the neopixel color every 500ms, R->G->B
	static ulong t2 = 0;
	if (t - t2 >= 500) {
		t2 = t;
		static ulong rgb = 0x000f0000;
		ada4991.setNeopixelColor(rgb);
		rgb >>= 8;
		if (rgb == 0)
			rgb = 0x000f0000;
	}
}
