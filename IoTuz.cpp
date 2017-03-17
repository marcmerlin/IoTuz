/*************************************************** 
  IoTuz class to provide basic driver methods for the ESP32 based IoTuz board
  from LCA 2017: https://github.com/CCHS-Melbourne/iotuz-esp32-hardware

  By Marc MERLIN <marc_soft@merlins.org>

  License: Apache 2.0.

  Required libraries:
  - Wire.h to support the pcf8574 port expander

 ****************************************************/

#include <Wire.h>
#include "IoTuz.h"

volatile int16_t encoder0Pos = 0;

// These need to be global because all sketches use them as a global
// tft adafruit library (games use tft2 a separate library, for which we skip the init
// since the adafruit init works fine for both)
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);
// faster, better lib, that doesn't work yet.
//ILI9341_t3 tft = ILI9341_t3(TFT_CS, TFT_DC, TFT_RST);

// If you are lacking the ESP32 patch, you will get no error, but the LEDs will not work
#ifdef NEOPIXEL
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUMPIXELS, RGB_LED_PIN, NEO_GRB + NEO_KHZ800);
#else
rgbVal pixels[NUMPIXELS];
#endif


// ADXL345
Adafruit_ADXL345_Unified accel = Adafruit_ADXL345_Unified(12345);

// Until further notice, there is a hack to get HW SPI be as fast as SW SPI:
// in espressif/esp32/cores/esp32/esp32-hal.h after the first define, add
// #define CONFIG_DISABLE_HAL_LOCKS 1
// Use with caution, this may cause unknown issues

// Touch screen
XPT2046_Touchscreen ts(TS_CS_PIN);  // Param 2 - NULL - No interrupts)


// If enabled by the user, call every millisecond and run the Aiko 
// event loop.
void onTimer() {
    Events.loop();
}

// Aiko can be run from the sketch's mail loop by simply calling Events.loop();
// but if you want Aiko events to be run from an ISR while the main loop is busy
// doing something else, then you call this once, and it'll setup a timer to call
// your code on interval at a millisecond precision (could be lower if needed)
// 
// Warning, you CANNOT call Serial or other things that take too long,
// take locks, or generate interrupts since you will already be inside
// an interrupt.
// No interrupt may spend more than 300ms, or the interrupt will time out with an
// error like this: Guru Meditation Error: Core 0 panic'ed (Interrupt wdt timeout on CPU1)
void IoTuz::enable_aiko_ISR() {
    // ESP32 timer
    hw_timer_t *timer;
    
    // 3 timers, choose #3, 80 divider nanosecond precision, 1 to count up
    timer = timerBegin(3, 80, 1);
    timerAttachInterrupt(timer, &onTimer, 1);
    // every 1,000ns = 1ms, autoreload = true
    timerAlarmWrite(timer, 1000, true);
    timerAlarmEnable(timer);
}


void read_encoder_ISR() 
{
    static uint8_t old_AB = 0;
    // grey code
    // http://hades.mech.northwestern.edu/index.php/Rotary_Encoder
    // also read up on 'Understanding Quadrature Encoded Signals'
    // https://www.pjrc.com/teensy/td_libs_Encoder.html
    // another interesting lib: https://github.com/0xPIT/encoder/blob/arduino/ClickEncoder.cpp
    static int8_t enc_states[] = {0,-1,1,0,1,0,0,-1,-1,0,0,1,0,1,-1,0};

    old_AB <<= 2;
    old_AB |= ((digitalRead(ENCODERB_PIN))?(1<<1):0) | ((digitalRead(ENCODERA_PIN))?(1<<0):0);
    encoder0Pos += ( enc_states[( old_AB & 0x0f )]);
}

// Talking to the port expander:
// I2C/TWI success (transaction was successful).
#define ku8TWISuccess    0
// I2C/TWI device not present (address sent, NACK received).
#define ku8TWIDeviceNACK 2
// I2C/TWI data not received (data sent, NACK received).
#define ku8TWIDataNACK   3
// I2C/TWI other error.
#define ku8TWIError      4
void IoTuz::pcf8574_write_(uint8_t dt)
{
    uint8_t error;

    Wire.beginTransmission(I2C_EXPANDER);
    // Serial.print("Writing to I2CEXP: ");
    // Serial.println(dt);
    Wire.write(dt);
    error = Wire.endTransmission();
    if (error != ku8TWISuccess) {
	    // FIXME: do something here if you like
    }
}

// To clear bit #7, send 128
void IoTuz::i2cexp_clear_bits(uint8_t bitfield)
{
    // set bits to clear to 0, all other to 1, binary and to clear the bits
    _i2cexp &= (~bitfield);
    pcf8574_write_(_i2cexp);
}

// To set bit #7, send 128
void IoTuz::i2cexp_set_bits(uint8_t bitfield)
{
    _i2cexp |= bitfield;
    pcf8574_write_(_i2cexp);
}

