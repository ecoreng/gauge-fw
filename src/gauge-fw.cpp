#include "Gauge.h"
#include "Datasource.h"
#include "Display.h"
#include "SSD1306Ascii.h"
#include <Wire.h>
#include <SPI.h>
#include <Arduino.h>

// Use 3.3 Volts
#define V33

// instantiate gauge container
CompositeGauge gauge;

// instantiate shared sensor
TestSensor sensor(175,440,20);

// define some variables that we'll later reuse to describe our ring
// ... these leds are available for display of regular level
vector<int> sweepLeds = {15,16,17,18,19,20,21,22,23,0,1,2,3,4,5,6,7,8};
// ... these leds are alert leds
vector<int> alertLeds = {9,10,11,12,13,14};
// ... this is the RGB color of the alert leds
int alertColor[3] = {100,0,0};
// ... this is the RGB color of the level display leds
int sweepColor[3] = {10,10,5};
// ... blank color
int blankColor[3] = {0,0,0};

// Defines how we want to show the level of the gauge in the sweep
FullSweepIlluminationStrategy strategy;

Sweep sweep(
    &sensor,
    175,
    410,
    400,
    sweepColor,
    alertColor,
    blankColor,
    &sweepLeds,
    &alertLeds,
    &strategy
);
MultiSweepLEDStrip strip(
  D6,
  24
);
SingleDataSourceScreen screen(&sensor, 0x3C, &SH1106_128x64, -1, 15, 2, 4);


void setup() {
  // required to start i2c
  Wire.begin();

  // LED strip adds a "sweep"
  strip.addSweep(&sweep);
  // gauge adds a data source
  gauge.add(&sensor);
  // gauge adds an LED strip
  gauge.add(&strip);      
  // gauge adds a screen
  gauge.add(&screen);
}

void loop() {
  // clock ticks
  gauge.tick();
}
