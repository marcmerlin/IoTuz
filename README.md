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
- IR LED (may require use of RMT driver but I had no use for it)

TFT Driver from the standard Adafruit driver in github HEAD now supports 
accelerated SPI on ESP32 (added by me-no-dev), but as of this writing it's
missing some small function:
https://github.com/adafruit/Adafruit_ILI9341/pull/26
If you'd like, you can use my ready to clone, fork:
https://github.com/marcmerlin/Adafruit_ILI9341

This library however require a recent esp32 compiler suite:
https://github.com/espressif/arduino-esp32#installation-instructions 

Note that the esp32/tools/esptool binary did not work for me on debian, I had to symlink
esptool.py to esptool to replace the binary, and then things worked.

Please make sure you setup the exception decoder, it will make your life much
easier when you get a crash dump on your serial port:
- https://github.com/espressif/arduino-esp32#installation-instructions 
- https://github.com/me-no-dev/EspExceptionDecoder <<< this


Required External libraries
---------------------------
- Adafruit_GFX
- Adafruit_ILI9341 - https://github.com/marcmerlin/Adafruit_ILI9341
- Adafruit_Sensor
- Adafruit_ADXL345_U
- Adafruit_BME280
- Adafruit_NeoPixel or
- ESP32-Digital-RGB-LED-Drivers - https://github.com/MartyMacGyver/ESP32-Digital-RGB-LED-Drivers
- XPT2046_Touchscreen - https://github.com/PaulStoffregen/XPT2046_Touchscreen
- z3t0/Arduino-IRremote - https://github.com/marcmerlin/Arduino-IRremote (until https://github.com/z3t0/Arduino-IRremote/pull/425 is merged)

DemoSauce (WIP), if you have the latest libraries and the patch to espressif/Adafruit_ILI9341/pull/1, you will be able to do this:
See a few screenshots and videos: https://goo.gl/photos/bMVqdiAxDkEppuN88
![image](https://cloud.githubusercontent.com/assets/1369412/23584753/a0dd0492-011f-11e7-9898-dd428205e552.png)
![image](https://cloud.githubusercontent.com/assets/1369412/23584755/a81af3a4-011f-11e7-89b6-86de0ad00fcd.png)


Credits
-------
In addition to the original team who built IoTuz, I owe big thanks to:
- me-no-dev for all the help he provided while I was working with his code and integrating drivers
- MartyMacGyver for ESP32-Digital-RGB-LED-Drivers and going above and beyond on debugging
a weird compiler/optimization alignment problem that was crashing I2C when I was using his library ( https://github.com/MartyMacGyver/ESP32-Digital-RGB-LED-Drivers/issues/4#issuecomment-289185283 )