uint8_t IoTuz::i2cexp_read()
{
    // For read to work, we must have sent 1 bits on the ports that get used as input
    // This is done by i2cexp_clear_bits called in setup.
    Wire.requestFrom(I2C_EXPANDER, 1);	// FIXME: deal with returned error here?
    while (Wire.available() < 1) ;
    uint8_t read = ~Wire.read();	// Apparently one has to invert the bits read
    // When no buttons are pushed, this returns 0x91, which includes some ports
    // we use as output, so we do need to filter out the ports used as read.
    // Serial.println(read, HEX);
    return read;
}

int16_t IoTuz::read_encoder() 
{
    return encoder0Pos;
}

bool IoTuz::encoder_changed() {
    static int16_t old_encoder0Pos = 0;
    if (encoder0Pos != old_encoder0Pos)
    {
	old_encoder0Pos = encoder0Pos;
	return true;
    }
    return false;
}

// There is no separate method to return button state changed
// because this one already does it, look for PUSHED and RELEASED
ButtState IoTuz::read_encoder_button() 
{
    static bool butEnc = false;
    uint8_t butt_state = i2cexp_read() & I2CEXP_ENC_BUT;

    if (butt_state && !butEnc)
    {
	butEnc = true;
	//Serial.println("Encoder Button Pushed");
	return ENC_PUSHED;
    }
    if (!butt_state && butEnc)
    {
	butEnc = false;
	//Serial.println("Encoder Button Released");
	return ENC_RELEASED;
    }
    return (butt_state?ENC_DOWN:ENC_UP);
}

// True turns the BL on
void IoTuz::screen_bl(bool state) {
    state ? i2cexp_clear_bits(I2CEXP_LCD_BL_CTR) : i2cexp_set_bits(I2CEXP_LCD_BL_CTR);
}

void IoTuz::reset_tft() {
    tft.setRotation(3);
    tft.fillScreen(ILI9341_BLACK);
    tft.setTextColor(ILI9341_WHITE);
    tft.setTextSize(1);
    tft.scrollTo(0);
}

// maxlength is the maximum number of characters that need to be deleted before writing on top
void IoTuz::tftprint(uint16_t x, uint16_t y, uint8_t maxlength, char *text) {
    if (maxlength > 0) tft.fillRect(x*6, y*8, maxlength*6, 8, ILI9341_BLACK);
    tft.setCursor(x*6, y*8);
    tft.println(text);
}

TS_Point IoTuz::get_touch() {
    // Clear (i.e. set) CS for TS before talking to it
    i2cexp_clear_bits(I2CEXP_TOUCH_CS);
    // Calling getpoint calls SPI.beginTransaction with a speed of only 2MHz, so we need tohttps://github.com/marcmerlin/IoTuz
    // reset the speed to something faster before talking to the screen again.https://github.com/marcmerlin/IoTuz
    TS_Point p = ts.getPoint();
    // Then disable it again so that talking SPI to LCD doesn't reach TS
    i2cexp_set_bits(I2CEXP_TOUCH_CS);

    // Tell the caller that the touch is bogus or none happened by setting z to 0.
    if (p.z < MINPRESSURE || p.z > MAXPRESSURE) p.z = 0;
    // I've seen bogus touches where both x and y were 0
    if (p.x == 0 && p.y == 0) p.z = 0;


    return p;
}

void IoTuz::touchcoord2pixelcoord(uint16_t *pixel_x, uint16_t *pixel_y, uint16_t pixel_z) {
    // Pressure goes from 1000 to 2200 with a stylus but is unreliable,
    // 3000 if you mash a finger in, let's say 2048 range
    // Colors are 16 bits, so multiply pressure by 32 to get a color range from pressure
    // X goes from 320 to 3900 (let's say 3600), Y goes from 200 to 3800 (let's say 3600 too)
    // each X pixel is 11.25 dots of resolution on digitizer, and 15 dots for Y.
    Serial.print("Converted touch coordinates ");
    Serial.print(*pixel_x);
    Serial.print("x");
    Serial.print(*pixel_y);
    Serial.print(" / pressure:");
    Serial.print(pixel_z);
    //*pixel_x = constrain((*pixel_x-320)/11.25, 0, 319);
    //*pixel_y = constrain((*pixel_y-200)/15, 0, 239);
    *pixel_x = map(*pixel_x, TS_MINX, TS_MAXX, 0, tftw);
    *pixel_y = map(*pixel_y, TS_MINY, TS_MAXY, 0, tfth);
    Serial.print(" to pixel coordinates ");
    Serial.print(*pixel_x);
    Serial.print("x");
    Serial.println(*pixel_y);
}

IoTuz::IoTuz()
{
    // Any write to I2CEXP should contain those mask bits so allow reads to work later
    _i2cexp = I2CEXP_IMASK;

    pinMode (ENCODERA_PIN, INPUT_PULLUP);
    pinMode (ENCODERB_PIN, INPUT_PULLUP);

    pinMode(SPI_MOSI, OUTPUT);
    pinMode(SPI_MISO, INPUT);
    pinMode(SPI_CLK, OUTPUT);

    // Joystick Setup
    pinMode(JOYSTICK_BUT_PIN, INPUT_PULLUP);

    // TFT Setup
    pinMode(TFT_CS, OUTPUT);
    pinMode(TFT_DC, OUTPUT);
    pinMode(TFT_RST, OUTPUT);
}

