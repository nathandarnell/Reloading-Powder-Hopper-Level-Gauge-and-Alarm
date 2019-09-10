# Reloading Powder Hopper Level Gauge and Alarm

This was inspired by a video by Farrow Tech: [https://www.youtube.com/watch?v=m2Si6LQx2WA](https://www.youtube.com/watch?v=m2Si6LQx2WA).  The demonstration it is about two years old and I haven't seen anything else like it so I decided to make my own version.  

## Hardware
My version uses an ESP8266 NodeMCU, a ST7735S TFT display, a VL6180X time of flight sensor, a rotary encoder, and some various hardware.
  * [NodeMCU ESP8266](https://www.ebay.com/itm/NEW-NodeMcu-Lua-ESP8266-CH340G-ESP-12E-Wireless-WIFI-Internet-Development-Board/233313358002?ssPageName=STRK%3AMEBIDX%3AIT&_trksid=p2057872.m2749.l2649) - The case is designed to fit this particular board.  If you use a different size/shape board or one with different pin definitions it may not work correctly
  * [1.77" TFT Screen](https://www.ebay.com/itm/1-77-inch-1-8-TFT-Color-Display-Module-Breakout-SPI-ST7735S-for-Arduino-UNO-LCD/222565215470?ssPageName=STRK%3AMEBIDX%3AIT&_trksid=p2057872.m2749.l2649) - This particular screen seems to be sold very widely and if you use the same pins to wire it and use the same settings in User_Setup.h for the TFT_eSPI library, it should work fine
  * [VL6180X Time-of-Flight Sensor](https://www.ebay.com/itm/For-Arduino-I2C-Gesture-Recognition-Range-Finder-Optical-Sensor-Module-VL6180X/302991764658?ssPageName=STRK%3AMEBIDX%3AIT&_trksid=p2057872.m2749.l2649) - The Adafruit version of this sensor also seems to work correctly but is about 2x the cost
  * [Rotary Encoder](https://www.amazon.com/gp/product/B07BN3DGBS/ref=ppx_yo_dt_b_asin_title_o02_s00?ie=UTF8&psc=1) - See below for notes on what rotary encoders work well
  * 6 M3 nuts
  * 2 M2.5 nuts
  * 6 M3 x 10 screws
  * 2 M2.5 x 10 screws
  
## Libraries and firware.bin
While Farrow Tech was able to make it without libraries, mine uses:
  * [ArduinoMenu](https://github.com/neu-rah/ArduinoMenu)
  * [streamFlow](https://github.com/neu-rah/streamFlow)
  * [TFT_eSPI](https://github.com/Bodmer/TFT_eSPI) - You will have to setup your TFT settings in the User_Setup.h file.  I have included mine if you would like to copy it into the TFT_eSPI library folder.  If you use the library I included it is already correct
  * [VL6180X](https://github.com/pololu/vl6180x-arduino)
  * [ClickEncoder](https://github.com/soligen2010/encoder) - This is a fork of the rotary encoder library ArduinoMenu uses by default and has some updates over the original
  * [ardunoWebSockets](https://github.com/Links2004/arduinoWebSockets) - This library is needed by newer versions of ArduinoMenu even though it doesn't do anything for this program
  
### Importing Libraries
Because some of the libraries (mostly ArduinoMenu) are under active development, I have downloaded the versions that work currently (9/4/2019) as zip files for importing into Arduino IDE or PlatformIO.  For Arduino there is a import a zip file to library menu option.  For PlatformIO with VS Code extract the zip files to the include folder within the project folder.  

### firmware.bin
If you are running into problems and have all of the wiring done like described in PowderHopperGauge.ino, I also uploaded a firmware.bin file that can be uploaded with [nodemcu-pyflasher](https://github.com/marcelstoer/nodemcu-pyflasher/releases) without the need to import libraries or compile anything yourself.  If you make changes to your wiring however, you will need to compile the program and the firmware.bin file will not work for you.

New for this version is the inclusion of the serial interface.  If you are having issues with the click encoder rotations not registering but the clicks are working, you can use the serial interface to navigate the ArduinoMenu options.

## Rotary Encoders
The most difficult troubleshooting seems to be with rotary encoders.  I have tried four different kinds and have found success with two of them.  There are two seperate issues that have come up: how the rotary encoder counts pulses and how the circuit uses pullups and capacitors.
  * <img src="/images/61naG4DReuL._SX679_.jpg" width="200" /> <img src="/images/618-nZupIYL._SX679_.jpg" width="200" />[Amazon 6 pack](https://www.amazon.com/gp/product/B07BN3DGBS/ref=ppx_yo_dt_b_asin_title_o02_s00?ie=UTF8&psc=1) - These have pullup resistors on the button and on the rotary encoder and mostly worked correctly.  Out of the six I received one did not work at all and one had at least a single deadspot that didn't register rotations.  Otherwise the price and consistancy has been good.
  * <img src="/images/_mg_2645.jpg" width="200" /> <img src="/images/_mg_2646.jpg" width="200" />[Sunfounder Rotary Encoder](https://www.sunfounder.com/rotary-encoder-module.html) - These also have pullups on all the pins as well as a LED so you can tell when it's powered on.  These are the most expensive option ($6.99 each) but the quality of the encoder and the board is very good.
  * <img src="/images/s-l500%20(3).jpg" width="200" /> <img src="/images/s-l500%20(2).jpg" width="200" />[Round PCB with capacitors for 5V](https://www.ebay.com/itm/Rotary-encoder-module-brick-sensor-development-audio-potentiometer-knob-SKUS/233308683253?ssPageName=STRK%3AMEBIDX%3AIT&_trksid=p2057872.m2749.l2649) - These seem like a great option since they have capacitors for debouncing but they only work correctly with 5V and will not work at 3.3V.  They seem easy to distinguish since the power pins are labeled "5V" and not "VIN" or "+" like the others here.  And they are round.  I haven't seen any that look like this one that do not say "5V" on them so I would steer clear.
  * <img src="/images/s-l500%20(1).jpg" width="200" /> <img src="/images/s-l500.jpg" width="200" />[Half step wth no switch pullup](https://www.ebay.com/itm/2X-Rotary-Encoder-Digital-Potentiometer-20mm-Knurled-Shaft-with-Switch-USA/382606587878?ssPageName=STRK%3AMEBIDX%3AIT&_trksid=p2057872.m2749.l2649) - This one has two issues: the switch doesn't have pullups on it like the others above but the deal breaker is that the rotary encoder outputs half the pulses of the other models.  Each "click" moves both pins to high or both pins to low.  The others move both pins to low and high on every "click."

## Rotary Encoder Testing
From bristolwatch.com, a diagram of what the rotary encoder should do for each "click" (the dashed vertical lines are each a detent that the encoder stops in):

<img src="/images/Encoderth.jpg" width="550" />

I wrote a simple sketch to output the voltage of each pin the encoders are wired to.  It is from some code by [Random Nerd Tutorials](https://randomnerdtutorials.com/esp32-adc-analog-read-arduino-ide/) and adjusted for rotary encoders.  It needs an ESP32 and the encoder needs to have the CLK and DT pins on any of these red-highlighted pins: 

<img src="/images/adc-pins-esp32-f.jpg" width="550" />

Once you upload and run it, the serial monitor will tell you the voltage of each of the two pins.  As you rotate the encoder slowly, you can see what each one does.  Ideally, they should both start high (4095 on ESP32) and as you rotate one will drop to low (0) followed by the second pin, then both will go to high one after the other in the same order as you finish the click.  Rotating slowly the opposite direction should show the leading pin is now the trailing pin going low and high.  The half step encoder shown above only does half of that each click and so it doesn't work reliably with the ClickEncoder library (menu selection will jump down and then back up with each click).

## Building and Wiring
Use a breadboard and jumper wires to make sure everything is wired correctly and test the sketch.  

<img src="/images/20190908_182504.jpg" width="550" />

When you are ready to install it in the enclosure, use jumpers between the VL6180X sensor and the ESP8266, desolder the pins on the TFT and Rotary Encoder and directly solder the jumper wires from the ESP8266 onto each.  I used some hot glue to protect the connections and provide some strain reliefe on them.  

<img src="/images/20190830_080819.jpg" width="550" />

<img src="/images/20190830_080829.jpg" width="550" />

Install the four 3mm nuts in the enclosure.  Lower the assembly from the top and slide the TFT and ESP8266 into their slots and then install the VL6180X to the bottom cover.  I found some hot glue around the edges kept the sensor nice and level when securing it down to the base with screws.  

<img src="/images/20190311_131357.jpg" width="550" />

Screw the bottom cover in place.

<img src="/images/20190311_144628 (1).jpg" width="550" />

Next install the screen's 2.5mm screws and nuts and the rotary encoder to the top cover.  Last, install the top.  The fit is tight so I suggest testing to make sure all the connections are still secure as you do each step.

<img src="/images/20190311_144654 (1).jpg" width="550" />

## Adjusting to Your Needs
The STL is included for the Hornady Powder Hopper as well as the Dillon (untested by me so YMMV) and the original design file is on Onshape if you want to make a copy and adjust the design for another brand of powder hopper: [Onshape Design](https://cad.onshape.com/documents/c187c6855e5b717b6eb50d9e/w/ed8cc32766a24fa2c6d4d2bb/e/9d09fd4bb0b1d438497f7fdc)
