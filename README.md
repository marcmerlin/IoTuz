IoTuz
=====
This is a library for driving the different hardware on the ESP32 based LCA 2017 IoTuz board:
https://github.com/CCHS-Melbourne/iotuz-esp32-hardware

![tassie](https://cloud.githubusercontent.com/assets/1369412/23584813/3b8b49a8-0121-11e7-9833-13882e22dcd4.jpg)

Supported hardware:
- TFT (hw SPI)
  Make sure your library has this patch 
  https://github.com/adafruit/Adafruit_ILI9341/blob/master/Adafruit_ILI9341.cpp#L98
- Touchscreen (hw SPI)
- Rotary Encoder via interrupt driven driver
- Support for A and B buttons "hidden" behind the I2C IO multiplexer
- Joystick
- Color LEDs (requires a patch I wrote for the adafruit library)
  https://github.com/adafruit/Adafruit_NeoPixel/pull/125
- Accelerometer
- IO expander (pcf8574)

Unused/Unsupported:
- SD Card (not working currently in ESP32)
  https://github.com/espressif/arduino-esp32/issues/43
- microphone (miswired on the original board)
- audio (requires a non trivial driver and wouldn't be useful without sdcard support)
- IR LED / IR receiver (haven't yet used them)

TFT Driver with standard Adafruit driver is slow in hardare SPI due to lock and imperfect support, see
https://github.com/adafruit/Adafruit_ILI9341/issues/19#issuecomment-262851759
on how to #define CONFIG_DISABLE_HAL_LOCKS 1
See also https://github.com/espressif/arduino-esp32/issues/149

As a result, my code now relies on a new driver from @me-no-dev optimized for ESP32:
- https://github.com/espressif/Adafruit_ILI9341
- https://github.com/espressif/Adafruit-GFX-Library

These libraries however require a recent esp32 compiler suite:
https://github.com/espressif/arduino-esp32#installation-instructions 

Note that the esp32/tools/esptool binary did not work for me on debian, I had to symlink
esptool.py to esptool to replace the binary, and then things worked.

Please make sure you setup the exception decoder, it will make your life much
easier when you get a crash dump on your serial port:
- https://github.com/espressif/arduino-esp32#installation-instructions 
- https://github.com/me-no-dev/EspExceptionDecoder <<< this


Required External libraries
---------------------------
- Adafruit_GFX (https://github.com/espressif/Adafruit-GFX-Library)
- Adafruit_ILI9341 (https://github.com/espressif/Adafruit_ILI9341)
- Adafruit_NeoPixel
- Adafruit_Sensor
- Adafruit_ADXL345_U
- XPT2046_Touchscreen - https://github.com/PaulStoffregen/XPT2046_Touchscreen
- Note that for some demos, you will most likely want this patch: https://github.com/espressif/Adafruit_ILI9341/pull/1

DemoSauce, if you have the latest libraries and the patch to espressif/Adafruit_ILI9341/pull/1, you will be able to do this:
See a few screenshots and videos: https://goo.gl/photos/bMVqdiAxDkEppuN88
![image](https://cloud.githubusercontent.com/assets/1369412/23584753/a0dd0492-011f-11e7-9898-dd428205e552.png)
![image](https://cloud.githubusercontent.com/assets/1369412/23584755/a81af3a4-011f-11e7-89b6-86de0ad00fcd.png)


Credits
-------
In addition to the original team who built IoTuz, I owe big thanks to:
- me-no-dev for all the help he provided while I was working with his code and integrating drivers
