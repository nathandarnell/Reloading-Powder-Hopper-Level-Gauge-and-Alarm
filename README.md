# Reloading Powder Hopper Level Gauge and Alarm

This was inspired by a video by Farrow Tech: [https://www.youtube.com/watch?v=m2Si6LQx2WA](https://www.youtube.com/watch?v=m2Si6LQx2WA).  The demonstration it is about two years old and I haven't seen anything else like it so I decided to make my own version.  

## Hardware
Mine version uses an ESP8266 NodeMCU, a ST7735S TFT display, a VL6180X time of flight sensor, and a rotary encoder.
  * [NodeMCU ESP8266](https://www.ebay.com/itm/NEW-NodeMcu-Lua-ESP8266-CH340G-ESP-12E-Wireless-WIFI-Internet-Development-Board/233313358002?ssPageName=STRK%3AMEBIDX%3AIT&_trksid=p2057872.m2749.l2649) - The case is designed to fit this particular board.  If you use a different size/shape board or one with different pin definitions it may not work correctly
  * [1.77" TFT Screen](https://www.ebay.com/itm/1-77-inch-1-8-TFT-Color-Display-Module-Breakout-SPI-ST7735S-for-Arduino-UNO-LCD/222565215470?ssPageName=STRK%3AMEBIDX%3AIT&_trksid=p2057872.m2749.l2649) - This particular screen seems to be sold very widely and if you use the same pins to wire it and use the same settings in User_Setup.h for the TFT_eSPI library, it should work fine
  * [VL6180X Time-of-Flight Sensor](https://www.ebay.com/itm/For-Arduino-I2C-Gesture-Recognition-Range-Finder-Optical-Sensor-Module-VL6180X/302991764658?ssPageName=STRK%3AMEBIDX%3AIT&_trksid=p2057872.m2749.l2649) - The Adafruit version of this sensor also seems to work correctly but for about 2x the cost
  * [Rotary Encoder](https://www.amazon.com/gp/product/B07BN3DGBS/ref=ppx_yo_dt_b_asin_title_o02_s00?ie=UTF8&psc=1) - See below for notes about what rotary encoders work well
  
## Libraries and firware.bin
While Farrow Tech was able to make it without libraries, mine uses:
  * [ArduinoMenu](https://github.com/neu-rah/ArduinoMenu)
  * [streamFlow](https://github.com/neu-rah/streamFlow)
  * [TFT_eSPI](https://github.com/Bodmer/TFT_eSPI) - You will have to setup your TFT settings in the User_Setup.h file.  I have included mine if you would like to copy it into the TFT_eSPI library folder
  * [VL6180X](https://github.com/pololu/vl6180x-arduino)
  * [ClickEncoder](https://github.com/soligen2010/encoder) - This is a fork of the library ArduinoMenu uses by default and has some updates over the original
  * [ardunoWebSockets](https://github.com/Links2004/arduinoWebSockets) - This library is needed by newer versions of ArduinoMenu even though it doesn't do anything for this program
  
Because some of the libraries (mostly ArduinoMenu) are under active development, I have downloaded the versions that work as of now (9/4/2019) as zip files for importing into Arduino IDE or PlatformIO.  If you are running into problems and have all of the wiring done like described in PowderHopperGauge.ino, I also uploaded a firmware.bin file that can be uploaded with [nodemcu-pyflasher](https://github.com/marcelstoer/nodemcu-pyflasher/releases) without the need to import libraries or compile anything yourself.  If you make changes to your wiring however, you will need to compile the program and the .bin file will not work for you.

New for this version is the inclusion of the serial interface.  If you are having issues with the click encoder rotations not registering but the clicks are working, you can use the serial interface to navigate the ArduinoMenu options.

## Rotary Encoders
The most difficult troubleshooting seems to be with rotary encoders.  I have tried four different kinds and have found success with two of them.
  * [The best option](https://www.amazon.com/gp/product/B07BN3DGBS/ref=ppx_yo_dt_b_asin_title_o02_s00?ie=UTF8&psc=1) - These have pullup resistors on the button and the rotary encoder and have generally worked correctly.  One of out the six I received did not work at all and one had at lease one deadspot that didn't register rotations.
  * 
  * 
  * 

The STL is included and the original design file is on Onshape if you want to make a copy and adjust the design for another brand of powder hopper: [Onshape Design](https://cad.onshape.com/documents/c187c6855e5b717b6eb50d9e/w/ed8cc32766a24fa2c6d4d2bb/e/9d09fd4bb0b1d438497f7fdc)
