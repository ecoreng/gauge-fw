// Use 3.3 Volts
#define V33

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

BME280Spi::Settings settings(D3);
BME280Spi bmp(settings);
BMP280Sensor barometer(&bmp);

SingleDataSourceScreen screen(&testSensor, 15, 0x3C, &SH1106_128x64, -1, 15, 2, 4);

void setup() {
  Serial.begin(9600);
  // required for the i2c & spi protocols
  Wire.begin();
  SPI.begin();
  bmp.Reset();

  barometer.awaitReadyState();


  // // gauge assembly time =====================
  gauge.add(&testSensor);
  gauge.add(&screen);
  // // =========================================
}

void loop() {
  gauge.tick();
  delay(100);
}
