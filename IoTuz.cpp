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
#ifndef WROVER
// software SPI ought to work on IoTuz, but I get a white screen sometimes if I use it.
//Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, SPI_MOSI, SPI_CLK, TFT_RST, SPI_MISO);
// Whereas this shorter version works reliably for me
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);
#else
WROVER_KIT_LCD tft;
#endif


// If you are lacking the ESP32 patch, you will get no error, but the LEDs will not work
#ifdef NEOPIXEL
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUMPIXELS, RGB_LED_PIN, NEO_GRB + NEO_KHZ800);
#else
rgbVal pixels[NUMPIXELS];
#endif


// ADXL345
Adafruit_ADXL345_Unified accel = Adafruit_ADXL345_Unified(12345);

// Temp/Humidity/Pressure
Adafruit_BME280 bme;

// Touch screen
XPT2046_Touchscreen ts(TS_CS_PIN);  // Param 2 - NULL - No interrupts)

IRrecv irrecv(IR_RX_PIN);
// Must be global to be passed to the IRRecord library.
decode_results IR_result;

// If enabled by the user, call every millisecond and run the Aiko 
// event loop.
void onTimer() {
    // Only run ISR loop events (those must be ISR safe obviously)
    Events.loop(true);
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

#ifndef WROVER
    Wire.beginTransmission(I2C_EXPANDER);
    // Serial.print("Writing to I2CEXP: ");
    // Serial.println(dt);
    Wire.write(dt);
    error = Wire.endTransmission();
    if (error != ku8TWISuccess) {
	    // FIXME: do something here if you like
    }
#endif
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
#ifndef WROVER
    // For read to work, we must have sent 1 bits on the ports that get used as input
    // This is done by i2cexp_clear_bits called in setup.
    Wire.requestFrom(I2C_EXPANDER, 1);	// FIXME: deal with returned error here?
    while (Wire.available() < 1) ;
    uint8_t read = ~Wire.read();	// Apparently one has to invert the bits read
    // When no buttons are pushed, this returns 0x91, which includes some ports
    // we use as output, so we do need to filter out the ports used as read.
    // Serial.println(read, HEX);
    return read;
#endif
}
int16_t IoTuz::read_encoder() 
{
    return encoder0Pos;
}

int8_t IoTuz::encoder_changed() {
    static int16_t old_encoder0Pos = 0;
    int8_t encoder0Diff = encoder0Pos - old_encoder0Pos;

    old_encoder0Pos = encoder0Pos;
    return encoder0Diff;
}

// There is no separate method to return button state changed
// because this one already does it, look for PUSHED and RELEASED
ButtState IoTuz::_but(uint8_t button) 
{
    static bool but[9];
#ifdef WROVER
    uint8_t butt_state = !digitalRead(ENCODER_BUT_PIN);
#else
    uint8_t butt_state = i2cexp_read() & button;
#endif

    if (butt_state && !but[button])
    {
	but[button] = true;
	Serial.println("Button Pushed");
	return BUT_PUSHED;
    }
    if (!butt_state && but[button])
    {
	but[button] = false;
	Serial.println("Button Released");
	return BUT_RELEASED;
    }
    return (butt_state?BUT_DOWN:BUT_UP);
}

ButtState IoTuz::butEnc() 
{
    return(_but(I2CEXP_ENC_BUT));
}

ButtState IoTuz::butA() 
{
    return(_but(I2CEXP_A_BUT));
}

ButtState IoTuz::butB() 
{
    return(_but(I2CEXP_B_BUT));
}


void IoTuz::read_joystick(bool showdebug) 
{
    // X is wired in reverse.
    joyValueX = 4096-analogRead(JOYSTICK_X_PIN);
    joyValueY = analogRead(JOYSTICK_Y_PIN);
    joyBtn = !digitalRead(JOYSTICK_BUT_PIN);

    // Sadly on my board, the middle is 1785/1850 and not 2048/2048 and it's not the same
    // on other boards, so add a huge dead center:
    joyRelX = map(joyValueX, 0, 1600, -5, 0);
    joyRelY = map(joyValueY, 0, 1600, -5, 0);
    if (joyValueX > 1600) joyRelX = map(constrain(joyValueX, 2400, 4095), 2400, 4095, 0, 5);
    if (joyValueY > 1600) joyRelY = map(constrain(joyValueY, 2400, 4095), 2400, 4095, 0, 5);

    if (showdebug) {
	// print the results to the serial monitor:
	Serial.print("X Axis = ");
	Serial.print(joyValueX);
	Serial.print("/rel: ");
	Serial.print(joyRelX);
	Serial.print("\t Y Axis = ");
	Serial.print(joyValueY);
	Serial.print("/rel: ");
	Serial.print(joyRelY);
	Serial.print("\t Joy Button = ");
	Serial.println(joyBtn);
    }
}

