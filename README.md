# Reloading Powder Hopper Level Gauge and Alarm

This was inspired by a video by Farrow Tech: [https://www.youtube.com/watch?v=m2Si6LQx2WA](https://www.youtube.com/watch?v=m2Si6LQx2WA).  The demonstration it is about two years old and I haven't seen anything else like it so I decided to make my own version.  Mine uses an ESP8266 NodeMCU, a ST7735S TFT display, a VL6180X time of flight sensor, and a rotary encoder.  While he was able to make it without libraries, mine uses:
  * [ArduinoMenu](https://github.com/neu-rah/ArduinoMenu)
  * [streamFlow](https://github.com/neu-rah/streamFlow)
  * [TFT_eSPI](https://github.com/Bodmer/TFT_eSPI) - You will have to setup your TFT settings in the User_Setup.h file
  * [VL6180X](https://github.com/pololu/vl6180x-arduino)
  * [ClickEncoder](https://github.com/soligen2010/encoder) - This is a fork of the library ArduinoMenu uses by default and has some updates over the original
  * [ardunoWebSockets](https://github.com/Links2004/arduinoWebSockets) - This library is needed by newer versions of ArduinoMenu even though it doesn't do anything for this program

## Libraries and firware.bin
Because some of the libraries (mostly ArduinoMenu) are under active development, I have downloaded the versions that work as of now as zip files for importing into Arduino or PlatformIO.  If you are running into problems and have all of the wiring done like described in PowderHopperGauge.ino I also uploaded a firmware.bin file that can be uploaded with [nodemcu-pyflasher](https://github.com/marcelstoer/nodemcu-pyflasher/releases) without the need to import libraries or compile anything yourself.  If you make changes to your wiring however, you will need to compile the program and the .bin file will not work for you.

New for this version is the inclusion of the serial interface.  If you are having issues with the click encoder rotations not registering but the clicks are working, you can use the serial interface to navigate the ArduinoMenu options.

## Rotary Encoders
The most difficult troubleshooting seems to be with rotary encoders.  I have tried four different kinds and have found success with two of them.

The STL is included and the original design file is on Onshape if you want to make a copy and adjust the design for another brand of powder hopper: [Onshape Design](https://cad.onshape.com/documents/c187c6855e5b717b6eb50d9e/w/ed8cc32766a24fa2c6d4d2bb/e/9d09fd4bb0b1d438497f7fdc)
