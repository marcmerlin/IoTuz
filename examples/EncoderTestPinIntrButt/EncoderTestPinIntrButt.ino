/* License: Apache-2  */

// Inspired from fbonan
// https://www.circuitsathome.com/mcu/reading-rotary-encoder-on-arduino/

// Another example on how to do this:
// https://github.com/prampec/arduino-softtimer/blob/master/examples/SoftTimer6Rotary/SoftTimer6Rotary.ino

#include <IoTuz.h>
IoTuz iotuz = IoTuz();

// the setup routine runs once when you press reset:
void setup() {
    iotuz.begin();
    iotuz.reset_tft();
    // backlight is off by default, turn it on.
    iotuz.screen_bl(true);

    Serial.println("This driver works reliably on IoTuz, but generates 4 rotation clicks");
    Serial.println("per click you can feel in the knob. This is due the the hardware and not a bug");
    iotuz.tftprint(0, 0, 0, "Encoder: ");
    iotuz.tftprint(0, 1, 0, "Button: ");
}

// the loop routine runs over and over again forever:
void loop() {

    if (iotuz.encoder_changed()) {
	sprintf(iotuz.tft_str, "%d", iotuz.read_encoder());
	Serial.print ("Encoder val: ");
	Serial.println (iotuz.tft_str);
	iotuz.tftprint(9, 0, 5, iotuz.tft_str);
    }

    // There is no debouncing here, delay at the end will help a bit, but is not foolproof
    switch (iotuz.butEnc()) {
    case BUT_DOWN:
	// This would spam the console too much
	//Serial.println("Encoder Button Down");
	iotuz.tftprint(8, 1, 8, "Down");
	break;
    case BUT_PUSHED:
	Serial.println("Encoder Button Just Pushed");
	iotuz.tftprint(8, 1, 8, "Pushed");
	break;
    case BUT_UP:
	//Serial.println("Encoder Button Up");
	iotuz.tftprint(8, 1, 8, "Up");
	break;
    case BUT_RELEASED:
	Serial.println("Encoder Button Just Released");
	iotuz.tftprint(8, 1, 6, "Released");
	break;
    }

    iotuz.read_joystick(true);

    //Serial.println("Do other stuff in loop()");
    delay(100);
}

// vim:sts=4:sw=4
