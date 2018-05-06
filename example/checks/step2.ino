#include <Arduino.h>
#include <Adafruit_MCP3008.h>
#include <Wire.h>
#include <SPI.h>
#include <Gauge.h>
#include <Datasource.h>
#include <Display.h>
#include <BME280Spi.h>


BME280Spi::Settings settings(D3);
BME280Spi bmp(settings);
BMP280Sensor barometer(&bmp);


void setup() {
  Serial.begin(9600);
  
  SPI.begin();
  bmp.Reset();

  barometer.awaitReadyState();
}

void loop() {
  barometer.tick();
  Serial.println("I'm Alive!");
  delay(1000);
  Serial.print("and the absolute pressure in KPA is:");
  Serial.println(barometer.format());
  delay(1000);
}
