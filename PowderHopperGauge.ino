extern "C" {//esp8266 stuff
  #include "user_interface.h"
}

#include <Arduino.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include <VL6180X.h>
#include <Wire.h>
#include <EEPROM.h>
#include <menu.h>
#include <menuIO/TFT_eSPIOut.h>
#include <menuIO/esp8266Out.h> // Needed for esp8266
#include <streamFlow.h>
#include <ClickEncoder.h>
#include <menuIO/clickEncoderIn.h>
#include <menuIO/keyIn.h>
#include <menuIO/chainStream.h>
#include <menuIO/serialIO.h> // For debugging the TFT_eSPIOut.h library
#include <plugin/barField.h>

using namespace Menu;

// Declare pins for rotary encoder
#define encA D3
#define encB D4
#define encBtn D6
#define encSteps 4

// No need to declare pins for 6180 sensor if you use the standard pins on ESP8266
//SDA=4 => D2.
//SCL=5 => D1

// Choose the color scheme
#define Hornady
//#define Dillon
//#define RCBS
//#define Lyman

// Setup TFT colors
#define SCALECOLOR TFT_WHITE
#define BACKCOLOR TFT_BLACK
#define BARCOLOR TFT_GREEN
#define ALERTCOLOR TFT_RED
#define TEXTCOLOR TFT_WHITE

// Bar gragh size and position
int barWidth = 20;
float barHeight = 144.0;
int barX = 100;
int barY = 8;

// Measurement Declarations for Sensor
int rawValue = 20; // Measurement from the sensor in mm
int minDist = 20; // The closest reading to the sensor in mm: 100% full
int maxDist = 120; // The furthest reading from the sensor in mm: 0% full
int lastPercent = 0;
int newPercent;
int alertPercent = 20;
int grainsPerMM = 12;
int grainsLeft = 0;
int grainsAdded = 50; // For measuring grains per mm
int grainsAddedStart = 0;
int grainsAddedFinish = 0;
int alertPercentGrains = 0;

// For saving some variables to EEPROM, only need minDist, maDist, alertPercent, and grainsPerMM
// TODO form into struct: https://github.com/esp8266/Arduino/issues/1053
#define MIN_DIST_ADDR 0
#define MAX_DIST_ADDR MIN_DIST_ADDR + sizeof(int) 
#define ALERT_PERCENT_ADDR MAX_DIST_ADDR + sizeof(int) 
#define GRAINS_PER_MM_ADDR ALERT_PERCENT_ADDR + sizeof(int) 

// Strings for displaying text
String textRawSensorValue = "Distance:";
String textGrainsLeft = "GR Left:";

// Define the number of samples to keep track of. The higher the number, the
// more the readings will be smoothed, but the slower the output will respond to
// the input. Using a constant rather than a normal variable lets us use this
// value to determine the size of the readings array.
// 10 seemed too fast
const int numReadings = 35;

int readings[numReadings];      // the readings from the analog input
int readIndex = 0;              // the index of the current reading
int total = 0;                  // the running total
int average = 0;                // the average

// Function definitions
int checkEEPROM(int ADDR, int value, int min, int max);
void readSensor();
void writeToScreen();
void drawScale();
void drawBar (float nPer);
void writePercentText (int nPer);
void writeSensorValues (int sensorValue);

// Declare the clickencoder
// encBtn, was the third option but made a doubleclick when combined with the keyboard below
ClickEncoder clickEncoder = ClickEncoder(encA,encB,encSteps);
ClickEncoderStream encStream(clickEncoder,1);

// A keyboard with only one key as the encoder button
// Negative pin numbers use internal pull-up, this is on when low
keyMap encBtn_map[]={{-encBtn,defaultNavCodes[enterCmd].ch}};
keyIn<1> encButton(encBtn_map);//1 is the number of keys

//////////////////////////////////////////////////////////

// VL6180 sensor
VL6180X sensor;

// TFT gfx is what the ArduinoMenu TFT_eSPIOut.h is expecting
TFT_eSPI gfx = TFT_eSPI();

//////////////////////////////////////////////////////////
// Start ArduinoMenu
//////////////////////////////////////////////////////////

