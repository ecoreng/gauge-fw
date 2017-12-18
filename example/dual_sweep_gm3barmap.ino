// Use 3.3 Volts
#define V33

#include <Arduino.h>
#include <Adafruit_MCP3008.h>
#include <Wire.h>
#include <SPI.h>
#include <Gauge.h>
#include <Datasource.h>
#include <Display.h>

//define pin connections
#define CS_PIN D8
#define CLOCK_PIN D5
#define MOSI_PIN D7
#define MISO_PIN D6

// instantiate gauge container
CompositeGauge gauge;

// instantiate test sensor
TestSensor sensor1(175,440,4);
GM3BarMapSensor sensor2(0, 0);

Adafruit_MCP3008 adc;

readerFunc adcRead = *([adc](char channel) -> int {
  return adc.readADC(channel);
});

// led indexes for the sweep
vector<int> sweepLeds1 = {6,7,8,9,10,11,12,13,14,15,16,17};
vector<int> alertLeds1 = {17};
vector<int> sweepLeds2 = {5,4,3,2,1,0,23,22,21,20,19,18};
vector<int> alertLeds2 = {18};

// colors to use
int sweepColor1[3] = {0, 1, 2};
int blankColor1[3] = {0, 0, 0};
int alertColor[3] = {10, 0, 0};
int sweepColor2[3] = {10, 1, 0};
int blankColor2[3] = {1, 2, 1};

// how to illuminate our gauge
LevelOnlyIlluminationStrategy illuminationStrategy(0);
FullSweepIlluminationStrategy fullSweepIlluminationStrategy;

Sweep sweep1(
  &sensor2,
  175,
  410,
  400,
  sweepColor2,
  alertColor,
  blankColor2,
  &sweepLeds1,
  &alertLeds1,
  &fullSweepIlluminationStrategy
);
Sweep sweep2(
  &sensor1,
  175,
  410,
  400,
  sweepColor1,
  alertColor,
  blankColor1,
  &sweepLeds2,
  &alertLeds2,
  &illuminationStrategy
);
MultiSweepLEDStrip ring(D4, 24);

DualDataSourceScreen screen(&sensor2, &sensor1, 15, 0x3C, &SH1106_128x64, -1);

void setup() {
  // required for the i2c protocol
  Wire.begin();

  // initialize adc reader
  adc.begin(CLOCK_PIN, MOSI_PIN, MISO_PIN, CS_PIN);

  // gauge assembly time =====================
      sensor2.setReader(&adcRead);

      gauge.add(&sensor1);
      gauge.add(&sensor2);
      gauge.add(&ring);
      gauge.add(&screen);

      ring.addSweep(&sweep1);
      ring.addSweep(&sweep2);
  // =========================================
}

void loop() {
  // tick, like in a clock, not like the insect
  gauge.tick();
  // delay(10);
}
