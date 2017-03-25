/***************************************************
  Written by Marc MERLIN, License Apache-2
 ****************************************************/

#include <IoTuz.h>
#include <setjmp.h> 
IoTuz iotuz = IoTuz();

#include "tasmanian-devil.c"

bool butA   = false;
bool butB   = false;
bool butEnc = false;
uint16_t joyValueX;
uint16_t joyValueY;
// When driving the LEDs directly, turn off the background handler that controls the LEDs
bool DISABLE_RGB_HANDLER = 0;
// Same with rotary encoder
bool DISABLE_ROTARY_HANDLER = 1;

jmp_buf jump_env;

#define NUMLED 2
// 5 is fast-ish, 1 is super duper fast, 50 is very very slow color transitions
// LED_SPEED can be changed with the remote or the rotary button if pushed
uint8_t LED_SPEED = 5;
//#define DEBUG_LED

// OVerride for LED brightness controlled by rotary knob
uint8_t RGB_LED_BRIGHTNESS = 16;
uint8_t LCD_BRIGHTNESS = 14;

// How many options to display in the rectangle screen
#define NHORIZ 5
#define NVERT 5
// Option names to display on screen
// 40 chars wide, 5 boxes, 8 char per box
// 30 chars high, 5 boxes, 6 lines per box
char* opt_name[NVERT][NHORIZ][3] = {
    { { "", "Finger", "Paint"},  { "Adafruit", "Touch", "Paint"}, { "Joystick", "Absolute", "Paint"}, { "Joystick", "Relative", "Paint"}, { "", "Accel", "Paint"}, },
    { { "Select", "LEDs", "Color"}, { "Rotary", "For Bright", "LED Off"}, { "", "Rotary", "Encoder"}, { "", "Round", "Rects"}, { "Round", "Fill", "Rects"}, },
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

void finger_draw() {
    uint16_t color_pressure, color;
    static uint8_t update_coordinates = 0;
    TS_Point p = iotuz.get_touch();

    if (!p.z) {
	// If we were touching the screen, and we release, show coordinates next time around.
	if (update_coordinates > 0) update_coordinates = 32;
	return;
    }

    uint16_t pixel_x = p.x, pixel_y = p.y;
    iotuz.touchcoord2pixelcoord(&pixel_x, &pixel_y, p.z);

    // Writing coordinates every time is too slow, write less often
    if (update_coordinates == 32) {
	update_coordinates = 0;
	sprintf(iotuz.tft_str, "%d", p.x);
	iotuz.tftprint(2, 0, 4, iotuz.tft_str);
	sprintf(iotuz.tft_str, "%d", p.y);
	iotuz.tftprint(2, 1, 4, iotuz.tft_str);
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
    bool joyBtnValue = !digitalRead(JOYSTICK_BUT_PIN);

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
    // Add 2 because we don't want to write at 0 or it'll wrap to other side
    uint16_t pixel_x = joyValueX/12.8+2;
    uint16_t pixel_y = joyValueY/17+2;
    tft.fillCircle(pixel_x, pixel_y, 2, ILI9341_WHITE);

    // Do not write the cursor values too often, it's too slow
    if (!update_cnt++ % 16)
    {
	sprintf(iotuz.tft_str, "%d > %d", joyValueX, pixel_x);
	iotuz.tftprint(2, 0, 10, iotuz.tft_str);
	sprintf(iotuz.tft_str, "%d > %d", joyValueY, pixel_y);
	iotuz.tftprint(2, 1, 10, iotuz.tft_str);
    }
}

void rotary_encoder() {
    int16_t   encoder = iotuz.read_encoder();
    ButtState encoder_button = iotuz.read_encoder_button();
    int16_t offset;

    if (iotuz.encoder_changed() || encoder_button == ENC_PUSHED || encoder_button == ENC_RELEASED) {
	offset = encoder % 320;
	if (offset < 0) offset += 320;
	if (encoder_button == ENC_RELEASED) offset = 0;

	sprintf(iotuz.tft_str, "%d/%d/%d", encoder_button, encoder, offset);
	iotuz.tftprint(0, 0, 14, iotuz.tft_str);
	tft.scrollTo(offset);
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
    // don't go all the way to the border, or the drawing will wrap to the other side of the screen.
    pixel_x = constrain(pixel_x + move_x, 2, 318);
//#ifdef NEOPIXEL
   pixel_y = constrain(pixel_y + move_y, 2, 238);
//#endif

    // Do not write the cursor values too often, it's too slow
    if (!(update_cnt++ % 32)) {
	sprintf(iotuz.tft_str, "%.1f (%d) > %d", move_x, intmove_x, int(pixel_x));
	iotuz.tftprint(2, 0, 16, iotuz.tft_str);                        
	sprintf(iotuz.tft_str, "%.1f (%d) > %d", move_y, intmove_y, int(pixel_y));
	iotuz.tftprint(2, 1, 16, iotuz.tft_str);
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
    // don't go all the way to the border, or the drawing will wrap to the other side of the screen.
    pixel_x = constrain(pixel_x + accel_x, 2, 318);
    pixel_y = constrain(pixel_y + accel_y, 2, 238);

    // Do not write the cursor values too often, it's too slow
    if (!(update_cnt++ % 32)) {
	sprintf(iotuz.tft_str, "%.1f > %.1f", accel_x, int(pixel_x));
	iotuz.tftprint(2, 0, 10, iotuz.tft_str);
	sprintf(iotuz.tft_str, "%.1f > %.1f", accel_y, int(pixel_y));
	iotuz.tftprint(2, 1, 10, iotuz.tft_str);
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
    if (!p.z) return;

    // Red and Green seem reversed on APA106
#ifdef NEOPIXEL
    pixels.setPixelColor(0, RGB[1], RGB[0], RGB[2]);
    pixels.setPixelColor(1, 255-RGB[1], 255-RGB[0], 255-RGB[2]);
    pixels.show();
#else
    pixels[0] = makeRGBVal(RGB[1], RGB[0], RGB[2]);
    pixels[1] = makeRGBVal(255-RGB[1], 255-RGB[0], 255-RGB[2]);
    ws2812_setColors(NUMPIXELS, pixels);
#endif

    uint16_t pixel_x = p.x, pixel_y = p.y;
    iotuz.touchcoord2pixelcoord(&pixel_x, &pixel_y, p.z);

//    sprintf(iotuz.tft_str, "%d", pixel_x);
//    iotuz.tftprint(2, 0, 3, iotuz.tft_str);
//    sprintf(iotuz.tft_str, "%d", pixel_y);
//    iotuz.tftprint(2, 1, 3, iotuz.tft_str);
    sprintf(iotuz.tft_str, "%.2x/%.2x/%.2x", RGB[0], RGB[1], RGB[2]);
    iotuz.tftprint(0, 0, 8, iotuz.tft_str);

    if (pixel_y < 80) colnum = 0;
    else if (pixel_y < 160) colnum = 1;
    else if (pixel_y < 240) colnum = 2;
    
    color = map(pixel_x, 0, 320, 0, 255);
//    sprintf(iotuz.tft_str, "col %d: %2x", colnum, color);
//    iotuz.tftprint(0, 3, 9, iotuz.tft_str);
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
	iotuz.reset_tft();
	reset_textcoord();
	iotuz.tftprint(0, 2, 0, "But A");
    }
    if (!(butt_state & I2CEXP_A_BUT) && butA)
    {
	butA = false;
	iotuz.tftprint(0, 2, 5, "");
    }
    if (butt_state & I2CEXP_B_BUT && !butB)
    {
	butB = true;
	iotuz.tftprint(0, 3, 0, "But B");
	*need_select = true;
	Serial.println("Button B pushed, going back to main menu");
    }
    if (!(butt_state & I2CEXP_B_BUT) && butB)
    {
	butB = false;
	// When changing modes, this could delete a block over a new mode
	// that draws a background.
	//iotuz.tftprint(0, 3, 5, "");
    }
#if 0
    if (butt_state & I2CEXP_ENC_BUT && !butEnc)
    {
	butEnc = true;
	iotuz.tftprint(0, 4, 0, "Enc But");
    }
    if (!(butt_state & I2CEXP_ENC_BUT) && butEnc)
    {
	butEnc = false;
	iotuz.tftprint(0, 4, 7, "");
    }
#endif
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
		} else if (y*NHORIZ+x == 4 || y*NHORIZ+x == 7) {
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
    tft.fillRect(x*boxw+1, y*boxh+1, boxw-1, boxh-1, ILI9341_LIGHTGREY);
    for (uint16_t i=0; i<200; i++) {
	delay(1);
	// Ensure Aiko events can run.
	Events.loop();
    }
    tft.fillRect(x*boxw+1, y*boxh+1, boxw-1, boxh-1, ILI9341_BLACK);
    for (uint16_t i=0; i<200; i++) {
	delay(1);
	// Ensure Aiko events can run.
	Events.loop();
    }
}

uint8_t get_selection(void) {
    TS_Point p;
    uint8_t x, y, select;

    Serial.println("Waiting for finger touch to select option");
    do {
	p = iotuz.get_touch();
	// Ensure Aiko events can run.
	Events.loop();
    // at boot, I get a fake touch with pressure 1030
    } while (!p.z);

    uint16_t pixel_x = p.x, pixel_y = p.y;
    iotuz.touchcoord2pixelcoord(&pixel_x, &pixel_y, p.z);

    x = pixel_x/(tftw/NHORIZ);
    y = pixel_y/(tfth/NVERT);
    Serial.print("Got touch in box coordinates ");
    Serial.print(x);
    Serial.print("x");
    Serial.print(y);
    Serial.print(" pressure: ");
    Serial.println(p.z);
    show_selected_box(x, y);
	
    return (x + y*NHORIZ);
}

void show_logo(uint16_t offset=0) {
    tft.drawBitmap(0, offset, 320, 240, (uint16_t*)picture3);
}

// FIXME: the returned values do not need to be float, but the variables I use
// are float for smooth transitions, so it carries on to this function for now.
void LED_Color_Picker(uint8_t numled, float *red, float *green, float *blue) {
    uint8_t led_color[3];
    // Initialize LED colors to -1 to keep track of whether each color slot has been assigned 
    // to by random assignment below
    float led[3] = { -1, -1, -1};

    // Keep the first color component bright.
    led_color[0] = random(128, 256);
    // The lower the previous LED is, the brigher that color was
    // and the darker we try to make the second one.
    led_color[1] = random(0, 255);
    // The 2nd color is allowed to go to 0. The less bright is is, the brighter we push this one
    led_color[2] = random(0, (255 - led_color[1])/2);

    // Since color randomization is weighed towards the first color
    // we randomize color attribution between the 3 colors.
    uint8_t i = 0;
    while(i  < 3)
    {
	uint8_t color_guess = random(0, 3);

	// Loop until we get a color slot that's not be assigned to
	if (led[color_guess] != -1.0) continue;
    	led[color_guess] = led_color[i++];
    }

    *red =   led[0];
    *green = led[1];
    *blue =  led[2];
#ifdef DEBUG_LED
    Serial.print("LED ");
    Serial.print(numled);
    Serial.print(": picked RGB Color: ");
    Serial.print((int)*red, HEX);
    Serial.print("/");
    Serial.print((int)*green, HEX);
    Serial.print("/");
    Serial.println((int)*blue, HEX);
#endif
}

bool rgbLedFade(uint8_t red[], uint8_t green[], uint8_t blue[]) {
    static uint8_t RGB_led_stage = 0;
    static float RGBa[NUMLED][3] = { 255, 255, 255 };
    static float RGBb[NUMLED][3];
    static float RGBdiff[NUMLED][3];
    // How many intermediate shades of colors.
    uint16_t rgb_color_change_steps = 20 * LED_SPEED;
    // How many steps we hold the target colors.
    uint8_t rgb_color_hold_steps  = 10 * LED_SPEED;

    if (RGB_led_stage < rgb_color_change_steps)
    {
	for (uint8_t numled=0; numled < NUMLED; numled++) {
	    if (RGB_led_stage == 0) {
		LED_Color_Picker(numled, &RGBb[numled][0], &RGBb[numled][1], &RGBb[numled][2]);
	    }
		for (uint8_t i = 0; i <= 2; i++)
		{
		    if (RGB_led_stage == 0)
		    {
			RGBdiff[numled][i] = (RGBb[numled][i] - RGBa[numled][i]) / rgb_color_change_steps;
		    }
		    RGBa[numled][i] = constrain(RGBa[numled][i] + RGBdiff[numled][i], 0, 255);
		}
		if (RGB_LED_BRIGHTNESS) {
		    red[numled]   = int(RGBa[numled][0]) / (17-RGB_LED_BRIGHTNESS);
		    green[numled] = int(RGBa[numled][1]) / (17-RGB_LED_BRIGHTNESS);
		    blue[numled]  = int(RGBa[numled][2]) / (17-RGB_LED_BRIGHTNESS);
		} else {
		    red[numled]   = 0;
		    green[numled] = 0;
		    blue[numled]  = 0;
		}
	}
	RGB_led_stage++;
	return 1;
    }
    if (RGB_led_stage == rgb_color_change_steps) 
    {
	// Cannot use Serial in an ISR or things will crash
#ifdef DEBUG_LED
	Serial.print(millis());
	Serial.println(": Holding Color for LEDs");
#endif
    }
    if (RGB_led_stage < rgb_color_change_steps + rgb_color_hold_steps)
    {
	RGB_led_stage++;
	return 0;
    }
    else
    {
#ifdef DEBUG_LED
	Serial.print(millis());
	Serial.println(": Done Holding Color for LEDs");
#endif
	RGB_led_stage = 0;
	return 0;
    }
}


void LED_Handler() {
    uint8_t numled;
    // This needs to be static because some calls to rgbLedFade do not return a modified value
    // (holding a color at end of stage)
    static uint8_t red[NUMLED], green[NUMLED], blue[NUMLED];
    static uint8_t oldred[NUMLED], oldgreen[NUMLED], oldblue[NUMLED];
    bool changed = false;

    rgbLedFade(red, green, blue);
    for (uint8_t numled=0; numled<NUMLED; numled++) {
	// APA106 Mapping is actually Green, Red, Blue (not RGB)
	if (! DISABLE_RGB_HANDLER) {
	    // Do not push re-display the same color already being displayed.
	    if ( oldred[numled] == red[numled] && oldgreen[numled] == green[numled] && oldblue[numled] == blue[numled] ) continue;
	    oldred[numled] = red[numled]; oldgreen[numled] = green[numled]; oldblue[numled] = blue[numled];
	    changed = true;

#ifdef NEOPIXEL
	    pixels.setPixelColor(numled, green[numled], red[numled], blue[numled]);
#else
	    pixels[numled] = makeRGBVal(green[numled], red[numled], blue[numled]);
#endif
	}
#if 0
	Serial.print("Set LED ");
	Serial.print(numled);
	Serial.print(": ");
	Serial.print((int)red[numled], HEX);
	Serial.print("/");
	Serial.print((int)green[numled], HEX);
	Serial.print("/");
	Serial.println((int)blue[numled], HEX);
#endif
    }
    if (! DISABLE_RGB_HANDLER && changed) {
#ifdef NEOPIXEL
	pixels.show();
#else
	ws2812_setColors(NUMPIXELS, pixels);
#endif
    }
}

void Rotary_Handler() {
    static int16_t   last_brightness_encoder = 0;
    static int16_t   last_speed_encoder = 0;
    int16_t speed_encoder;
    int16_t brightness_encoder;
    int16_t offset;

    if (! DISABLE_ROTARY_HANDLER) {
	switch (iotuz.read_encoder_button()) {
	case ENC_PUSHED:
	    last_speed_encoder = iotuz.read_encoder();
	    break;

	case ENC_DOWN:
	    speed_encoder = iotuz.read_encoder();
	    offset = (speed_encoder - last_speed_encoder)/4;
	    last_speed_encoder = speed_encoder;

	    if (offset) {
		LED_SPEED = constrain(LED_SPEED + offset, 1, 50);
		sprintf(iotuz.tft_str, "LED Spd:%d", LED_SPEED);
		iotuz.tftprint(29, 0, 10, iotuz.tft_str);
	    }
	    break;

	default:
	    brightness_encoder = iotuz.read_encoder();
	    // Encoder jumps 4 notches for each detent click
	    offset = (brightness_encoder - last_brightness_encoder)/4;
	    last_brightness_encoder = brightness_encoder;

	    if (offset) {
		RGB_LED_BRIGHTNESS = constrain(RGB_LED_BRIGHTNESS + offset, 0, 16);
		sprintf(iotuz.tft_str, "LED Bright:%d", RGB_LED_BRIGHTNESS);
		iotuz.tftprint(40, 0, 13, iotuz.tft_str);
		// 0 -> 0, 1-4 -> 1, 4-8 -> 2, etc...
		LCD_BRIGHTNESS = RGB_LED_BRIGHTNESS+3/4;
	    }

	} 
    }
}

void lcd_flash(uint16_t color) {
    tft.fillScreen(color);
    delay(1000);
    // Here, we want to get back to the main display loop, from whereever we happened
    // to be. Given that we're 2 functions deep and called by an interrupt handler potentially
    // longjump is the best way to unroll those functions and jump back to the main loop.
    // If you are not familiar: http://www.cplusplus.com/reference/csetjmp/longjmp/
    Serial.println("Screen color flashed via IM, longjmp back to main loop");
    longjmp(jump_env, 1);
}

void IR_Handler() {
    static bool lcd_bl = true;

    if (irrecv.decode(&IR_result)) {
	irrecv.resume(); // Receive the next value
	switch (IR_result.value) {
	case IR_RGBZONE_POWER:
	    lcd_bl = !lcd_bl;

	    iotuz.screen_bl(lcd_bl);
	    if (lcd_bl) {
		Serial.println("Toggle LCD and LEDs on");
		RGB_LED_BRIGHTNESS = 16;
		LCD_BRIGHTNESS = 4;
	    } else {
		Serial.println("Toggle LCD and LEDs off");
		RGB_LED_BRIGHTNESS = 0;
		LCD_BRIGHTNESS = 0;
	    }
	    break;

	case IR_RGBZONE_RED:
	    lcd_flash(ILI9341_RED);
	    break;

	case IR_RGBZONE_BLUE:
	    lcd_flash(ILI9341_BLUE);
	    break;

	case IR_RGBZONE_GREEN:
	    lcd_flash(ILI9341_GREEN);
	    break;

	case IR_JUNK:
	    break;

	default:
	    Serial.print("Got unknown IR value: ");
	    Serial.println(IR_result.value, HEX);
	}
    }
}

// This implementation is nice on the port expander by avoiding to toggle it more often than needed
// but it doesn't flip the brightness quickly enough and causes visible flickering as soon as you reach 75%
// due to the 4ms+ during which the screen remains off
#if 0
void LCD_PWM_Handler() {
    static uint8_t pwm_timer = 0;

    if (pwm_timer++ == 16) {
	pwm_timer = 0;
	// when timer resets, turn screen on.
	iotuz.screen_bl(true);
	return;
    }

    // and only turn it off if it reaches the brightness level (for 16 = never)
    if (pwm_timer == LCD_BRIGHTNESS) {
	iotuz.screen_bl(false);
    }

}
#endif

// Scale everything down by 4, in the 50% case, sadly, we have 
void LCD_PWM_Handler() {
    static uint8_t pwm_timer = 0;

    pwm_timer++;
    // on off on off instead of on on off off for less visible flickering
    if (LCD_BRIGHTNESS == 1) {
	iotuz.screen_bl(pwm_timer % 2);
	return;
    }

    if (pwm_timer == 4) {
	pwm_timer = 0;
	// when timer resets, turn screen on.
	// unless brightness is all the way down to 0.
	if (LCD_BRIGHTNESS) iotuz.screen_bl(true);
	return;
    }

    // and only turn it off if it reaches the brightness level (for 16 = never)
    if (pwm_timer == LCD_BRIGHTNESS+1) {
	iotuz.screen_bl(false);
    }

}

void Battery_Handler() {
    sprintf(iotuz.tft_str, "Bat:%4.2fV", iotuz.battery_level());
    iotuz.tftprint(44, 29, 9, iotuz.tft_str);
}

void HumiTemp_Handler() {
    sprintf(iotuz.tft_str, "%5.2fC hum:%2d%%", bme.readTemperature(), (int)bme.readHumidity());
    iotuz.tftprint(0, 29, 14, iotuz.tft_str);
}

void loop() {
    static bool need_select = true;
    static uint8_t select;
    // See lcd_flash's longjmp and
    // http://www.cplusplus.com/reference/csetjmp/setjmp/
    int ret = setjmp (jump_env);
    if (ret) {
	Serial.println("Returned to main loop via longjmp");
	need_select = true;
    }
    
    if (need_select) {
	// reset the screen
	iotuz.reset_tft();
	draw_choices();

	// add overlays
	sprintf(iotuz.tft_str, "LED Spd:%d", LED_SPEED);
	iotuz.tftprint(29, 0, 10, iotuz.tft_str);
	sprintf(iotuz.tft_str, "LED Bright:%d", RGB_LED_BRIGHTNESS);
	iotuz.tftprint(40, 0, 13, iotuz.tft_str);
	Battery_Handler();
	DISABLE_RGB_HANDLER = 0;
	DISABLE_ROTARY_HANDLER = 0;

	select = get_selection();
	Serial.print("Got menu selection #");
	Serial.println(select);
	tft.fillScreen(ILI9341_BLACK);
    }

    // Ok, this is tricky. the last row of demos does not come back through the
    // loop. After they run, Aiko events stop. In turn backlight PWM stops too
    // so we make sure it didn't just stop on an LCD off event.
    if (select >= 20 and need_select)iotuz.screen_bl (true);

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
	// and turn off RGB handler
	if (need_select) { 
	    draw_color_selector();
	    DISABLE_RGB_HANDLER = 1;
	}
	led_color_selector();
	break;
    case LEDOFF:
	RGB_LED_BRIGHTNESS = 0;
	// shortcut scan buttons below and go back to main menu
	need_select = true;
	return;
	break;
    case ROTARTYENC:
	if (need_select) {
	    show_logo();
	    DISABLE_ROTARY_HANDLER = 1;
	}
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

    // Run the Aiko event loop since it's not safe to run from an ISR
    Events.loop();
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
    Serial.println("Turn backlight on");
    // backlight is off by default, turn it on.
    iotuz.screen_bl(true);


    // Tri-color APA106 LEDs Setup
    // Mapping is actually Green, Red, Blue (not RGB)
#ifdef NEOPIXEL
    pixels.setPixelColor(0, 32, 0, 0);
    pixels.setPixelColor(1, 0, 0, 32);
    pixels.show();
#else
    pixels[0] = makeRGBVal(32, 32, 0);
    pixels[1] = makeRGBVal(0, 0, 32);
    ws2812_setColors(NUMPIXELS, pixels);
#endif

    show_logo();
    Serial.println("Logo displayed, waiting for timeout or click");
    tft.setTextColor(ILI9341_BLACK);
    iotuz.tftprint(1, 1, 0, "Click joystick or rotary encoder");
    tft.setTextColor(ILI9341_WHITE);
    for (uint8_t i=0; i<50; i++) {
	delay(100);
	if (iotuz.read_encoder_button() == ENC_RELEASED || !digitalRead(JOYSTICK_BUT_PIN)) {
	    Serial.println("Button clicked");
	    break;
	}
    }
    Serial.println("Setting up Aiko event handlers");

    Events.addHandler(LED_Handler, 10);
    Events.addHandler(Rotary_Handler, 333);
    Events.addHandler(IR_Handler, 100);
    Events.addHandler(LCD_PWM_Handler, 1);
    Events.addHandler(Battery_Handler, 5000);
    Events.addHandler(HumiTemp_Handler, 5000);
    // Make use of my ISR driven mini port of Andy Gelme's Aiko
    // Sadly, I cannot. Calling pixels.show() from an ISR causes crashes.
    //iotuz.enable_aiko_ISR();

    Serial.println("Aiko Event handlers installed");
}

// vim:sts=4:sw=4