result saveEEPROMQuit() {
  EEPROM.commit();
  return quit;
}

result rebootESP(){
  delay(1000);
  ESP.restart();
  return proceed;
}

result setGrainsSave() {
  Serial.print("Writing to EEPROM location: ");
  Serial.println(GRAINS_PER_MM_ADDR);
  Serial.print("Writing: ");
  Serial.println(grainsPerMM);
  EEPROM.put(GRAINS_PER_MM_ADDR,grainsPerMM);
  EEPROM.commit();
  return proceed;
}

result alertPercentSave() {
//  grainsAddedFinish = sensor.readRangeSingleMillimeters();
//  grainsPerMM = grainsAdded / (grainsAddedStart - grainsAddedFinish);
  Serial.print("Writing to EEPROM location: ");
  Serial.println(ALERT_PERCENT_ADDR);
  Serial.print("Writing: ");
  Serial.println(alertPercent);
  EEPROM.put(ALERT_PERCENT_ADDR,alertPercent);
  EEPROM.commit();
  return quit;
}

result measureGrainsStart(eventMask e) {
  grainsAddedStart = rawValue;
  return proceed;
}

result measureGrainsAdded(eventMask e) {
  grainsAddedFinish = rawValue;
  grainsPerMM = grainsAdded / (grainsAddedStart - grainsAddedFinish);
  Serial.print("Writing to EEPROM location: ");
  Serial.println(GRAINS_PER_MM_ADDR);
  Serial.print("Writing: ");
  Serial.println(grainsPerMM);
  EEPROM.put(GRAINS_PER_MM_ADDR,grainsPerMM);
  return proceed;
}

result measureMinLevel(eventMask e) {
  maxDist = rawValue;
  Serial.print("Writing to EEPROM location: ");
  Serial.println(MAX_DIST_ADDR);
  Serial.print("Writing maxDist: ");
  Serial.println(maxDist);
  EEPROM.put(MAX_DIST_ADDR,maxDist);
  return proceed;
}

result measureMaxLevel(eventMask e) {
  minDist = rawValue;
  Serial.print("Writing to EEPROM location: ");
  Serial.println(MIN_DIST_ADDR);
  Serial.print("Writing minDist: ");
  Serial.println(minDist);
  EEPROM.put(MIN_DIST_ADDR,minDist);
  return proceed;
}

result showEvent(eventMask e,navNode& nav,prompt& item) {
  Serial.print("event: ");
  Serial.println(e);
  return proceed;
}

// Customizing a menu look by extending the menu class
// This makes a confirmation prompt before rebooting
class confirmReboot:public menu {
public:
  confirmReboot(constMEM menuNodeShadow& shadow):menu(shadow) {}
  Used printTo(navRoot &root,bool sel,menuOut& out, idx_t idx,idx_t len,idx_t p) override {
      if(idx<0){menu::printTo(root,sel,out,idx,len,p);}
      else{
        out.printRaw((constText*)F("Reboot"),len);
      }
      return idx;
  }
};

altMENU(confirmReboot,subMenuReboot,"Reboot?",doNothing,noEvent,wrapStyle,(Menu::_menuData|Menu::_canNav)
  ,OP("Yes",rebootESP,enterEvent)
  ,EXIT("Cancel")
);

MENU(subMenuAlertPercent,"Set Alarm Level",showEvent,noEvent,noStyle 
  ,FIELD(alertPercent,"Alarm Level:","%",0,100,10,1,doNothing,noEvent,noStyle)
  ,FIELD(alertPercentGrains,"Alarm in GR:","GR",0,10000,10,1,doNothing,noEvent,noStyle)
  ,OP("<Save to EEPROM",alertPercentSave,enterEvent)
  ,EXIT("<Back")
);

MENU(subMenuCalibrate,"Calibrate Height",showEvent,noEvent,noStyle
  ,FIELD(rawValue,"Current Dist:","mm",0,255,10,1,NULL,enterEvent,noStyle)
  ,OP("Measure Empty Level",measureMinLevel,enterEvent)
  ,OP("Measure Full Level",measureMaxLevel,enterEvent)
  ,FIELD(maxDist,"Empty Level:","mm",0,255,10,1,doNothing,noEvent,noStyle)
  ,FIELD(minDist,"Full Level:","mm",0,255,10,1,doNothing,noEvent,noStyle)
  ,OP("<Save to EEPROM",saveEEPROMQuit,enterEvent)
  ,EXIT("<Back")
);

