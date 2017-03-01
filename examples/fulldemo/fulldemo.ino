/***************************************************
  This is our GFX example for the Adafruit ILI9341 Breakout and Shield
  ----> http://www.adafruit.com/products/1651

  Check out the links above for our tutorials and wiring diagrams
  These displays use SPI to communicate, 4 or 5 pins are required to
  interface (RST is optional)
  Adafruit invests time and resources providing this open source code,
  please support Adafruit and open-source hardware by purchasing
  products from Adafruit!

  Written by Limor Fried/Ladyada for Adafruit Industries.
  MIT license, all text above must be included in any redistribution
 ****************************************************/

#include <Wire.h>
#include <IoTuz.h>
IoTuz iotuz = IoTuz();

bool butA   = false;
bool butB   = false;
bool butEnc = false;
// Are we drawing on the screen with joystick, accelerator, or finger?
uint16_t joyValueX;
uint16_t joyValueY;
bool joyBtnValue;

// How many options to display in the rectangle screen
#define NHORIZ 5
#define NVERT 5
// Option names to display on screen
// 40 chars wide, 5 boxes, 8 char per box
// 30 chars high, 5 boxes, 6 lines per box
char* opt_name[NVERT][NHORIZ][3] = {
    { { "", "Finger", "Paint"},  { "Adafruit", "Touch", "Paint"}, { "Joystick", "Absolute", "Paint"}, { "Joystick", "Relative", "Paint"}, { "", "Accel", "Paint"}, },
    { { "Select", "LEDs", "Color"}, { "", "LEDs", "Off"}, { "", "Rotary", "Encoder"}, { "", "Round", "Rects"}, { "Round", "Fill", "Rects"}, },
    { { "", "Text", ""}, { "", "Fill", ""}, { "", "Diagonal", "Lines"}, { "Horizon", "Vert", "Lines"}, { "", "Rectangle", ""}, },
    { { "", "Fill", "Rectangle"}, { "", "Circles", ""}, { "", "Fill", "Circles"}, { "", "Triangles", ""}, { "", "Fill", "Triangles"}, },
    { { "", "Tetris", ""}, { "", "Breakout", ""}, { "", "", ""}, { "", "", ""}, { "", "", ""}, },
};
// tft_width, tft_height, calculated in setup after tft init
uint16_t tftw, tfth;
// number of pixels of each selection box (height and width)
uint16_t boxw, boxh;



typedef enum {
    FINGERPAINT = 0,
    TOUCHPAINT = 1,
    JOYABS = 2,
    JOYREL = 3,
    ACCELPAINT = 4,
    COLORLED = 5,
    LEDOFF = 6,
    ROTARTYENC = 7,
    ROUNDREC = 8,
    ROUNDRECFILL = 9,
    TEXT = 10,
    FILL = 11,
    LINES = 12,
    HORIZVERT = 13,
    RECT = 14,
    RECTFILL = 15,
    CIRCLE = 16,
    CIRCFILL = 17,
    TRIANGLE = 18,
    TRIFILL = 19,
    TETRIS = 20,
    BREAKOUT = 21,
} LCDtest;


void lcd_test(LCDtest choice) {
    switch (choice) {

    case TEXT:
	Serial.print(F("Text                     "));
	Serial.println(testText());
        break;
	 
    case FILL:
	Serial.print(F("Screen fill              "));
	Serial.println(testFillScreen());
        break;

    case LINES:
	Serial.print(F("Lines                    "));
	Serial.println(testLines(ILI9341_CYAN));
	break;

    case HORIZVERT:
	Serial.print(F("Horiz/Vert Lines         "));
	Serial.println(testFastLines(ILI9341_RED, ILI9341_BLUE));
        break;

    case RECT:
	Serial.print(F("Rectangles (outline)     "));
	Serial.println(testRects(ILI9341_GREEN));
	break;

    case RECTFILL:
	Serial.print(F("Rectangles (filled)      "));
	Serial.println(testFilledRects(ILI9341_YELLOW, ILI9341_MAGENTA));
	break;

    case CIRCLE:
	Serial.print(F("Circles (outline)        "));
	Serial.println(testCircles(10, ILI9341_WHITE));
	break;

    case CIRCFILL:
	Serial.print(F("Circles (filled)         "));
	Serial.println(testFilledCircles(10, ILI9341_MAGENTA));
	break;

    case TRIANGLE:
	Serial.print(F("Triangles (outline)      "));
	Serial.println(testTriangles());
	break;

    case TRIFILL:
	Serial.print(F("Triangles (filled)       "));
	Serial.println(testFilledTriangles());
	break;

    case ROUNDREC:
	Serial.print(F("Rounded rects (outline)  "));
	Serial.println(testRoundRects());
	break;

    case ROUNDRECFILL:
	Serial.print(F("Rounded rects (filled)   "));
	Serial.println(testFilledRoundRects());
	break;
    }
}

