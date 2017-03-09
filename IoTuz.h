#ifndef IOTUZ_H
#define IOTUZ_H

#include <Wire.h>
#include <SPI.h>

#include <AikoEvents.h>
using namespace Aiko;

#include <Adafruit_GFX.h>
// Support for LCD screen
// The latest version of that library may not be up to date and miss a patch for ESP32
// which will cause a compilation error:
// Adafruit_ILI9341.cpp:113:3: error: 'mosiport' was not declared in this scope
// If so, get the latest version from github, or just patch this single line
// https://github.com/adafruit/Adafruit_ILI9341/blob/master/Adafruit_ILI9341.cpp#L98
#include <Adafruit_ILI9341.h>
// faster, better lib, that doesn't work yet.
//#include <ILI9341_t3.h>

// https://learn.adafruit.com/adafruit-neopixel-uberguide/
// Support for APA106 RGB LEDs
// APA106 is somewhat compatible with WS2811 or WS2812 (well, maybe not 100%, but close enough to work)
// I have patched the Adafruit library to support ESP32. If that hasn't been merged yet, see this patch
// https://github.com/adafruit/Adafruit_NeoPixel/pull/125
// If you do NOT apply my patch, the LEDS WILL NOT WORK
#include "Adafruit_NeoPixel.h"

// Accelerometer
#include <Adafruit_Sensor.h>
#include <Adafruit_ADXL345_U.h>

// Touch screen https://github.com/PaulStoffregen/XPT2046_Touchscreen
#include <XPT2046_Touchscreen.h>

// https://github.com/CCHS-Melbourne/iotuz-esp32-hardware/wiki has hardware mapping details
/*
I2C addresses:
Audio:  0x1A
IO_EXP: 0x20 => Spec sheet says 
"To enter the Read mode the master (microcontroller) addresses the slave
device and sets the last bit of the address byte to logic 1 (address byte read)"
The Wire lib automatically uses the right I/O port for read and write.
ADXL:   0x53
BME230: 0x77 (Temp/Humidity/Pressure)
*/



// TFT + Touch Screen Setup Start
// These are the minimal changes from v0.1 to get the LCD working
#define TFT_DC 4
#define TFT_CS 19
#define TFT_RST 32
// SPI Pins are shared by TFT, touch screen, SD Card
#define SPI_MISO 12
#define SPI_MOSI 13
#define SPI_CLK 14

// APA106 LEDs
#define RGB_LED_PIN 23
#define NUMPIXELS 2

// LCD brightness control and touchscreen CS are behind the port
// expander, as well as both push buttons
#define I2C_EXPANDER 0x20	//0100000 (7bit) address of the IO expander on i2c bus

/* Port expander PCF8574, access via I2C on */
#define I2CEXP_ACCEL_INT    0x01	// (In)
#define I2CEXP_A_BUT	    0x02	// (In)
#define I2CEXP_B_BUT	    0x04	// (In)
#define I2CEXP_ENC_BUT	    0x08	// (In)
#define I2CEXP_SD_CS	    0x10	// (Out)
#define I2CEXP_TOUCH_INT    0x20	// (In)
#define I2CEXP_TOUCH_CS	    0x40	// (Out)
#define I2CEXP_LCD_BL_CTR   0x80	// (Out)

// Dealing with the I/O expander is a bit counter intuitive. There is no difference between a
// write meant to toggle an output port, and a write designed to turn off pull down resistors and trigger
// a read request.
// The write just before the read should have bits high on the bits you'd like to read back, but you
// may get high bits back on other bits you didn't turn off the pull down resistor on. This is normal.
// Just filter out the bits you're trying to get from the value read back and keep in mind that when
// you write, you should still send the right bit values for the output pins.
// This is all stored in i2cexp which we initialize to the bits used as input:
#define I2CEXP_IMASK ( I2CEXP_ACCEL_INT + I2CEXP_A_BUT + I2CEXP_B_BUT + I2CEXP_ENC_BUT + I2CEXP_TOUCH_INT )

// This is calibration data for the raw touch data to the screen coordinates
#define TS_MINX 320
#define TS_MINY 220
#define TS_MAXX 3920
#define TS_MAXY 3820
#define MINPRESSURE 400
#define MAXPRESSURE 3000

// Joystick Setup
#define JOYSTICK_X_PIN 39
#define JOYSTICK_Y_PIN 34
#define JOYSTICK_BUT_PIN 0

// Center on my IoTuz board (not used anymore)
#define JOYSTICK_CENTERX  1785
#define JOYSTICK_CENTERY  1854

#define ENCODERA_PIN 15
#define ENCODERB_PIN 36

// Touch screen select is on port expander line 6, not directly connected, so the library
// cannot toggle it directly. It however requires a CS pin, so I'm giving it 33, a spare IO
// pin so that it doesn't break anything else.
// CS is then toggled manually before talking to the touch screen.
#define TS_CS_PIN  33

extern Adafruit_ILI9341 tft;
extern Adafruit_NeoPixel pixels; 
extern Adafruit_ADXL345_Unified accel;
extern XPT2046_Touchscreen ts;

typedef enum {
    ENC_DOWN = 0,
    ENC_PUSHED = 1,
    ENC_UP = 2,
    ENC_RELEASED = 3,

} ButtState;

class IoTuz {
  public:
    IoTuz();

    // tft_width, tft_height, calculated in setup after tft init
    uint16_t tftw, tfth;

    // Buffer to store strings going to be printed on tft
    char tft_str[64];

    void enable_aiko_ISR();
    void i2cexp_clear_bits(uint8_t);
    void i2cexp_set_bits(uint8_t);
    uint8_t i2cexp_read();
    int16_t read_encoder();
    bool encoder_changed();
    ButtState read_encoder_button();
    void screen_bl(bool);
    void reset_tft();
    void tftprint(uint16_t, uint16_t, uint8_t, char *);
    TS_Point get_touch();
    void touchcoord2pixelcoord(uint16_t *, uint16_t *, uint16_t);
    void begin();

  private:
    uint8_t _i2cexp;
    void pcf8574_write_(uint8_t);
};



#endif

// vim:sts=4:sw=4