MENU(subMenuMeasureGrains,"Measure Grains/mm",showEvent,noEvent,wrapStyle
  ,FIELD(rawValue,"Current Dist:","mm",0,255,10,1,NULL,enterEvent,noStyle)
  ,FIELD(grainsAdded,"Grains to Add:","GR",0,250,10,1,doNothing,noEvent,noStyle)
  ,OP("Measure Low Distance",measureGrainsStart,enterEvent)
  ,OP("Measure High Distance",measureGrainsAdded,enterEvent)
  ,FIELD(grainsAddedStart,"Starting Distance:","mm",0,255,10,1,doNothing,noEvent,noStyle)
  ,FIELD(grainsAddedFinish,"Finish Distance:","mm",0,255,10,1,doNothing,noEvent,noStyle)
  ,OP("<Save to EEPROM",saveEEPROMQuit,enterEvent)
  ,EXIT("<Back")
);

MENU(subMenuSetGrains,"Set Grains/mm",showEvent,noEvent,noStyle
  ,FIELD(grainsPerMM,"Grains/mm:","GR/mm",0,100,10,1,doNothing,noEvent,noStyle)
  ,OP("<Save to EEPROM",setGrainsSave,enterEvent)
  ,EXIT("<Back")
);

MENU(mainMenu,"POWDER LEVEL SENSOR",doNothing,noEvent,wrapStyle
  ,SUBMENU(subMenuCalibrate)
  ,SUBMENU(subMenuAlertPercent)
  ,SUBMENU(subMenuMeasureGrains)
  ,SUBMENU(subMenuSetGrains)
  ,EXIT("Exit Menu")
  ,SUBMENU(subMenuReboot)
);

#define MAX_DEPTH 3

// define menu colors --------------------------------------------------------
#define Black RGB565(0,0,0)
#define Red RGB565(255,0,0)
#define Green RGB565(0,255,0)
#define Blue RGB565(0,0,255)
#define Gray RGB565(128,128,128)
#define LighterRed RGB565(255,150,150)
#define LighterGreen RGB565(150,255,150)
#define LighterBlue RGB565(150,150,255)
#define LighterGray RGB565(211,211,211)
#define DarkerRed RGB565(150,0,0)
#define DarkerGreen RGB565(0,150,0)
#define DarkerBlue RGB565(0,0,150)
#define Cyan RGB565(0,255,255)
#define Magenta RGB565(255,0,255)
#define Yellow RGB565(255,255,0)
#define White RGB565(255,255,255)
#define DarkerOrange RGB565(255,140,0)

#ifdef Hornady
// TFT color table
const colorDef<uint16_t> colors[] MEMMODE={
//{{disabled normal,disabled selected},{enabled normal,enabled selected, enabled editing}}
  {{(uint16_t)Black,(uint16_t)Black}, {(uint16_t)Black, (uint16_t)Red,   (uint16_t)Red}},//bgColor
  {{(uint16_t)White, (uint16_t)White},  {(uint16_t)White, (uint16_t)White, (uint16_t)White}},//fgColor
  {{(uint16_t)Red,(uint16_t)Red}, {(uint16_t)Yellow,(uint16_t)Yellow,(uint16_t)Yellow}},//valColor
  {{(uint16_t)White,(uint16_t)White}, {(uint16_t)White, (uint16_t)White,(uint16_t)White}},//unitColor
  {{(uint16_t)White,(uint16_t)Gray},  {(uint16_t)Black, (uint16_t)Red,  (uint16_t)White}},//cursorColor
  {{(uint16_t)White,(uint16_t)Yellow},{(uint16_t)Black,  (uint16_t)Red,   (uint16_t)Red}},//titleColor
};
#endif