// True turns the BL on
void IoTuz::screen_bl(bool state) {
#ifndef WROVER
    state ? i2cexp_clear_bits(I2CEXP_LCD_BL_CTR) : i2cexp_set_bits(I2CEXP_LCD_BL_CTR);
#else
    pinMode(LCD_BL_CTR, OUTPUT);
    digitalWrite(LCD_BL_CTR, LOW);
#endif
}


float IoTuz::battery_level() 
{
    // return volts, 75 is what I hand calculated as a ratio
    return(float(analogRead(BAT_PIN)/75.0));
}

void IoTuz::reset_tft() {
    tft.setRotation(3);
    tft.fillScreen(ILI9341_BLACK);
    tft.setTextColor(ILI9341_WHITE);
    tft.setTextSize(1);
    tft.scrollTo(0);
}

// maxlength is the maximum number of characters that need to be deleted before writing on top
// 55x30 characters
void IoTuz::tftprint(uint8_t x, uint8_t y, uint8_t maxlength, char *text) {
    if (maxlength > 0) tft.fillRect(x*6, y*8, maxlength*6, 8, ILI9341_BLACK);
    tft.setCursor(x*6, y*8);
    tft.println(text);
}

TS_Point IoTuz::get_touch() {
    // Clear (i.e. set) CS for TS before talking to it
    i2cexp_clear_bits(I2CEXP_TOUCH_CS);
    // Calling getpoint calls SPI.beginTransaction with a speed of only 2MHz, so we need to
    // reset the speed to something faster before talking to the screen again.
    TS_Point p = ts.getPoint();
    // Then disable it again so that talking SPI to LCD doesn't reach TS
    i2cexp_set_bits(I2CEXP_TOUCH_CS);
    
    if (ts_revX) p.x = 4096 - p.x;
    if (ts_revY) p.y = 4096 - p.y;

    // Tell the caller that the touch is bogus or none happened by setting z to 0.
    if (p.z < minpressure || p.z > maxpressure) p.z = 0;
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
    *pixel_x = map(*pixel_x, ts_minX, ts_maxX, 0, tftw);
    *pixel_y = map(*pixel_y, ts_minY, ts_maxY, 0, tfth);
    Serial.print(" to pixel coordinates ");
    Serial.print(*pixel_x);
    Serial.print("x");
    Serial.println(*pixel_y);
}

void IoTuz::_getMinMaxTS () {
    TS_Point p;
    uint16_t untouch = 0;

    do {
	p = get_touch();
	sprintf(tft_str, "%d", p.x);
	tftprint(18, 15, 4, tft_str);
	sprintf(tft_str, "%d", p.y);
	tftprint(30, 15, 4, tft_str);

	if (p.x < ts_minX) { ts_minX = p.x; untouch = 0; }
	if (p.y < ts_minY) { ts_minY = p.y; untouch = 0; }
	if (p.x > ts_maxX) { ts_maxX = p.x; untouch = 0; }
	if (p.y > ts_maxY) { ts_maxY = p.y; untouch = 0; }

	sprintf(tft_str, "%d", ts_minX);
	tftprint(21, 16, 4, tft_str);
	sprintf(tft_str, "%d", ts_maxX);
	tftprint(33, 16, 4, tft_str);
	sprintf(tft_str, "%d", ts_minY);
	tftprint(21, 17, 4, tft_str);
	sprintf(tft_str, "%d", ts_maxY);
	tftprint(33, 17, 4, tft_str);

	if (!p.z) untouch++;
	if (!(untouch % 100)) {
	    Serial.print("Untouch count: ");
	    Serial.println(untouch);
	}

    } while (untouch < 300);
}

