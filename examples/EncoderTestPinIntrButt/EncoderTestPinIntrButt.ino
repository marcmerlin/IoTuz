/* License: Apache-2  */

// Inspired from fbonan
// https://www.circuitsathome.com/mcu/reading-rotary-encoder-on-arduino/

// This is better than EncoderTestIntr because it only interrupts on
// pin change instead of some time interval.

#include <IoTuz.h>
IoTuz iotuz = IoTuz();

// the setup routine runs once when you press reset:
void setup() {
    // initialize serial communication at 9600 bits per second:
    iotuz.begin();

    Serial.println("This driver works reliably on IoTuz, but generates 4 rotation clicks");
    Serial.println("per click you can feel in the knob. This is due the the hardware and not a bug");
}

// the loop routine runs over and over again forever:
void loop() {

    if (iotuz.encoder_changed()) {
	Serial.print ("Encoder val: ");
	Serial.println (iotuz.read_encoder());
    }

    // There is no debouncing here, delay at the end will help a bit, but is not foolproof
    switch (iotuz.read_encoder_button()) {
    case ENC_DOWN:
	// This would spam the console too much
	//Serial.println("Encoder Button Down");
	break;
    case ENC_PUSHED:
	Serial.println("Encoder Button Just Pushed");
	break;
    case ENC_UP:
	//Serial.println("Encoder Button Up");
	break;
    case ENC_RELEASED:
	Serial.println("Encoder Button Just Released");
	break;
    }


    //Serial.println("Do other stuff in loop()");
    delay(10);
}

// vim:sts=4:sw=4
