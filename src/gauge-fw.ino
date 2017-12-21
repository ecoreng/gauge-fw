#include <Adafruit_MCP3008.h>
#include "Gauge.h"
#include "Datasource.h"
#include "Display.h"
#include "SSD1306Ascii.h"
#include <Wire.h>
#include <SPI.h>


//define pin connections
#define CS_PIN D8
#define CLOCK_PIN D5
#define MOSI_PIN D7
#define MISO_PIN D6

// instantiate gauge container
CompositeGauge gauge;

// instantiate shared sensor
TestSensor sensor(90, 120, 1);
TestSensor sensor2(90, 120, 1);
TestSensor sensor3(80, 105, 2);

vector<int> sweepLeds1 = {17,16,15,14,13,12,11,10,9,8,7,6};
vector<int> alertLeds1 = {11,10,9,8,7,6};
vector<int> sweepLeds2 = {18,19,20,21,22,23,0,1,2,3,4,5};
vector<int> alertLeds2 = {23,0,1,2,3,4,5};

// ... this is the RGB color of the alert leds
int alertColor[3] = {10,0,0};
// ... this is the RGB color of the level display leds
int sweepColor1[3] = {0,0,1};
int sweepColor2[3] = {0,0,1};
int blankColor[3] = {0,0,0};


FullSweepIlluminationStrategy illumination;
AlertOverlappedSweep sweep1(
  &sensor3,
  31,
  99,
  99,
  sweepColor1,
  alertColor,
  blankColor,
  &sweepLeds1,
  &alertLeds1,
  &illumination
);
AlertOverlappedSweep sweep2(
  &sensor2,
  31,
  99,
  99,
  sweepColor2,
  alertColor,
  blankColor,
  &sweepLeds2,
  &alertLeds2,
  &illumination
);
MultiSweepLEDStrip strip(D4, 24);

// instantiate gauge screen
DualDataSourceScreen screen(&sensor2, &sensor3, 15, 0x3C, &SH1106_128x64, -1);

Adafruit_MCP3008 adc;

readerFunc adcRead = *([adc](char channel) -> int {
  return adc.readADC(channel);
});

void setup() {
  // required for the i2c protocol
  Serial.begin(9600);
  Wire.begin();
  
  adc.begin(CLOCK_PIN, MOSI_PIN, MISO_PIN, CS_PIN);
  
  // gauge assembly time =====================
    
    // Single Sensor, Boost gauge with alert, OLED and Ring ====
      //sensor2.setReader(&adcRead);
      strip.addSweep(&sweep1);
      strip.addSweep(&sweep2);
      
      gauge.add(&sensor);
      gauge.add(&sensor2);
      gauge.add(&sensor3);
      
      // add the ring to the gauge
      gauge.add(&strip);
      
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
