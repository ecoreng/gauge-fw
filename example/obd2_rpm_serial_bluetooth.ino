#define DELAYLENGTH 1000 // less than 2 secs or it resets
#define SERIALTIMEOUT 1000 // less than 2 secs or it resets
#define V33

#define DEBUG_ELM 0
#define BUTTON_PIN 2


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
#include "GaugeMultiplexer.h"
#include "Display.h"
#include "Bounce2.h"



SoftwareSerial softSerial(15, 13);
ELM327<SoftwareSerial> obd2Driver(&softSerial);

vector<int> sweepLeds1 = {7,6,5,4,3,2,1,0};
vector<int> alertLeds1 = {7,6,5,4,3,2,1,0};
int alertColor[3] = {10,0,0};
int sweepColor1[3] = {2,2,1};
int sweepColor2[3] = {0,2,2};
int batSweepColor[3] = {0,1,0};
int blankColor[3] = {0,0,0};
int transitionColor[3] = {0,0,1};

FullSweepIlluminationStrategy fullSweepIlluminationStrategy;


// -- Mocked OBD2 sensor --
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
        strcpy(data, " ZZ ZZ 5F  FF      ");
      }
    }
    
    int available() {
      return total - i;
    }
    
    int read() {
      return data[i++];
    }
};

ArrayMockSerial serialMock;
ELM327<ArrayMockSerial> obd(&serialMock);
OBD2Source<ELM327<ArrayMockSerial>, Measurement> tSensor(
  &obd,
  PID_RPM
);
AlertOverlappedSweep tSweep(
  &tSensor,
  0,
  1023,
  1023,
  sweepColor1,
  alertColor,
  blankColor,
  &sweepLeds1,
  &alertLeds1,
  &fullSweepIlluminationStrategy
);
MultiSweepLEDStrip tStrip(12, 24);
CompositeGauge tGauge;


// -- RPM gauge--
OBD2Source<ELM327<SoftwareSerial>, Measurement> rpmObd2Source(
  &obd2Driver,
  PID_RPM
);
AlertOverlappedSweep rpmSweep(
  &rpmObd2Source,
  0,
  220,
  200,
  sweepColor1,
  alertColor,
  blankColor,
  &sweepLeds1,
  &alertLeds1,
  &fullSweepIlluminationStrategy
);
MultiSweepLEDStrip rpmStrip(12, 24);
CompositeGauge rpmGauge;


// -- TP gauge --
OBD2Source<ELM327<SoftwareSerial>, Measurement> tpObd2Source(
  &obd2Driver,
  PID_THROTTLE
);
AlertOverlappedSweep tpSweep(
  &tpObd2Source,
  0,
  1023,
  2000,
  sweepColor2,
  alertColor,
  blankColor,
  &sweepLeds1,
  &alertLeds1,
  &fullSweepIlluminationStrategy
);
MultiSweepLEDStrip tpStrip(12, 24);
CompositeGauge tpGauge;


// -- Battery source --
LiPoBatteryCharge batSource(
  A0,
  580,
  714
);
Sweep batSweep(
  &batSource,
  580,
  714,
  1023,
  batSweepColor,
  alertColor,
  blankColor,
  &sweepLeds1,
  &alertLeds1,
  &fullSweepIlluminationStrategy
);
MultiSweepLEDStrip batStrip(12, 24);
CompositeGauge batGauge;


Bounce debouncer = Bounce(); 
GaugeMultiplexer gMux(
  &debouncer,
  &batStrip,
  transitionColor,
  10
);

void setup() {
  Serial.begin(9600);
  while(!Serial){}
  
  softSerial.begin(38400);

  // rpmStrip.addSweep(&rpmSweep);
  // rpmGauge.add(&rpmObd2Source);
  // rpmGauge.add(&rpmStrip);

  // tpStrip.addSweep(&tpSweep);
  // tpGauge.add(&tpObd2Source);
  // tpGauge.add(&tpStrip);

  batStrip.addSweep(&batSweep);
  batGauge.add(&batSource);
  batGauge.add(&batStrip);

  // // Setup the button with an internal pull-up :
  // pinMode(BUTTON_PIN, INPUT_PULLUP);
  
  // // After setting up the button, setup the Bounce instance :
  // debouncer.attach(BUTTON_PIN);
  // debouncer.interval(150);

  // tStrip.addSweep(&tSweep);
  // tGauge.add(&tSensor);
  // tGauge.add(&tStrip);

  // gMux.addGauge(&batGauge);
  // gMux.addGauge(&rpmGauge);
  // gMux.addGauge(&tpGauge);
  // gMux.addGauge(&tGauge);
}

void loop() {
  // gMux.tick();
  batGauge.tick();
  // tGauge.tick();
  delay(1500);
}