// maxlength is the maximum number of characters that need to be deleted before writing on top
void tftprint(uint16_t x, uint16_t y, uint8_t maxlength, char *text) {
    if (maxlength > 0) tft.fillRect(x*6, y*8, maxlength*6, 8, ILI9341_BLACK);
    tft.setCursor(x*6, y*8);
    tft.println(text);
}

void finger_draw() {
    uint16_t color_pressure, color;
    static uint8_t update_coordinates = 0;
    TS_Point p = iotuz.get_touch();

    if (p.z < MINPRESSURE || p.z > MAXPRESSURE) {
	// If we were touching the screen, and we release, show coordinates next time around.
	if (update_coordinates > 0) update_coordinates = 32;
	return;
    }

    uint16_t pixel_x = p.x, pixel_y = p.y;
    iotuz.touchcoord2pixelcoord(&pixel_x, &pixel_y);

    // Writing coordinates every time is too slow, write less often
    if (update_coordinates == 32) {
	update_coordinates = 0;
	sprintf(iotuz.tft_str, "%d", p.x);
	tftprint(2, 0, 4, iotuz.tft_str);
	sprintf(iotuz.tft_str, "%d", p.y);
	tftprint(2, 1, 4, iotuz.tft_str);
    }

    // Colors are 16 bits, 5 bit: red, 6 bits: green, 5 bits: blue
    // to map a pressure number to colors and avoid random black looking colors,
    // let's seed the color with 2 lowest bits per color: 0001100001100011
    // this gives us 10 bits we need to fill in for which color we'll use,
    color_pressure = p.z-1000;
    if (p.z < 1000) color_pressure = 0;
    color_pressure = constrain(color_pressure, 0, 2047)/2;
    color = tenbitstocolor(color_pressure);
    tft.fillCircle(pixel_x, pixel_y, 2, color);
    update_coordinates++;
}

void read_joystick(bool showdebug=true) {
    // read the analog in value:
    joyValueX = 4096-analogRead(JOYSTICK_X_PIN);
    joyValueY = analogRead(JOYSTICK_Y_PIN);
    joyBtnValue = !digitalRead(JOYSTICK_BUT_PIN);

    if (showdebug) {
	// print the results to the serial monitor:
	Serial.print("X Axis = ");
	Serial.print(joyValueX);
	Serial.print("\t Y Axis = ");
	Serial.print(joyValueY);
	Serial.print("\t Joy Button = ");
	Serial.println(joyBtnValue);
    }
}

// Draw the dot directly to where the joystick is pointing.
void joystick_draw() {
    static int8_t update_cnt = 0;
    // 4096 -> 320 (divide by 12.8) and -> 240 (divide by 17)
    // Sadly on my board, the middle is 1785/1850 and not 2048/2048
    read_joystick();
    uint16_t pixel_x = joyValueX/12.8;
    uint16_t pixel_y = joyValueY/17;
    tft.fillCircle(pixel_x, pixel_y, 2, ILI9341_WHITE);

    // Do not write the cursor values too often, it's too slow
    if (!update_cnt++ % 16)
    {
	sprintf(iotuz.tft_str, "%d > %d", joyValueX, pixel_x);
	tftprint(2, 0, 10, iotuz.tft_str);
	sprintf(iotuz.tft_str, "%d > %d", joyValueY, pixel_y);
	tftprint(2, 1, 10, iotuz.tft_str);
    }
}

void rotary_encoder() {

    int16_t   encoder = iotuz.read_encoder();
    ButtState encoder_button = iotuz.read_encoder_button();

    if (iotuz.encoder_changed() || encoder_button == ENC_PUSHED || encoder_button == ENC_RELEASED) {
	sprintf(iotuz.tft_str, "%d/%d", encoder_button, encoder);
	tftprint(0, 0, 10, iotuz.tft_str);
    }
}

