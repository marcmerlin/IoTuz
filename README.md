IoTuz
=====
This is a library for driving the different hardware on the ESP32 based LCA 2017 IoTuz board:
https://github.com/CCHS-Melbourne/iotuz-esp32-hardware

Please see my blog post about the origin of the board and more screenshots/videos:
http://marc.merlins.org/perso/arduino/post_2017-01-16_IoTuz-Driver-for-our-ESP32-board-built-at-Open-Hardware-Miniconf-at-Linux_Conf_au-2017.html

Now, this code has a lot of dependencies. You should be able to gather them all and make it work, but if you want a shortcut, see the release page for a precompiled binary (you'll still need to have your arduino environment setup with an up to date arduino-esp32 environment and be able to compile/upload some hello world before trying this method): https://github.com/marcmerlin/IoTuz/releases (there is more help below on how to setup the environment).

![tassie](https://cloud.githubusercontent.com/assets/1369412/23584813/3b8b49a8-0121-11e7-9833-13882e22dcd4.jpg)

Supported hardware:
- TFT (hw SPI)
  Make sure your library has this patch 
  https://github.com/adafruit/Adafruit_ILI9341/blob/master/Adafruit_ILI9341.cpp#L98
- Touchscreen (hw SPI)
- Replacement touchscreen support (calibration option in case your touchscreen is reversed or has different calibration)
- Rotary Encoder via pin interrupt driven driver
- Support for A and B buttons "hidden" behind the I2C IO multiplexer
- Joystick
- Color LEDs
- Accelerometer
- BME280 (temperature, humidity, pressure)
- IO expander (pcf8574)
- Infrared receiver

Unused/Unsupported:
- SD Card (should be working in very recent arduino-esp driver)
- microphone (miswired on the original board)
- audio (requires a non trivial driver and wouldn't be useful without sdcard support)
- IR TX LED (I had no use for it)

TFT Driver from the standard Adafruit driver in github HEAD now supports 
accelerated SPI on ESP32 (added by me-no-dev), make sure you have a recent
git version that includes this patch from me:
https://github.com/adafruit/Adafruit_ILI9341/pull/26

This library however require a recent esp32 compiler suite (if you installed it during
LCA 2017, that one is way too old):
https://github.com/espressif/arduino-esp32#installation-instructions 

Note that the esp32/tools/esptool binary did not work for me on debian, I had to symlink
esptool.py to esptool to replace the binary, and then things worked.

Please make sure you setup the exception decoder, it will make your life much
easier when you get a crash dump on your serial port:
- https://github.com/espressif/arduino-esp32#installation-instructions 
- https://github.com/me-no-dev/EspExceptionDecoder <<< this

WROVER Support
--------------
Very few people got an IoTuz board, so I ported this code to work on a WROVER board
from expressif. While functionality will be limited without a touch screen, if you add
a 2 axis joystick with a push button, you'll be able to use most of the code.

Instead of a joystick, you can also use a rotary encoder with push button.
I've tested with these:
- https://tkkrlab.nl/wiki/Arduino_KY-040_Rotary_encoder_module
- https://tkkrlab.nl/wiki/Arduino_KY-023_XY-axis_joystick_module

Note #1: you *must* uncomment WROVER in IoTuz.h as well as check the pin mappings there
to connect at least a rotary encoder with push button, or a joystick.

Note #2: the joystick is likely not centered, which will mess up the main menu.
I recommend you try examples/EncoderTestPinIntrButt.ino first (with serial output enabled)
to get your rotary encoder and/or joystick configured.  
If the joystick is reversed, please see void IoTuz::read_joystick in Iotuz.cpp and you'll likely
also want to edit fulldemo/joystick.cpp for the 2 games.



Required External libraries
---------------------------
- Adafruit_GFX
- Adafruit_ILI9341 (please use git version to have my patches or use WROVER lib below if you have that board)
- Adafruit_Sensor (not needed on WROVER)
- Adafruit_ADXL345_U (not needed on WROVER)
- Adafruit_BME280 (not needed on WROVER)
- Adafruit_NeoPixel (not required, but if you do, please use git version to get my ESP32 patch) or
- ESP32-Digital-RGB-LED-Drivers - https://github.com/MartyMacGyver/ESP32-Digital-RGB-LED-Drivers (used by default and included in the source)
- XPT2046_Touchscreen - https://github.com/PaulStoffregen/XPT2046_Touchscreen (not required on WROVER)
- z3t0/Arduino-IRremote - https://github.com/z3t0/Arduino-IRremote/ (please use latest version to have my ESP32 patch)
- WROVER screen support if you have that devel board instead of IoTuz - https://github.com/espressif/WROVER_KIT_LCD

While I wrote all the menu system, IoTuz library and some of the driver support, fulldemo calls 
external demo code written by others, this includes 2 games, and DemoSauce:


See a few screenshots and videos:
![image](https://cloud.githubusercontent.com/assets/1369412/25074744/82b7619c-22b7-11e7-8e0c-99e2d5e20826.png)
![image](https://cloud.githubusercontent.com/assets/1369412/25074745/89e531d8-22b7-11e7-9e5a-e26123518ce0.png)
![image](https://cloud.githubusercontent.com/assets/1369412/25074746/9147f514-22b7-11e7-9125-09b0230f81fd.png)
![image](https://cloud.githubusercontent.com/assets/1369412/25074748/991a8cca-22b7-11e7-9467-843c9a7dc7df.png)
![image](https://cloud.githubusercontent.com/assets/1369412/25074751/9fe893bc-22b7-11e7-91ac-cc3f231a4338.png)
![image](https://cloud.githubusercontent.com/assets/1369412/23584753/a0dd0492-011f-11e7-9898-dd428205e552.png)
![image](https://cloud.githubusercontent.com/assets/1369412/23584755/a81af3a4-011f-11e7-89b6-86de0ad00fcd.png)


Short 2mn Video Summary:
[![IoTuz 2mn Demo](https://cloud.githubusercontent.com/assets/1369412/25075092/d2f4a828-22c0-11e7-8145-6690db60127b.jpg)](https://youtu.be/Kvcvpdip12A "IoTuz 2mn Demo")

Longer Video Demo: https://www.youtube.com/watch?v=562T4j7Pr1Q



Credits
-------
In addition to the original LCA team who built IoTuz, I owe big thanks to:
- me-no-dev for all the help he provided while I was working with his code and integrating drivers
- MartyMacGyver for ESP32-Digital-RGB-LED-Drivers and going above and beyond on debugging
a weird compiler/optimization alignment problem that was crashing I2C when I was using his library ( https://github.com/MartyMacGyver/ESP32-Digital-RGB-LED-Drivers/issues/4#issuecomment-289185283 )
- Adafruit and ￼PaulStoffregen for the demos I ported from their ILI9341 libraries.
