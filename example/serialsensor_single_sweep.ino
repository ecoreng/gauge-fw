#include "gauge_fw.h"
#include "datasource.h"
#include "display.h"
#include <SoftwareSerial.h>

// Use 3.3 Volts
#define V33

SoftwareSerial btSerial(D8, D7); // RX, TX

// instantiate gauge container
CompositeGauge gauge;
SoftwareSerialSensor serialSensor(&btSerial, "", 9600);

vector<int> sweepLeds1 = {0,1,2,3,4,5,6};
vector<int> alertLeds1 = {7};
int alertColor[3] = {255,0,0};
int sweepColor1[3] = {2,2,1};
int blankColor[3] = {0,0,0};

SingleSweepLEDStrip strip(
  &serialSensor,
  D5,
  8,
  0,
  1023,
  900,
  sweepColor1,
  blankColor,
  alertColor,
  &sweepLeds1,
  &alertLeds1
);


void setup() {
  Serial.begin(9600);

  gauge.add(&serialSensor);
  gauge.add(&strip);
}

void loop() {
  gauge.tick();
  delay(0);
}