// Move the dot relative to the joystick position (like driving a ball).
void joystick_draw_relative() {
    static uint16_t update_cnt = 0;
    static float pixel_x = 160;
    static float pixel_y = 120;
    // Sadly on my board, the middle is 1785/1850 and not 2048/2048
    read_joystick();
    float move_x = (joyValueX-2300.0)/2048;
    float move_y = (joyValueY-1850.0)/2048;
    int8_t intmove_x = map(joyValueX, 0, 1700, -5, 0);
    int8_t intmove_y = map(joyValueY, 0, 1700, -5, 0);
    if (joyValueX > 1700) intmove_x = map(constrain(joyValueX, 2300, 4096), 2300, 4096, 0, 5);
    if (joyValueY > 1700) intmove_y = map(constrain(joyValueY, 2300, 4096), 2300, 4096, 0, 5);

    tft.fillCircle(int(pixel_x), int(pixel_y), 2, tenbitstocolor(update_cnt % 1024));
    pixel_x = constrain(pixel_x + move_x, 0, 319);
    pixel_y = constrain(pixel_y + move_y, 0, 239);

    // Do not write the cursor values too often, it's too slow
    if (!(update_cnt++ % 32)) {
	sprintf(iotuz.tft_str, "%.1f (%d) > %d", move_x, intmove_x, int(pixel_x));
	tftprint(2, 0, 16, iotuz.tft_str);                        
	sprintf(iotuz.tft_str, "%.1f (%d) > %d", move_y, intmove_y, int(pixel_y));
	tftprint(2, 1, 16, iotuz.tft_str);
    }
}

void accel_draw() {
    static uint16_t update_cnt = 0;
    static float pixel_x = 160;
    static float pixel_y = 120;
    sensors_event_t event; 
    accel.getEvent(&event);
    float accel_x = -event.acceleration.x;
    // My accelerator isn't really level, it shows 2 when my board is flat
    float accel_y = event.acceleration.y - 2;

    tft.fillCircle(int(pixel_x), int(pixel_y), 2, tenbitstocolor(update_cnt % 1024));
    pixel_x = constrain(pixel_x + accel_x, 0, 319);
    pixel_y = constrain(pixel_y + accel_y, 0, 239);

    // Do not write the cursor values too often, it's too slow
    if (!(update_cnt++ % 32)) {
	sprintf(iotuz.tft_str, "%.1f > %.1f", accel_x, int(pixel_x));
	tftprint(2, 0, 10, iotuz.tft_str);
	sprintf(iotuz.tft_str, "%.1f > %.1f", accel_y, int(pixel_y));
	tftprint(2, 1, 10, iotuz.tft_str);
    }
}

void draw_color_selector() {
    // 240 pixels high: 75/5 (R) + 75/5 (G) + 75/5 (B)
    // 6 bits of colors for G, 64 colors, 320 pixels wide: 5 pixels wide per color tone
    for (uint8_t i=0; i<3; i++) {
	for (uint8_t j=0; j<64; j++) {
	    uint16_t color = j;
	    
	    // R and B are only 32 shades (5 bits) while G is 64 shades/6 bits.
	    if (i != 1) color /= 2;

	    if (i == 0) color = color << 11;
	    if (i == 1) color = color << 5;
	    tft.fillRect(j*5, i*80, 5, 75, color);
	}
    }   
}

void led_color_selector() {
    uint16_t color_pressure;
    uint8_t colnum, color;
    static uint8_t RGB[3] = {255, 255, 255};

    TS_Point p = iotuz.get_touch();

    if (p.z < MINPRESSURE || p.z > MAXPRESSURE) {
	return;
    }

    // Red and Green seem reversed on APA106
    pixels.setPixelColor(0, RGB[1], RGB[0], RGB[2]);
    pixels.setPixelColor(1, 255-RGB[1], 255-RGB[0], 255-RGB[2]);
    pixels.show();

    uint16_t pixel_x = p.x, pixel_y = p.y;
    iotuz.touchcoord2pixelcoord(&pixel_x, &pixel_y);

//    sprintf(iotuz.tft_str, "%d", pixel_x);
//    tftprint(2, 0, 3, iotuz.tft_str);
//    sprintf(iotuz.tft_str, "%d", pixel_y);
//    tftprint(2, 1, 3, iotuz.tft_str);
    sprintf(iotuz.tft_str, "%.2x/%.2x/%.2x", RGB[0], RGB[1], RGB[2]);
    tftprint(0, 0, 8, iotuz.tft_str);

    if (pixel_y < 80) colnum = 0;
    else if (pixel_y < 160) colnum = 1;
    else if (pixel_y < 240) colnum = 2;
    
    color = map(pixel_x, 0, 320, 0, 255);
//    sprintf(iotuz.tft_str, "col %d: %2x", colnum, color);
//    tftprint(0, 3, 9, iotuz.tft_str);
    tft.fillRect(0, 80*(colnum+1)-5, 320, 4, ILI9341_BLACK);
    tft.fillTriangle(pixel_x, 80*(colnum+1)-5, pixel_x-2, 80*(colnum+1)-2, pixel_x+2, 80*(colnum+1)-2, ILI9341_WHITE);
    
    RGB[colnum] = color;
}