void IoTuz::begin()
{
    Serial.begin(115200);
    Serial.println("Serial Begin");

    // required for i2exp to work
    Wire.begin();

    // Hardware SPI on ESP32 is actually slower than software SPI. Giving 80Mhz
    // here does not make things any faster. There seems to be a fair amount of
    // overhead in the fancier hw SPI on ESP32 which is designed to send more than
    // one byte at the time, and only ends up sending one byte when called from an
    // arduino library.
    // Sadly, using software SPI in the adafruit library would prevent SPI from working
    // in the touch screen code which only supports hardware SPI
    // The TFT code runs at 24Mhz as per below, but testing shows that any speed over 2Mhz
    // seems ignored and taken down to 2Mhz
    //SPI.beginTransaction(SPISettings(24000000, MSBFIRST, SPI_MODE0));

    // Talking to the touch screen can only work at 2Mhz, and both drivers change the SPI
    // speed before sending data, so this works transparently.

    // ESP32 requires an extended begin with pin mappings (which is not supported by the
    // adafruit library), so we do an explicit begin here and then the other SPI libraries work
    // with hardware SPI as setup here (they will do a second begin without pin mappings and
    // that will be ignored).
    SPI.begin(SPI_CLK, SPI_MISO, SPI_MOSI);

    // Turn off TFT by default. 
    // Note this also initializes the read bits on PCF8574 by setting them to 1 as per I2CEXP_IMASK
    i2cexp_set_bits(I2CEXP_LCD_BL_CTR);

    Serial.println("ILI9341 Test!"); 
    tft.begin();
    // read diagnostics (optional but can help debug problems)
    uint8_t x = tft.readcommand8(ILI9341_RDMODE);
    Serial.print("Display Power Mode: 0x"); Serial.println(x, HEX);
    x = tft.readcommand8(ILI9341_RDMADCTL);
    Serial.print("MADCTL Mode: 0x"); Serial.println(x, HEX);
    x = tft.readcommand8(ILI9341_RDPIXFMT);
    Serial.print("Pixel Format: 0x"); Serial.println(x, HEX);
    x = tft.readcommand8(ILI9341_RDIMGFMT);
    Serial.print("Image Format: 0x"); Serial.println(x, HEX);
    x = tft.readcommand8(ILI9341_RDSELFDIAG);
    Serial.print("Self Diagnostic: 0x"); Serial.println(x, HEX); 
    tft.setRotation(3);
    tftw = tft.width(), tfth = tft.height();
    Serial.print("Resolution: "); Serial.print(tftw); 
    Serial.print(" x "); Serial.println(tfth);
    Serial.println(F("Done!"));

#ifdef NEOPIXEL
    // Tri-color APA106 LEDs Setup
    // Mapping is actually Green, Red, Blue (not RGB)
    // Init LEDs to very dark (show they work, but don't blind)
    pixels.begin();
    // This first pixelcolor is ignored, not sure why
    pixels.setPixelColor(0, 10, 10, 10);
    pixels.setPixelColor(1, 5, 5, 5);
    pixels.show();
    // this one works.
    //pixels.setPixelColor(0, 10, 10, 10);
    //pixels.show();
#else
    ws2812_init(RGB_LED_PIN, LED_WS2812B);
    pixels[0] = makeRGBVal(20, 20, 20);
    pixels[1] = makeRGBVal(0, 20, 0);
    ws2812_setColors(NUMPIXELS, pixels);
#endif

    Serial.println(F("LEDs turned on, setting up Accelerometer next"));

    // init accel
    if(!accel.begin()) {
	/* there was a problem detecting the adxl345 ... check your connections */
	Serial.println("Ooops, no ADXL345 detected ... Check your wiring!");
	while(1);
    }
    accel.setRange(ADXL345_RANGE_16_G);
    sensor_t sensor;
    accel.getSensor(&sensor);
    Serial.println("------------------------------------");
    Serial.print  ("Sensor:       "); Serial.println(sensor.name);
    Serial.print  ("Driver Ver:   "); Serial.println(sensor.version);
    Serial.print  ("Unique ID:    "); Serial.println(sensor.sensor_id);
    Serial.print  ("Max Value:    "); Serial.print(sensor.max_value); Serial.println(" m/s^2");
    Serial.print  ("Min Value:    "); Serial.print(sensor.min_value); Serial.println(" m/s^2");
    Serial.print  ("Resolution:   "); Serial.print(sensor.resolution); Serial.println(" m/s^2");  
    Serial.println("------------------------------------");
    Serial.println("");


    Serial.println("Enable rotary encoder ISR:");
    // Initialize rotary encoder reading and decoding
    attachInterrupt(ENCODERA_PIN, read_encoder_ISR, CHANGE);
    attachInterrupt(ENCODERB_PIN, read_encoder_ISR, CHANGE);
}

// vim:sts=4:sw=4