#ifdef Dillon
// TFT color table
const colorDef<uint16_t> colors[] MEMMODE={
//{{disabled normal,disabled selected},{enabled normal,enabled selected, enabled editing}}
  {{(uint16_t)Black,(uint16_t)Black}, {(uint16_t)Black, (uint16_t)Blue,   (uint16_t)Blue}},//bgColor
  {{(uint16_t)White, (uint16_t)White},  {(uint16_t)White, (uint16_t)White, (uint16_t)White}},//fgColor
  {{(uint16_t)Blue,(uint16_t)Blue}, {(uint16_t)Yellow,(uint16_t)Yellow,(uint16_t)Yellow}},//valColor
  {{(uint16_t)White,(uint16_t)White}, {(uint16_t)White, (uint16_t)White,(uint16_t)White}},//unitColor
  {{(uint16_t)White,(uint16_t)Gray},  {(uint16_t)Black, (uint16_t)Blue,  (uint16_t)White}},//cursorColor
  {{(uint16_t)White,(uint16_t)Yellow},{(uint16_t)Black,  (uint16_t)Blue,   (uint16_t)Blue}},//titleColor
};
#endif

#ifdef RCBS
// TFT color table
const colorDef<uint16_t> colors[] MEMMODE={
//{{disabled normal,disabled selected},{enabled normal,enabled selected, enabled editing}}
  {{(uint16_t)Black,(uint16_t)Black}, {(uint16_t)Black, (uint16_t)DarkerGreen,   (uint16_t)DarkerGreen}},//bgColor
  {{(uint16_t)White, (uint16_t)White},  {(uint16_t)White, (uint16_t)White, (uint16_t)White}},//fgColor
  {{(uint16_t)DarkerGreen,(uint16_t)DarkerGreen}, {(uint16_t)Yellow,(uint16_t)Yellow,(uint16_t)Yellow}},//valColor
  {{(uint16_t)White,(uint16_t)White}, {(uint16_t)White, (uint16_t)White,(uint16_t)White}},//unitColor
  {{(uint16_t)White,(uint16_t)Gray},  {(uint16_t)Black, (uint16_t)DarkerGreen,  (uint16_t)White}},//cursorColor
  {{(uint16_t)White,(uint16_t)Yellow},{(uint16_t)Black,  (uint16_t)DarkerGreen,   (uint16_t)DarkerGreen}},//titleColor
};
#endif

#ifdef Lyman
// TFT color table
const colorDef<uint16_t> colors[] MEMMODE={
//{{disabled normal,disabled selected},{enabled normal,enabled selected, enabled editing}}
  {{(uint16_t)Black,(uint16_t)Black}, {(uint16_t)Black, (uint16_t)DarkerOrange,   (uint16_t)DarkerOrange}},//bgColor
  {{(uint16_t)White, (uint16_t)White},  {(uint16_t)White, (uint16_t)White, (uint16_t)White}},//fgColor
  {{(uint16_t)DarkerOrange,(uint16_t)DarkerOrange}, {(uint16_t)Yellow,(uint16_t)Yellow,(uint16_t)Yellow}},//valColor
  {{(uint16_t)White,(uint16_t)White}, {(uint16_t)White, (uint16_t)White,(uint16_t)White}},//unitColor
  {{(uint16_t)White,(uint16_t)Gray},  {(uint16_t)Black, (uint16_t)DarkerOrange,  (uint16_t)White}},//cursorColor
  {{(uint16_t)White,(uint16_t)Yellow},{(uint16_t)Black,  (uint16_t)DarkerOrange,   (uint16_t)DarkerOrange}},//titleColor
};
#endif

// Define the width and height of the TFT and how much of it to take up
#define GFX_WIDTH 128
#define GFX_HEIGHT 160
#define fontW 6
#define fontH 9

constMEM panel panels[] MEMMODE = {{0, 0, GFX_WIDTH / fontW, GFX_HEIGHT / fontH}}; // Main menu panel
//constMEM panel panelsOverlay[] MEMMODE = {{2, 2, GFX_WIDTH / fontW * 0.8, GFX_HEIGHT / fontH * 0.8}}; // Overlay panel
navNode* nodes[sizeof(panels)/sizeof(panel)];//navNodes to store navigation status
panelsList pList(panels, nodes, sizeof(panels)/sizeof(panel)); //a list of panels and nodes
//idx_t tops[MAX_DEPTH]={0,0}; // store cursor positions for each level
idx_t eSpiTops[MAX_DEPTH]={0};
TFT_eSPIOut eSpiOut(gfx,colors,eSpiTops,pList,fontW,fontH+1);
menuOut* constMEM outputs[] MEMMODE={&eSpiOut}; //list of output devices

