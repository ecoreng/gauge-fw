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
TestSensor testSensor(175,440,4);
GM3BarMapSensor boostSensor(0, 0, 0.035);

BME280Spi::Settings settings(D3);
BME280Spi bmp(settings);
BMP280Sensor barometer(&bmp);

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
int sweepColor2[3] =  {10, 1, 0};
int blankColor2[3] = {1, 2, 1};

// how to illuminate our gauge
LevelOnlyIlluminationStrategy illuminationStrategy(0);
FullSweepIlluminationStrategy fullSweepIlluminationStrategy;

Sweep sweep1(
  &boostSensor,
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
  &testSensor,
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

DualDataSourceScreen screen(&boostSensor, &testSensor, 15, 0x3C, &SH1106_128x64, -1);

void setup() {
  Serial.begin(9600);
  // required for the i2c & spi protocols
  Wire.begin();  
  SPI.begin();
  bmp.Reset();

  barometer.awaitReadyState();

  // initialize adc reader
  adc.begin(CS_PIN);

  // gauge assembly time =====================
  boostSensor.setReader(&adcRead);
  boostSensor.setBarometer(&barometer);
  gauge.add(&barometer);
  gauge.add(&testSensor);
  gauge.add(&boostSensor);
  gauge.add(&ring);
  gauge.add(&screen);

  ring.addSweep(&sweep1);
  ring.addSweep(&sweep2);
  // =========================================
}

void loop() {
  // tick, like in a clock, not like the insect
  gauge.tick();
  delay(1);
}
