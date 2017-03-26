/* 
 * Demo code for digital RGB LEDs using the RMT peripheral on the ESP32
 *
 * Modifications Copyright (c) 2017 Martin F. Falatic
 *
 * Based on public domain code created 19 Nov 2016 by Chris Osborn <fozztexx@fozztexx.com>
 * http://insentricity.com
 *
 * The RMT peripheral on the ESP32 provides very accurate timing of
 * signals sent to the WS2812 LEDs.
 *
 */
/* 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

//#include "ws2812.h"
#include <Wire.h>
#include <IoTuz.h>
IoTuz iotuz = IoTuz();

#include "tasmanian-devil.c"



uint8_t MAX_COLOR_VAL = 32; // Limits brightness

int pausetime = 500;

void displayOff();
void rainbow(unsigned long, unsigned long);
void scanner(unsigned long, unsigned long);
void dumpDebugBuffer(int, char *);

void dumpDebugBuffer(int id, char * debugBuffer) {
  Serial.print("DEBUG: (");
  Serial.print(id);
  Serial.print(") ");
  Serial.println(debugBuffer);
  debugBuffer[0] = 0;
}

void setup() {
    iotuz.begin();
    // backlight is off by default, turn it on.
    iotuz.screen_bl(true);


    // Tri-color APA106 LEDs Setup
    // Mapping is actually Green, Red, Blue (not RGB)
    //pixels.setPixelColor(0, 32, 0, 0);
    //pixels.setPixelColor(1, 0, 0, 32);
    //pixels.show();
    pixels[0] = makeRGBVal(32, 0, 0);
    pixels[1] = makeRGBVal(0, 0, 32);
    ws2812_setColors(NUMPIXELS, pixels);

    tft.drawBitmap(0, 0, 320, 240, (uint16_t*)picture3);
    tft.setTextColor(ILI9341_BLACK);
    iotuz.tftprint(1, 1, 0, "Click joystick or rotary encoder");
    tft.setTextColor(ILI9341_WHITE);
    for (uint8_t i=0; i<50; i++) {
	delay(100);
	if (iotuz.read_encoder_button() == ENC_RELEASED || !digitalRead(JOYSTICK_BUT_PIN)) break;
    }
}

int passes = 0;
int MAX_PASSES = 10;

void loop() {
  static uint8_t color=0;
  //rainbow(0, 5000);
  //scanner(0, 5000);
  //displayOff();
  pixels[0] = makeRGBVal(255-color, 0, 0);
  pixels[1] = makeRGBVal(0, 0, color++);
  ws2812_setColors(2, pixels);
  delay(5);
}

void loop_FOR_DEBUG_TESTING() {
  for(uint16_t i=0; i<NUMPIXELS; i++) {
    pixels[i] = makeRGBVal(1, 1, 1);
  }
  pixels[0] = makeRGBVal(2, 1, 3);
  pixels[1] = makeRGBVal(5, 4, 6);
  pixels[2] = makeRGBVal(8, 7, 9);
  ws2812_setColors(2, pixels);
  //ws2812_setColors(NUMPIXELS, pixels);
  #if DEBUG_WS2812_DRIVER
    dumpDebugBuffer(passes, ws2812_debugBuffer);
  #endif
  delay(1);
  if (++passes >= MAX_PASSES) {
    while(1) {}
  }
}

void displayOff() {
  for (int i = 0; i < NUMPIXELS; i++) {
    pixels[i] = makeRGBVal(0, 0, 0);
  }
  ws2812_setColors(2, pixels);
  //ws2812_setColors(NUMPIXELS, pixels);
}

void scanner(unsigned long delay_ms, unsigned long timeout_ms) {
  int currIdx = 0;
  int prevIxd = 0;
  bool RUN_FOREVER = (timeout_ms == 0 ? true : false);
  unsigned long start_ms = millis();
  while (RUN_FOREVER || (millis() - start_ms < timeout_ms)) {
    pixels[prevIxd] = makeRGBVal(0, 0, 0);
    pixels[currIdx] = makeRGBVal(MAX_COLOR_VAL, MAX_COLOR_VAL, MAX_COLOR_VAL);;
    ws2812_setColors(NUMPIXELS, pixels);
    prevIxd = currIdx;
    currIdx++;
    if (currIdx >= NUMPIXELS) {
      currIdx = 0;
    }
    delay(delay_ms);
  }
}

void rainbow(unsigned long delay_ms, unsigned long timeout_ms)
{
  const uint8_t color_div = 4;
  const uint8_t anim_step = 1;
  const uint8_t anim_max = MAX_COLOR_VAL - anim_step;
  rgbVal color = makeRGBVal(anim_max, 0, 0);
  rgbVal color2 = makeRGBVal(anim_max, 0, 0);
  uint8_t stepVal = 0;
  uint8_t stepVal2 = 0;

  bool RUN_FOREVER = (timeout_ms == 0 ? true : false);
  unsigned long start_ms = millis();
  while (RUN_FOREVER || (millis() - start_ms < timeout_ms)) {
    color = color2;
    stepVal = stepVal2;
  
    for (uint16_t i = 0; i < NUMPIXELS; i++) {
      pixels[i] = makeRGBVal(color.r/color_div, color.g/color_div, color.b/color_div);
  
      if (i == 1) {
        color2 = color;
        stepVal2 = stepVal;
      }
  
      switch (stepVal) {
        case 0:
        color.g += anim_step;
        if (color.g >= anim_max)
          stepVal++;
        break;
        case 1:
        color.r -= anim_step;
        if (color.r == 0)
          stepVal++;
        break;
        case 2:
        color.b += anim_step;
        if (color.b >= anim_max)
          stepVal++;
        break;
        case 3:
        color.g -= anim_step;
        if (color.g == 0)
          stepVal++;
        break;
        case 4:
        color.r += anim_step;
        if (color.r >= anim_max)
          stepVal++;
        break;
        case 5:
        color.b -= anim_step;
        if (color.b == 0)
          stepVal = 0;
        break;
      }
    }
  
    ws2812_setColors(NUMPIXELS, pixels);
  
    delay(delay_ms);
  }
}