uint16_t tenbitstocolor(uint16_t tenbits) {
    uint16_t color;

    // TFT colors are 5 bit Red, 6 bits Green, 5 bits Blue, we want to avoid
    // black looking colors, so we seed the last 2 bits of each color to 1
    // and then interleave 10 bits ot color spread across the remaining bits
    // that affect the end color more visibly.
    // 3 highest bits (9-7), get shifted 6 times to 15-13
    // 4 middle  bits (6-3), get shifted 4 times to 10-07
    // 3 lowest  bits (2-0), get shifted 2 times to 04-02
    // This means that if the 10 input bits are 0, the output color is
    // 00011000 01100011 = 0x1863
    color = B00011000*256 + B01100011 + ((tenbits & 0x380) << 6) +
					((tenbits & B01111000) << 4) +
					((tenbits & B00000111) << 2);

//    Serial.print("Color: ");
//    Serial.print(tenbits, HEX);
//    Serial.print(" -> ");
//    Serial.println(color, HEX);
    return color;
}


void scan_buttons(bool *need_select) {
    uint8_t butt_state = iotuz.i2cexp_read();
    *need_select = false;

    if (butt_state & I2CEXP_A_BUT && !butA)
    {
	butA = true;
	reset_tft();
	reset_textcoord();
	tftprint(0, 2, 0, "But A");
    }
    if (!(butt_state & I2CEXP_A_BUT) && butA)
    {
	butA = false;
	tftprint(0, 2, 5, "");
    }
    if (butt_state & I2CEXP_B_BUT && !butB)
    {
	butB = true;
	tftprint(0, 3, 0, "But B");
	*need_select = true;
    }
    if (!(butt_state & I2CEXP_B_BUT) && butB)
    {
	butB = false;
	// When changing modes, this could delete a block over a new mode
	// that draws a background.
	//tftprint(0, 3, 5, "");
    }
    if (butt_state & I2CEXP_ENC_BUT && !butEnc)
    {
	butEnc = true;
	tftprint(0, 4, 0, "Enc But");
    }
    if (!(butt_state & I2CEXP_ENC_BUT) && butEnc)
    {
	butEnc = false;
	tftprint(0, 4, 7, "");
    }
}

void reset_tft() {
    tft.setRotation(3);
    tft.fillScreen(ILI9341_BLACK);
    tft.setTextColor(ILI9341_WHITE);
    tft.setTextSize(1);
}

void reset_textcoord() {
    tft.setCursor(0, 0);
    tft.println("x=");
    tft.print("y=");
}


void draw_choices(void) {

    for(uint16_t x=tftw/NHORIZ; x<tftw; x+=boxw) tft.drawFastVLine(x, 0, tfth, ILI9341_LIGHTGREY);
    for(uint16_t y=tfth/NVERT;  y<tfth; y+=boxh) tft.drawFastHLine(0, y, tftw, ILI9341_LIGHTGREY);
    
    for(uint8_t y=0; y<NVERT; y++) { 
	for(uint8_t x=0; x<NHORIZ; x++) { 
	    for(uint8_t line=0; line<3; line++) { 
		tft.setCursor(x*boxw + 4, y*boxh + line*8 + 16);
		if (y*NHORIZ+x == 1 || y*NHORIZ+x == 5 || y*NHORIZ+x == 20 || y*NHORIZ+x == 21) {
		    tft.setTextColor(ILI9341_RED);
		} else if (y*NHORIZ+x == 4) {
		    tft.setTextColor(ILI9341_BLUE);
		} else {
		    tft.setTextColor(ILI9341_WHITE);
		}
		tft.println(opt_name[y][x][line]);
	    }
	}
    }

}

