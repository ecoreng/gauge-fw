// Use 3.3 Volts
#define V33

#include "gauge_fw.h"
#include "datasource.h"
#include "display.h"
#include "SSD1306Ascii.h"
#include <Wire.h>

int readAnalog2(char location) {
    return analogRead(location);
};
readerFunc analogReader2 = &readAnalog2;


// instantiate gauge container
CompositeGauge gauge;

// instantiate shared sensor
TestSensor sensor(175,440,11);
TestSensor sensor2(175,440,20);


/*
// define some variables that we'll later reuse to describe our ring
// ... these leds are available for display of regular level
vector<int> sweepLeds = {20,21,22,23,0,1,2,3,4,5,6,7,8,9,10,11,12};
// ... these leds are alert leds
vector<int> alertLeds = {13,14,15,16,17,18,19};
// ... this is the RGB color of the alert leds
int alertColor[3] = {255,0,0};
// ... this is the RGB color of the level display leds
int sweepColor[3] = {25,8,0};
// ... blank color
int blankColor[3] = {1,1,1};


// instantiate light ring
SingleSweepLEDStrip ring(
    &sensor,    // sensor
    D6,          // data pin
    24,         // total number of leds in the ring
    175,        // minimum displayed level
    410,        // maximum displayed level
    400,        // alert level
    sweepColor, // color of the sweep leds
    alertColor, // color of the alert leds
    blankColor, // blank color for the sweep
    &sweepLeds, // ids of the sweep leds
    &alertLeds  // ids of the alert leds
);

// define some variables that we'll later reuse to describe our ring
// ... these leds are available for display of regular level
vector<int> sweepLeds2 = {3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19};
// ... these leds are alert leds
vector<int> alertLeds2 = {20,21,22,23,0,1,2};

SingleSweepLEDStrip ring2(
    &sensor2,    // sensor
    D7,          // data pin
    24,         // total number of leds in the ring
    175,        // minimum displayed level
    410,        // maximum displayed level
    400,        // alert level
    sweepColor, // color of the sweep leds
    alertColor, // color of the alert leds
    blankColor, // blank color for the sweep
    &sweepLeds2, // ids of the sweep leds
    &alertLeds2  // ids of the alert leds
);
*/


vector<int> sweepLeds1 = {6,7,8,9,10,11,12,13,14,15,16,17};
vector<int> alertLeds1 = {};
vector<int> sweepLeds2 = {5,4,3,2,1,0,23,22,21,20,19,18};
vector<int> alertLeds2 = {};


// ... this is the RGB color of the alert leds
int alertColor[3] = {255,0,0};
// ... this is the RGB color of the level display leds
int sweepColor1[3] = {25,8,0};
int sweepColor2[3] = {0,8,25};
int blankColor[3] = {0,0,0};

//LevelOnlyIlluminationStrategy illumination(0);
FullSweepIlluminationStrategy illumination;
IndAddrLEDStripSweep sweep1(
  &sensor,
  175,
  410,
  400,
  sweepColor1,
  alertColor,
  blankColor,
  &sweepLeds1,
  &alertLeds1,
  &illumination
);
IndAddrLEDStripSweep sweep2(
  &sensor2,
  175,
  410,
  400,
  sweepColor2,
  alertColor,
  blankColor,
  &sweepLeds2,
  &alertLeds2,
  &illumination
);
DualSweepLEDStrip ring2(&sweep1, &sweep2, D7, 24);
DualSweepLEDStrip ring(&sweep1, &sweep2, D6, 24);


// instantiate gauge screen
//SingleDataSourceScreen screen(0x3D, &Adafruit128x64, &sensor, D4, 15, 2, 4);
//SingleDataSourceScreen screen2(0x3C, &SH1106_128x64, &sensor2, -1, 15, 2, 4);
DualDataSourceScreen screen2(&sensor, &sensor2, 15, 0x3C, &SH1106_128x64, -1);
DualDataSourceScreen screen(&sensor2, &sensor, 15, 0x3D, &Adafruit128x64, -1);

void setup() {
  // required for the i2c protocol
  Wire.begin();
  
  // gauge assembly time =====================
    
    // Single Sensor, Boost gauge with alert, OLED and Ring ====
      sensor.setReader(&analogReader2);
      
      gauge.add(&sensor);
      gauge.add(&sensor2);
    
      // add the ring to the gauge
      gauge.add(&ring);
      gauge.add(&ring2);
      
      // add the oled screen
      gauge.add(&screen);
      gauge.add(&screen2);
      
      // ===================================

  // =========================================
}

void loop() {
  // tick, like in a clock, not like the insect
  gauge.tick();

  delay(0);
}
