// Use 3.3 Volts
#define V33
#define DEBUG_BMP280

#include <Arduino.h>
#include <Adafruit_MCP3008.h>
#include <Wire.h>
#include <SPI.h>
#include <Gauge.h>
#include <Datasource.h>
#include <Display.h>
#include <BME280Spi.h>

//define pin connections
#define CS_PIN D8


// instantiate gauge container
CompositeGauge gauge;

// instantiate test sensor
GM3BarMapSensor boostSensor(0, 9);
AFRSensor wideband(1, 7.35, 22.4);


BME280Spi::Settings settings(D3);
BME280Spi bmp(settings);
BMP280Sensor barometer(&bmp);


Adafruit_MCP3008 adc;

readerFunc adcRead = *([adc](char channel) -> int {
  return adc.readADC(channel);
});


// led indexes for the sweep
vector<int> testSweepLeds = {17,16,15,14,13,12,11,10,9,8,7,6};
vector<int> testAlertLeds = {};
vector<int> boostSweepLeds = {18,19,20,21,22,23,0,1,2,3,4,5};
vector<int> boostAlertLeds = {5};

// colors to use
int boostSweepColor[3] = {1, 5, 2};
int testSweepColor[3] =  {10, 2, 0};
int blankColor[3] = {0, 0, 0};
int alertColor[3] = {255, 0, 0};


// how to illuminate our gauge
LevelOnlyIlluminationStrategy illuminationStrategy(0);
FullSweepIlluminationStrategy fullSweepIlluminationStrategy;

Sweep boostSweep(
  &boostSensor,
  100,
  550,
  540,
  boostSweepColor,
  alertColor,
  blankColor,
  &boostSweepLeds,
  &boostAlertLeds,
  &fullSweepIlluminationStrategy
);
Sweep testSweep(
  &wideband,
  0,
  1024,
  1025,
  testSweepColor,
  alertColor,
  blankColor,
  &testSweepLeds,
  &testAlertLeds,
  &illuminationStrategy
);
MultiSweepLEDStrip ring(D4, 24);


DualDataSourceScreen screen(&boostSensor, &wideband, 15, 0x3C, &SH1106_128x64, -1);


void setup() {
  Serial.begin(9600);

  // required for the i2c & spi protocols
  Wire.begin();  
  SPI.begin();
  bmp.Reset();

  barometer.awaitReadyState();

  // // initialize adc reader
  adc.begin(CS_PIN);

  // // gauge assembly time =====================
  boostSensor.setReader(&adcRead);
  boostSensor.setBarometer(&barometer);
  wideband.setReader(&adcRead);
  gauge.add(&boostSensor);
  gauge.add(&wideband);
  gauge.add(&ring);
  gauge.add(&screen);

  ring.addSweep(&boostSweep);
  ring.addSweep(&testSweep);
  // // =========================================
}


void loop() {
  // tick, like in a clock, not like the insect
  gauge.tick();
  delay(20);
}
