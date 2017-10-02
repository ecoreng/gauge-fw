#include <Adafruit_MCP3008.h>
#include "gauge_fw.h"
#include "datasource.h"
#include "display.h"
#include "SSD1306Ascii.h"
#include <Wire.h>

// Use 3.3 Volts
#define V33

//define pin connections
#define CS_PIN D8
#define CLOCK_PIN D5
#define MOSI_PIN D7
#define MISO_PIN D6

// instantiate gauge container
CompositeGauge gauge;

// instantiate shared sensor
TestSensor sensor(175,440,11);
// TestSensor sensor2(175,440,20);
MPX5500Sensor sensor2(0, 40);

vector<int> sweepLeds1 = {6,7,8,9,10,11,12,13,14,15,16,17};
vector<int> alertLeds1 = {17};
vector<int> sweepLeds2 = {5,4,3,2,1,0,23,22,21,20,19,18};
vector<int> alertLeds2 = {18};

// ... this is the RGB color of the alert leds
int alertColor[3] = {255,0,0};
// ... this is the RGB color of the level display leds
int sweepColor1[3] = {2,2,1};
int sweepColor2[3] = {8,1,0};
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
  0,
  70,
  55,
  sweepColor2,
  alertColor,
  blankColor,
  &sweepLeds2,
  &alertLeds2,
  &illumination
);
DualSweepLEDStrip ring(&sweep1, &sweep2, D4, 24);


// instantiate gauge screen
DualDataSourceScreen screen(&sensor, &sensor2, 15, 0x3C, &SH1106_128x64, -1);

Adafruit_MCP3008 adc;

readerFunc adcRead = *([adc](char channel) -> int {
  return adc.readADC(channel);
});

void setup() {
  // required for the i2c protocol
  Wire.begin();
  
  adc.begin(CLOCK_PIN, MOSI_PIN, MISO_PIN, CS_PIN);
  
  // gauge assembly time =====================
    
    // Single Sensor, Boost gauge with alert, OLED and Ring ====
      sensor2.setReader(&adcRead);
      
      gauge.add(&sensor);
      gauge.add(&sensor2);
    
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
