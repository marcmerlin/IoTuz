/* License: Apache-2  */

// Inspired from fbonan
// https://www.circuitsathome.com/mcu/reading-rotary-encoder-on-arduino/

// Another example on how to do this:
// https://github.com/prampec/arduino-softtimer/blob/master/examples/SoftTimer6Rotary/SoftTimer6Rotary.ino

#include <IoTuz.h>
IoTuz iotuz = IoTuz();

void aikotest() {
    static bool state = false;
    if (state) {
	pixels.setPixelColor(0, 255, 10, 10);
    } else {
	pixels.setPixelColor(0, 10, 255, 10);
    } 
    pixels.show();
    state = !state;
}

void aikotest2() {
    static bool state = false;
    if (state) {
	pixels.setPixelColor(1, 10, 10, 255);
    } else {
	pixels.setPixelColor(1, 10, 255, 10);
    } 
    pixels.show();
    state = !state;
}

// the setup routine runs once when you press reset:
void setup() {
    iotuz.begin();
    // backlight is off by default, turn it on.
    //iotuz.screen_bl(true);

    Events.addHandler(aikotest, 1000);
    Events.addHandler(aikotest2, 500);
    iotuz.enable_aiko_ISR();
}

// the loop routine runs over and over again forever:
void loop() {
    Serial.println("Main loop is busy and not able to call Aiko's event loop often enough");
    delay(10000);
    // This will flush the Aiko event queue  and stop further processing.
    Events.reset();
}

// vim:sts=4:sw=4