void IoTuz::calibrateScreen() {
    TS_Point p;

    Serial.print("Old calibration data MinX: ");
    Serial.print(ts_minX);
    Serial.print(", MaxX: ");
    Serial.print(ts_maxX);
    Serial.print(", MinY: ");
    Serial.print(ts_minY);
    Serial.print(", MaxY: ");
    Serial.print(ts_maxY);
    Serial.println();
    ts_minX = ts_minY = ts_maxX = ts_maxY = 2048;

    tftprint(15, 12, 0, "Touch Screen Calibration");
    tftprint(15, 13, 0, "Please Draw the 4 lines");
    tftprint(15, 14, 0, " From center to corner ");
    tftprint(15, 15, 0, "X: 0000     Y: 0000    ");
    tftprint(15, 16, 0, "MinX: 0000, MaxX: 0000");
    tftprint(15, 17, 0, "MinY: 0000, MaxY: 0000");

    tft.drawLine(32 , 24, 0, 0, ILI9341_BLUE);
    // Wait for touch
    do { p = get_touch();} while (!p.z);
    if (p.x > 2048) {
	ts_revX = true;
	tftprint(15, 18, 0, "X Reversed");
    }
    if (p.y > 2048) {
	ts_revY = true;
	tftprint(26, 18, 0, "Y Reversed");
    }

    // Now that ts_revXY are set, get_touch will invert returned
    // coordinates to make handling easier.
    _getMinMaxTS();
    tft.drawLine(287 , 24, 319, 0, ILI9341_BLUE);
    _getMinMaxTS();
    tft.drawLine(32 , 215, 0, 239, ILI9341_BLUE);
    _getMinMaxTS();
    tft.drawLine(287 , 215, 319, 239, ILI9341_BLUE);
    _getMinMaxTS();

    Serial.print("New calibration data MinX: ");
    Serial.print(ts_minX);
    Serial.print(", MaxX: ");
    Serial.print(ts_maxX);
    Serial.print(", MinY: ");
    Serial.print(ts_minY);
    Serial.print(", MaxY: ");
    Serial.print(ts_maxY);
    Serial.println();

    tftprint(30, 20, 0, "Done!");
    delay(3000);
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

    pinMode(IR_RX_PIN, INPUT);
#ifndef WROVER
    pinMode(IR_TX_PIN, OUTPUT);
    pinMode(BAT_PIN, INPUT);
#endif

}

void IoTuz::begin()
{
    Serial.begin(115200);
    Serial.println("Serial Begin");

    // required for i2exp to work but redundant because called by other drivers below too
    //Wire.begin();

    // ESP32 requires an extended begin with pin mappings (which is not supported by the
    // adafruit library), so we do an explicit begin here and then the other SPI libraries work
    // with hardware SPI as setup here (they will do a second begin without pin mappings and
    // that will be ignored).
    // Actually the TFT lib works without this extra begin, but the touch screen library will
    // not, so let's keep it.
#ifndef WROVER
    SPI.begin(SPI_CLK, SPI_MISO, SPI_MOSI);
#endif

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

#ifndef WROVER
    // init accel
    if(!accel.begin()) {
	/* there was a problem detecting the adxl345 ... check your connections */
	Serial.println("Ooops, no ADXL345 detected ... Check your wiring!");
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

    if(!bme.begin()) {
	/* there was a problem detecting the adxl345 ... check your connections */
	Serial.println("Ooops, no BME280 detected ... Check your wiring!");
    }
    // Wait a little bit for the BME to capture data
    delay(100);
    Serial.print("Temperature = ");
    Serial.print(bme.readTemperature());
    Serial.println(" *C");

    Serial.print("Pressure = ");

    Serial.print(bme.readPressure() / 100.0F);
    Serial.println(" hPa");

    Serial.print("Approx. Altitude = ");
    Serial.print(bme.readAltitude(1013.25));
    Serial.println(" m");

    Serial.print("Humidity = ");
    Serial.print(bme.readHumidity());
    Serial.println(" %");
#endif


    Serial.println("Enable rotary encoder ISR:");
    // Initialize rotary encoder reading and decoding
    attachInterrupt(ENCODERA_PIN, read_encoder_ISR, CHANGE);
    attachInterrupt(ENCODERB_PIN, read_encoder_ISR, CHANGE);

    Serial.println("Enable IR receiver ISR:");
    // Sigh, enabling this hardware interrupt made I2C unreliable.
    // Thankfully this HAL patch fixes the issue
    // https://github.com/espressif/arduino-esp32/issues/286
    irrecv.enableIRIn();

    Serial.println(F("Turning on LEDs"));
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
    Serial.println("Before colors");
    ws2812_setColors(NUMPIXELS, pixels);
    Serial.println("After colors");
#endif

    // This is interesting to log in case the last setup hangs
    Serial.println("IoTuz Setup done");
}

// vim:sts=4:sw=4