void show_selected_box(uint8_t x, uint8_t y) {
    tft.fillRect(x*boxw, y*boxh, boxw, boxh, ILI9341_LIGHTGREY);
}

uint8_t get_selection(void) {
    TS_Point p;
    uint8_t x, y, select;

    Serial.println("Waiting for finger touch to select option");
    do {
	p = iotuz.get_touch();
    // at boot, I get a fake touch with pressure 1030
    } while ( p.z < 1060);

    uint16_t pixel_x = p.x, pixel_y = p.y;
    iotuz.touchcoord2pixelcoord(&pixel_x, &pixel_y);

    x = pixel_x/(tftw/NHORIZ);
    y = pixel_y/(tfth/NVERT);
    Serial.print("Got touch in box coordinates ");
    Serial.print(x);
    Serial.print("x");
    Serial.print(y);
    Serial.print(" pressure: ");
    Serial.println(p.z);
    return (x + y*NHORIZ);
}

void loop(void) {
    static bool need_select = true;
    static uint8_t select;
    
    if (need_select) {
	reset_tft();
	draw_choices();
	select = get_selection();
	Serial.print("Got menu selection #");
	Serial.println(select);
	tft.fillScreen(ILI9341_BLACK);

	// Kind of random LED illumination
	pixels.setPixelColor(0, (select % 5)*50, 0, 255);
	pixels.setPixelColor(1, 0, (select/5)*64, 255);
	pixels.show();
    }


    // The first 4 demos display x/y coordinate text in the upper left corner
    // After the first time around the loop, need_select gets reset to false
    // and the coordinate text is not rewritten (to save screen drawing time)
    if (select <= 3 and need_select) reset_textcoord();
    switch (select) {
    case FINGERPAINT:
	finger_draw();
	break;
    case TOUCHPAINT:
	// First time around the loop, draw a color selection circle
	// FIXME, touch screen is not currently working
	if (need_select) touchpaint_setup();
	touchpaint_loop();
	break;
    case JOYABS:
	joystick_draw();
	break;
    case JOYREL:
	joystick_draw_relative();
	break;
    case ACCELPAINT:
	accel_draw();
	break;
    case COLORLED:
	// First time around the loop, draw a color selection circle
	if (need_select) draw_color_selector();
	led_color_selector();
	break;
    case LEDOFF:
	pixels.setPixelColor(0, 0, 0, 0);
	pixels.setPixelColor(1, 0, 0, 0);
	pixels.show();
	// shortcut scan buttons below and go back to main menu
	need_select = true;
	return;
	break;
    case ROTARTYENC:
	rotary_encoder();
	break;
    case TETRIS:
	// First time around the loop, draw a color selection circle
	if (need_select) tetris_setup();
	tetris_loop();
	break;
    case BREAKOUT:
	// First time around the loop, draw a color selection circle
	if (need_select) breakout_setup();
	breakout_loop();
	break;
    default:
	if (select >= 8) {
	    Serial.print("Running LCD Demo #");
	    Serial.println(select);
	    lcd_test((LCDtest) select);
	    delay(500);
	    // shortcut scan buttons below and go back to main menu
	    need_select = true;
	    return;
	}
    }
    // resets need_select to false unless 'B' is pushed.
    scan_buttons(&need_select);
    delay(1);
}


void setup() {
    // Basic init and pin mapping that should be common to anything using IoTuz
    iotuz.begin();
    tftw = iotuz.tftw;
    tfth = iotuz.tfth;
    boxw = tftw/NHORIZ;
    boxh = tfth/NVERT;
    Serial.print("Selection Box Size: "); Serial.print(boxw); 
    Serial.print(" x "); Serial.println(boxh);
    // backlight is off by default, turn it on.
    iotuz.screen_bl(true);


    // Tri-color APA106 LEDs Setup
    // Mapping is actually Green, Red, Blue (not RGB)
    pixels.begin();
    pixels.setPixelColor(0, 255, 0, 0);
    pixels.setPixelColor(1, 0, 0, 255);
    pixels.show();
}

// vim:sts=4:sw=4
