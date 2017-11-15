// Use 3.3 Volts
#include "gauge_fw.h"
#include "datasource.h"
#include "display.h"
#include "SSD1306Ascii.h"
#include <Wire.h>
#include <SPI.h>

#define V33

// instantiate gauge container
CompositeGauge gauge;

// instantiate shared sensor
TestSensor sensor(175,440,11);

// define some variables that we'll later reuse to describe our ring
// ... these leds are available for display of regular level
vector<int> sweepLeds = {15,16,17,18,19,20,21,22,23,0,1,2,3,4,5,6,7,8};
// ... these leds are alert leds
vector<int> alertLeds = {9,10,11,12,13,14};
// ... this is the RGB color of the alert leds
int alertColor[3] = {100,0,0};
// ... this is the RGB color of the level display leds
int sweepColor[3] = {25,8,0};
// ... blank color
int blankColor[3] = {0,0,0};


// instantiate light ring
SingleSweepLEDStrip ring(
    &sensor,    // sensor
    D4,         // data pin
    24,         // total number of leds in the ring
    175,        // minimum displayed level
    410,        // maximum displayed level
    400,        // alert level
    sweepColor, // color of the sweep leds
    blankColor, // blank color for the sweep
    alertColor, // color of the alert leds
    &sweepLeds, // ids of the sweep leds
    &alertLeds  // ids of the alert leds
);


// instantiate gauge screen
//SingleDataSourceScreen screen(0x3D, &Adafruit128x64, &sensor, D4, 15, 2, 4);
SingleDataSourceScreen screen(0x3C, &SH1106_128x64, &sensor, -1, 15, 2, 4);

void setup() {
  // required for the i2c protocol
  Wire.begin();
  
  // gauge assembly time =====================
    
    // Single Sensor, Boost gauge with alert, OLED and Ring ====      
      gauge.add(&sensor);
    
      // add the ring to the gauge
      gauge.add(&ring);
      
      // add the oled screen
      gauge.add(&screen);
      
      // ===================================

  // =========================================
}

void loop() {
  // tick, like in a clock, not like the insect
  gauge.tick();

  delay(0);
}
