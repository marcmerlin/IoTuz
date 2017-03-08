/* License: Apache-2  */

// Inspired from fbonan
// https://www.circuitsathome.com/mcu/reading-rotary-encoder-on-arduino/

// Another example on how to do this:
// https://github.com/prampec/arduino-softtimer/blob/master/examples/SoftTimer6Rotary/SoftTimer6Rotary.ino

#include <IoTuz.h>
IoTuz iotuz = IoTuz();

void aikotest() {
    Serial.println("Aiko Callback every second");
}

void aikotest2() {
    Serial.println("Aiko Callback every 0.1s");
}

// the setup routine runs once when you press reset:
void setup() {
    iotuz.begin();
    iotuz.reset_tft();
    // backlight is off by default, turn it on.
    iotuz.screen_bl(true);

    Events.addHandler(aikotest, 1000);
    Events.addHandler(aikotest2, 100);
}

// the loop routine runs over and over again forever:
void loop() {
    Events.loop();
}

// vim:sts=4:sw=4
