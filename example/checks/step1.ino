#include <Arduino.h>
#include <Adafruit_MCP3008.h>
#include <Wire.h>
#include <SPI.h>
#include <Gauge.h>
#include <Datasource.h>
#include <Display.h>
#include <BME280Spi.h>

void setup() {
  Serial.begin(9600);
}

void loop() {
  Serial.println("I'm Alive!");
}