outputsList out(outputs,1);//outputs list

MENU_INPUTS(in,&encStream,&encButton);

NAVROOT(nav,mainMenu,MAX_DEPTH,in,out);

//when menu is suspended
//result idle(menuOut &o, idleEvent e) {
//  switch(e) {
//    case idleStart:o.println("suspending menu!");o.clear(); break;
//    case idling:o.println("suspended...");o.clear(); break;
//    case idleEnd:o.println("resuming menu.");o.clear(); break;
//  }
//  return proceed;
//}

// esp8266 timer
// From eaxmples at https://github.com/neu-rah/ArduinoMenu/issues/210
os_timer_t myTimer;
void timerCallback(void *) {
  clickEncoder.service();
  }

//////////////////////////////////////////////////////////
// End Arduino Menu
//////////////////////////////////////////////////////////

void setup() {

  Serial.begin(115200);
  Serial.println("Booting");
  delay(3000);
  // commit 512 bytes of ESP8266 flash (for "EEPROM" emulation)
  // this step actually loads the content (512 bytes) of flash into 
  // a 512-byte-array cache in RAM
  EEPROM.begin(512);
  Serial.println("Starting EEPROM");
  // Read values from EEPROM into the variables if they fit in the min-max range
  minDist = checkEEPROM(MIN_DIST_ADDR, minDist, 0, 250);
  maxDist = checkEEPROM(MAX_DIST_ADDR, maxDist, 0, 250);
  alertPercent = checkEEPROM(ALERT_PERCENT_ADDR, alertPercent, 0, 100);
  grainsPerMM = checkEEPROM(GRAINS_PER_MM_ADDR, grainsPerMM, 0, 100);

  // Use this initializer if you're using a 1.8" TFT
  SPI.begin();
  gfx.init(); // Initialize a ST7735S chip
  Serial.println("Initialized ST7735S TFT");
  gfx.fillScreen(TFT_BLACK);
  Serial.println("done");
  delay(1000);

  pinMode(encBtn,INPUT); // Was: INPUT_PULLUP but already has pullup resistor on it
 
//  nav.idleTask=idle;//point a function to be used when menu is suspended
  
  subMenuCalibrate[0].enabled=disabledStatus; // Disables the first item in the subMenuCalibrate
  subMenuCalibrate[3].enabled=disabledStatus; // Disables the third item in the subMenuCalibrate
  subMenuCalibrate[4].enabled=disabledStatus; // Disables the fourth item in the subMenuCalibrate
  subMenuMeasureGrains[0].enabled=disabledStatus; // Disables the first item in the subMenuMeasureGrains
  subMenuMeasureGrains[4].enabled=disabledStatus; // Disables the fourth item in the subMenuMeasureGrains
  subMenuMeasureGrains[5].enabled=disabledStatus; // Disables the fifth item in the subMenuMeasureGrains
  subMenuAlertPercent[1].enabled=disabledStatus; // Disables the second item in the subMenuAlertPercent
  
  nav.showTitle=true; // SHow titles in the menus and submenus
  nav.timeOut = 30;  // Timeout after 10 seconds of inactivity and return to the sensor read screen
  
  // Encoder timer for ESP8266
  os_timer_setfn(&myTimer, timerCallback, NULL); //set the callback funtion
  os_timer_arm(&myTimer, 1, true); //setup the timer tick

  // Initialize the VL6180 sensor
  Wire.begin();
  sensor.init();
  sensor.configureDefault();
  sensor.setTimeout(500);

  // initialize all the readings from the VL6180x to 0
  for (int thisReading = 0; thisReading < numReadings; thisReading++) {
    readings[thisReading] = 0;
  }
}

