#define DELAYLENGTH 1000 // less than 2 secs or it resets
#define SERIALTIMEOUT 1000 // less than 2 secs or it resets
#define V33

#define DEBUG_ELM 0

#include "Arduino.h"
#include "PID.h"
#include "Wire.h"
#include "SPI.h"
#include "SoftwareSerial.h"
#include "elm327.h"
#include "elm327.cpp"
#include "Datasource.h"
#include "Datasource.cpp"
#include "Gauge.h"
#include "Display.h"


class ArrayMockSerial : public MockSerial {
  char data[20];
  int total = 20;
  int i = 0;

  public:
    void println(String payload) {
      i = 0;
      if (payload == "ATI") {
        strcpy(data, " ELM327            ");
      }
      else if (payload == PID_RPM) {
        strcpy(data, " ZZ ZZ 1F  FF      ");
      }
    }
    
    int available() {
      return total - i;
    }
    
    int read() {
      return data[i++];
    }
};

// ArrayMockSerial mockSerial;
// ELM327<ArrayMockSerial> obd2Driver(&mockSerial);
// OBD2Source<ELM327<ArrayMockSerial>, Measurement> rpmObd2Source(
//   &obd2Driver,
//   PID_RPM
// );

SoftwareSerial softSerial(15, 13);
ELM327<SoftwareSerial> obd2Driver(&softSerial);
OBD2Source<ELM327<SoftwareSerial>, Measurement> rpmObd2Source(
  &obd2Driver,
  PID_RPM
);

// instantiate gauge container
CompositeGauge gauge;


vector<int> sweepLeds1 = {0,1,2,3,4,5,6,7};
vector<int> alertLeds1 = {0,1,2,3,4,5,6,7};
int alertColor[3] = {255,0,0};
int sweepColor1[3] = {2,2,1};
int blankColor[3] = {0,0,0};

FullSweepIlluminationStrategy fullSweepIlluminationStrategy;

Sweep sweep1(
  &rpmObd2Source,
  31,
  99,
  99,
  sweepColor1,
  alertColor,
  blankColor,
  &sweepLeds1,
  &alertLeds1,
  &fullSweepIlluminationStrategy
);
MultiSweepLEDStrip strip(12, 24);

void setup() {
  Serial.begin(9600);
  while(!Serial){}
  
  softSerial.begin(38400);
  strip.addSweep(&sweep1);

  gauge.add(&rpmObd2Source);
  gauge.add(&strip);
}

void loop() {
  gauge.tick();
  delay(150);
}
