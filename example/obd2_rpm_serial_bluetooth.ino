#define DELAYLENGTH 1000 // less than 2 secs or it resets
#define SERIALTIMEOUT 1000 // less than 2 secs or it resets
#define V33

#define DEBUG_ELM 0

#include "PID.h"
#include "SoftwareSerial.h"
#include "elm327.h"
#include "elm327.cpp"
#include "datasource.h"
#include "datasource.cpp"
#include "gauge_fw.h"
#include "display.h"


SoftwareSerial softSerial(D8, D7);
ELM327<SoftwareSerial> obd2Driver(&softSerial);
OBD2Source<ELM327<SoftwareSerial>, Measurement> obd2Source(
  &obd2Driver,
  PID_RPM
);
// instantiate gauge container
CompositeGauge gauge;


vector<int> sweepLeds1 = {0,1,2,3,4,5,6};
vector<int> alertLeds1 = {7};
int alertColor[3] = {255,0,0};
int sweepColor1[3] = {2,2,1};
int blankColor[3] = {0,0,0};

SingleSweepLEDStrip strip(
  &obd2Source,
  D5,
  8,
  0,
  200,
  180,
  sweepColor1,
  blankColor,
  alertColor,
  &sweepLeds1,
  &alertLeds1
);

void setup() {
  Serial.begin(9600);
  while(!Serial){}
  
  softSerial.begin(38400);
  delay(500);

  gauge.add(&obd2Source);
  gauge.add(&strip);
}

void loop() {
  gauge.tick();
  delay(150);
}