void loop() {
  // Slow down the menu redraw rate
  constexpr int menuFPS = 1000/30;
  static unsigned long lastMenuFrame =- menuFPS;
  unsigned long now = millis();
  if (now - lastMenuFrame >= menuFPS) {
    lastMenuFrame = millis();
    readSensor(); // Constantly read the sensor in and out of the menu
    nav.poll(); // Poll the input devices
    if (nav.sleepTask) {
      writeToScreen(); // If the menu system is not active, draw the main screen
    }
  }
}

// Check each address in EEPROM if the values fit into the range passed, read them into memory
int checkEEPROM(int ADDR, int value, int min, int max){
  int tempEEPROM = EEPROM.read(ADDR);
  Serial.print("ADDR: ");
  Serial.println(ADDR);
  Serial.print("value: ");
  Serial.println(tempEEPROM);
  if (tempEEPROM >= min && tempEEPROM <= max){
    return tempEEPROM;//EEPROM.get(ADDR, value);
  }
  else{
    return value;
  }
}

// Determine if the bar graph and displayed values need updating
void writeToScreen(){
  if (newPercent != lastPercent){
    writePercentText(newPercent);
    writeSensorValues(rawValue);
    drawBar(newPercent);
  }

  drawScale();  // Set to always draw the scale
}

// Smoothly grab the new reading from the sensor and average to smooth
void readSensor(){
  // subtract the last reading:
  total = total - readings[readIndex];
  // read from the sensor:
  readings[readIndex] = sensor.readRangeSingleMillimeters();
  // add the reading to the total:
  total = total + readings[readIndex];
  // advance to the next position in the array:
  readIndex = readIndex + 1;

  // if we're at the end of the array...
  if (readIndex >= numReadings) {
    // ...wrap around to the beginning:
    readIndex = 0;
  }

  // calculate the average:
  average = total / numReadings;
  // send it to the computer as ASCII digits
  rawValue = average;
//  delay(1);        // delay in between reads for stability


  // Calculate the percent based on the reading and the range of minDist and maxDist
  newPercent = ((maxDist - rawValue) * 100) / (maxDist - minDist);

  // Calculate the alet percent in grains value for the menu screen
  alertPercentGrains = (grainsPerMM * (maxDist - minDist) * alertPercent) / 100;
/*  Serial.println("rawValue: ");
  Serial.println(rawValue);
  Serial.println("newPercent: ");
  Serial.println(newPercent);
  delay(100);
*/
}


// Draw the bar graph outline on the right of the main screen
void drawScale(){  
  gfx.drawFastVLine(barX, barY, barHeight, SCALECOLOR); // Vertical Scale Line Left
  gfx.drawFastVLine(barX + barWidth, barY, barHeight, SCALECOLOR); // Vertical Scale Line Right
  gfx.drawFastHLine(barX, barY, barWidth, SCALECOLOR); // Horizantal Scale Top
  gfx.drawFastHLine(barX, barHeight + barY - 1, barWidth, SCALECOLOR); // Horizantal Scale Bottom, subtract 1 from Y
}


// Draw the bar graph value on the main screen
void drawBar (float nPer){
  // Create a local variable to hold the bar color and change it based on the alert percent
  int GRAPHCOLOR;
  if (nPer <= alertPercent){
    GRAPHCOLOR = ALERTCOLOR;
  }
  else{
    GRAPHCOLOR = BARCOLOR;
  }

  // Check if above 100% and set the bar to 100% to keep it in the scale
  if (nPer > 100.0){
    nPer = 100.0;
  }

  // Check if below 0% and set the bar to 0% to keep it in the scale
  if (nPer < 0.0){
    nPer = 0.0;
  }
  
  // Variable to do the math for the bar height value
  float backBarHeight = barHeight / 100 * (100 - nPer);
  
  
  // Fill the bar with the background color
  gfx.fillRect(barX + 1, barY + 1, barWidth - 1, backBarHeight,  BACKCOLOR);

  // Add a bit of the background color above the bar if the percent is below 100%
  // Keeps a green bar from being stuck above the bar if the value ever went above 100%
  if (nPer <= 100){
    gfx.fillRect(barX + 1, 0, barWidth - 1, barY,  BACKCOLOR);
  }

  // Debugging code for the value of the bar
  /* 
  tft.setTextSize(1);
  tft.setTextColor(TEXTCOLOR, BACKCOLOR); // Make sure the previous text is overwritten
  char charbackBarHeight[5];
  tft.drawString(dtostrf(backBarHeight, 6, 0, charbackBarHeight),10,100);
  */

  // Variables to do the math for the filled bar graph
  float greenBarStart = barY + 1 + backBarHeight;
  float greenBarHeight = ((barHeight - 1) / 100 * nPer) - 1;
  
  gfx.fillRect(barX + 1, greenBarStart, barWidth - 1, greenBarHeight,  GRAPHCOLOR);

  // Debugging code for the value of the bar
  /*
  char chargreenBarStart[5];
  char chargreenBarHeight[5];

  tft.drawString(dtostrf(greenBarStart, 6, 0, chargreenBarStart), 10, 110);
  tft.drawString(dtostrf(greenBarHeight, 6, 0, chargreenBarHeight), 10, 120);
  */

  // Change the value of last percent to new percent to use in the main loop after drawing
//  lastPercent = nPer;
 
}


// Write the main screen large percent value
void writePercentText (int nPer){
  // Move to pointers and char arrays instead of Strings in the future
/*
  char *results_p[2];
  result_p[0] = myNewCombinedArray;
  result_p[1] = anotherArray;
  */
  // Or two dimentional char arrays...
/*
  char results[2][32];
  strcpy(results[0], myNewCombinedArray);
  strcpy(results[1], anotherArray);
  */
  
  char displayPercent[5];
  String paddingSpaces = "";
  String tempPercent = "";
  
  // Keep the percent from dropping to -100% so it doesn't cut off the percent symbol
  if (nPer < 0){
    nPer = 0;
  }
  
  // convert to a string
  String dPercent = String(nPer);
  // Append the percent symbol
  dPercent += "%";

  // Pad the value with leading spaces to keep it right-aligned
  if (nPer < 10){
    paddingSpaces = "  ";
  }
  else if(nPer < 100){
    paddingSpaces = " ";
  }
  else {
    paddingSpaces = "";
  }

  tempPercent = paddingSpaces + dPercent;
   
  // add to an array
  tempPercent.toCharArray(displayPercent, 5);

  gfx.setTextSize(4);
  if (nPer <= alertPercent){ // Write the percent in the alert color or the normal text color
    gfx.setTextColor(ALERTCOLOR, BACKCOLOR); 
  }
  else{
    gfx.setTextColor(TEXTCOLOR, BACKCOLOR); 
  }
  // print out and erase
  gfx.drawString(displayPercent, 2, 50);
  
}

// Display the raw value from the distance sensor with a small label and the grains remaining
void writeSensorValues (int sensorValue){
  // Char arrays for displaying values
  char displayValue[7];
  char displayGrainsLeft[6];
  
  // convert sensor value to a string
  String sValue = String(sensorValue);
  // Append the units and a space to erase the floating 0 when the sensorValue drops from three digits to two
  sValue += "mm ";
  // add to an array
  sValue.toCharArray(displayValue, 7);
  // Write the values to the screen
  gfx.setTextSize(1);
  gfx.setTextColor(TEXTCOLOR, BACKCOLOR); // Make sure the previous text is overwritten
  gfx.drawString(textRawSensorValue, 2, 140);
  gfx.drawString(displayValue, 58, 140);

  // Calculate grains left
  grainsLeft = (maxDist - sensorValue) * grainsPerMM; // New formula
  if (grainsLeft < 0){
    grainsLeft = 0;
  }
 /* // Display the readings for debugging the calculation
  Serial.print("grainsLeft: ");
  Serial.println(grainsLeft);
  Serial.print("grainsPerMM: ");
  Serial.println(grainsPerMM);
  Serial.print("sensorValue: ");
  Serial.println(sensorValue);
  */
  
  // convert to a string
  String gLeft = String(grainsLeft);
  // Append a space to erase the floating 0 when the value drops by a digit
  gLeft += " ";
  // add to an array
  gLeft.toCharArray(displayGrainsLeft, 6);
  // Write the values to the screen
  gfx.drawString(textGrainsLeft, 2, 120);
  gfx.drawString(displayGrainsLeft, 51, 120);

}
