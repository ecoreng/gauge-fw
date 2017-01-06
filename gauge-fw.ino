#include <Adafruit_NeoPixel.h>
#include <ArduinoSTL.h>

#include <Wire.h>
#include <SSD1306Ascii.h>
#include <SSD1306AsciiWire.h>

using namespace std;

/**
 * Interface of a sensor
 */
class Sensor{
  public:
    virtual String format(void) = 0;
    virtual String unit(void) = 0;
    virtual int raw(void) = 0;
};


/**
 * Abstract Analog Sensor 
 */
class AnalogSensor : public Sensor {
  protected:
    char analogPin;
    word measurement;
    void read () {
        measurement = analogRead(this->analogPin);
    }
  public:
    static const byte V_RESOLUTION_INV = 204; // (1023 / 5)
    AnalogSensor (char pin) : Sensor() {
        this->analogPin = pin;
    }
    int raw(void) {
        return this->measurement;
    }
};


/**
 * Abstract PressureSensor
 * 
 * Holds physics constants for pressure
 */
class PressureSensor {
  public:
    // divide these 2 by 10
    static const word ONE_ATM_KPA = 1013;
    static const byte ONE_ATM_PSI = 147;
};


/**
 * GaugeComponent Interface
 * 
 * Defines the contract for composable gauge components
 */
class GaugeComponent{
  public:
    virtual void tick(void) = 0;
    virtual void init(void) = 0;
};


/**
 * Test sensor that goes up to 'maxLevel' then switches direction until it reaches 'minLevel'
 *  then back up again, at a rate of 'speed' per tick
 *  
 *  Useful to simulate sensors with software only ;)
 */
class TestSensor : public Sensor, public GaugeComponent{
  boolean direction = 1; // 1 up; 0 down;
  word measurement = 0;
  word minLevel;
  word maxLevel;
  byte speed;
  public:
    TestSensor(
      word minLevel,
      word maxLevel,
      byte speed
    ) : Sensor(), GaugeComponent(){
      this->minLevel = minLevel;
      this->maxLevel = maxLevel;
      this->speed = speed;
      this->measurement = minLevel;
    }
    
    void tick(void){
      if (this->measurement > this->maxLevel && this->direction == 1) {
        this->direction = 0;
      }
      
      if (this->measurement < (this->minLevel + this->speed) && this->direction == 0) {
        this->direction = 1;
      }
      
      if (this->direction == 1) {
        this->measurement += this->speed;
      } else {
        this->measurement -= this->speed;
      }
    }
    
    void init(void){}
    
    String format(void){
      String concat = " ";
      return concat + (float)measurement / 10;
    };
    
    String unit(void){
      return "unit";
    };
    int raw(void){
      return measurement;  
    };
};


/**
 * MPX4250AP sensor
 * 
 * An analog pressure sensor, which can be composable into a gauge
 */
class MPX4250Sensor : public PressureSensor, public AnalogSensor, public GaugeComponent {
  protected:
    char percentDegradation;
    
    float toKpaAbs() {
      return (float)this->measurement / 
        ((float)AnalogSensor::V_RESOLUTION_INV) / 
        ((float)MPX4250Sensor::mV_PER_KPA / 1000) * 
        (1 + ((float)this->percentDegradation / 100));
    }
    
    float toKpaRel() {
      return toKpaAbs() - ((float)PressureSensor::ONE_ATM_KPA / 10);
    }
    
    float toPsiRel() {
      return 
        this->toKpaRel() * 
        ((float)PressureSensor::ONE_ATM_PSI/10) / 
        ((float)PressureSensor::ONE_ATM_KPA/10) ;
    }

  public:
    static const byte mV_PER_KPA = 20;
    
    MPX4250Sensor (char pin, char percentDegradation = 0) : 
      AnalogSensor(pin), 
      GaugeComponent() 
    {
      this->percentDegradation = percentDegradation;
    }

    String format(void){
      float level = toPsiRel();
      String concat = "";
      if (level > 0) {
        concat = " ";
      }
      if (level > 10) {
        concat += " ";
      }
      return concat + level;
    }
    
    String unit(void){
      return "psi";
    }

    void tick(void) {
      this->read();
    }

    void init(void) {}
};


/**
 * CompositeGauge
 * 
 * A container that takes GaugeComponents, inits them and 
 *  calls tick() on each one at each loop iteration
 *  
 * Always add the sensors before any other component
 */
class CompositeGauge{
  vector<GaugeComponent*> components;
  public:
    CompositeGauge (void){}
    
    void add(GaugeComponent *component){
      this->components.push_back(component);
      component->init();
    }
    
    void init(void) {}
    
    void tick(void) {
      vector<GaugeComponent*>::iterator it;
      for (
        it = this->components.begin();
        it != this->components.end();
        ++it
        ) {
         
        (*it)->tick();
        
      }
    }
};


/**
 * Individually Addressable LED Strip Sweep
 * 
 * Component that defines and operates the sweep of an LED Strip
 *  upon update call, the instance of the LED Strip that needs to
 *  contain this sweep needs to be passed
 */
class IndAddrLEDStripSweep {
  protected:
    Sensor *sensor;
    bool currentlyAlerting = false;
  public:
    vector<int> *sweepLeds;
    vector<int> *alertLeds;
    int minLevel = 0;
    int maxLevel = 1;
    int alertLevel = 2;
    int *baseColor;
    int *alertColor;
    IndAddrLEDStripSweep(
      Sensor *sensor,
      int minLevel,
      int maxLevel,
      int alertLevel,
      int baseColor[3],
      int alertColor[3],
      vector<int> *sweepLeds,
      vector<int> *alertLeds
      ) {
        this->sensor = sensor;
        this->minLevel = minLevel;
        this->maxLevel = maxLevel;
        this->alertLevel = alertLevel;
        this->baseColor = baseColor;
        this->alertColor = alertColor;
        this->sweepLeds = sweepLeds;
        this->alertLeds = alertLeds;
      }

    void update(Adafruit_NeoPixel *ledStrip) {
      // calculate how many leds should be lit, by calculating the ranges
      int relativeLevel = sensor->raw() - this->minLevel;
      int sweepRange = this->maxLevel - this->minLevel;
      float percentileLevel = relativeLevel / (float)sweepRange;
      int howManyLeds = percentileLevel * this->sweepLeds->size() - 1;

      int ledKey = 0;
      for (vector<int>::iterator it = this->sweepLeds->begin(); it != this->sweepLeds->end(); ++it) {
        if (ledKey < howManyLeds) {
          ledStrip->setPixelColor(*it, this->baseColor[0], this->baseColor[1], this->baseColor[2]);
        } else {
          ledStrip->setPixelColor(*it, 0, 0, 0);
        }
        ledKey++;
      }

      // check if new reading triggered alert
      if (this->isAlert()) {
       // set state to "alerting"
       this->currentlyAlerting = true;

       // set all alerting leds to the alert color
       for (vector<int>::iterator it = this->alertLeds->begin(); it != this->alertLeds->end(); ++it) {
          ledStrip->setPixelColor(*it, alertColor[0], alertColor[1], alertColor[2]);
       }
      } else {
        // alert threshold not crossed,
        //  now check if we were alerting @ the past tick
        if (currentlyAlerting) {
          // yes.. so we dont need to alert any more, set state to NOT alerting
          this->currentlyAlerting = false;
          
          // turn off all alert leds
          for (vector<int>::iterator it = this->alertLeds->begin(); it != this->alertLeds->end(); ++it) {
            ledStrip->setPixelColor(*it, 0, 0, 0);
          }
        }
      }
    }

    bool isAlert() {
      return sensor->raw() > this->alertLevel;
    }
};

/**
 * Single Sweep, single sensor LED Strip (aka LED Ring)
 */
class SingleSweepLEDStrip : public GaugeComponent, public Adafruit_NeoPixel {
  protected:
    Sensor *sensor;
    IndAddrLEDStripSweep *sweep;
  public: 
    SingleSweepLEDStrip(
      Sensor *sensor,
      uint16_t dataPin,
      uint8_t totalLeds,
      int minLevel,
      int maxLevel,
      int alertLevel,
      int baseColor[3],
      int alertColor[3],
      vector<int> *sweepLeds,
      vector<int> *alertLeds
      ) : GaugeComponent(),
          Adafruit_NeoPixel(totalLeds, dataPin, NEO_GRB + NEO_KHZ800)
       {
        this->sweep = new IndAddrLEDStripSweep(
          sensor,
          minLevel,
          maxLevel,
          alertLevel,
          baseColor,
          alertColor,
          sweepLeds,
          alertLeds
        );
       }

    void init(void) {
        begin();
        show();
    }
    
    void tick(void) {
      this->sweep->update(this);
      show();
    }
};

/**
 * Dual Sweep, Dual Sensor LED Strip (aka LED Ring)
 */
class DualSweepLEDStrip : public GaugeComponent, public Adafruit_NeoPixel {
  protected:
    IndAddrLEDStripSweep *sweep1;
    IndAddrLEDStripSweep *sweep2;
  public: 
    DualSweepLEDStrip(
      IndAddrLEDStripSweep *sweep1,
      IndAddrLEDStripSweep *sweep2,
      uint16_t dataPin,
      uint8_t totalLeds
      ) : GaugeComponent(),
          Adafruit_NeoPixel(totalLeds, dataPin, NEO_GRB + NEO_KHZ800)
       {
        this->sweep1 = sweep1;
        this->sweep2 = sweep2;
       }

    void init(void) {
        begin();
        show();
    }
    
    void tick(void) {
      this->sweep1->update(this);
      this->sweep2->update(this);
      show();
    }
};


/**
 * Screen
 * 
 * @todo: extract this to Abstract Screen AND SingleSensorAsciiScreen
 * 
 * An AnalogSensor aware screen, with positionable measurement and unit
 */
class Screen : public GaugeComponent, public SSD1306AsciiWire {
    byte address;
    Sensor *sensor;
    byte resetPin;
    byte measurementX;
    byte measurementY;
    byte unitX;
    byte unitY;
  public:
    Screen(
      byte address,
      Sensor *sensor,
      byte resetPin = 4,
      byte measurementX = 0,
      byte measurementY = 0,
      byte unitY = 0
    ) :
    SSD1306AsciiWire() {
      this->address = address;
      this->sensor = sensor;
      this->resetPin = resetPin;
      this->measurementX = measurementX;
      this->measurementY = measurementY;
      this->unitY = unitY;
      Wire.begin();
    }

    void init(void){      
      SSD1306Ascii::reset(this->resetPin);
      begin(&Adafruit128x64, this->address);
      clear();
      setFont(System5x7);
      set2X();
    }
    
    void tick(void) {
      setCol(this->measurementX);
      setRow(this->measurementY);
      print(this->sensor->format());
      set1X();
      setRow(this->unitY);
      print(this->sensor->unit());
      set2X();
      home();
    }
};



/* ***************************
 *  
 *  Runtime starts setup Here
 *  
 ***************************** */

// instantiate gauge container
CompositeGauge gauge;

// instantiate shared sensor
//MPX4250Sensor sensor1(0, 10);
TestSensor sensor2(175,440,20);


/*
// define some variables that we'll later reuse to describe our ring
// ... these leds are available for display of regular level
vector<int> sweepLeds = {15,16,17,18,19,20,21,22,23,0,1,2,3,4,5,6,7};
// ... these leds are alert leds
vector<int> alertLeds = {8,9,10,11,12,13,14};
// ... this is the RGB color of the alert leds
int alertColor[3] = {255,0,0};
// ... this is the RGB color of the level display leds
int sweepColor[3] = {25,8,0};

// instantiate light ring
//NeoPixelRing ring(
SingleSweepLEDStrip ring(
    &sensor,    // sensor
    6,          // data pin
    24,         // total number of leds in the ring
    175,        // minimum displayed level
    410,        // maximum displayed level
    400,        // alert level
    sweepColor, // color of the sweep leds
    alertColor, // color of the alert leds
    &sweepLeds, // ids of the sweep leds
    &alertLeds  // ids of the alert leds
);
*/

//top: 16,17,18,19,20,21,22,23,0,1,2,3
//bot: 15,14,13,12,11,10,9,8,7,6,5,4
vector<int> sweepLeds1 = {17,18,19,20,21,22,23,0,1,2,3};
vector<int> alertLeds1 = {4};
vector<int> sweepLeds2 = {16,15,14,13,12,11,10,9,8,7,6};
vector<int> alertLeds2 = {5};

// ... this is the RGB color of the alert leds
int alertColor[3] = {255,0,0};
// ... this is the RGB color of the level display leds
int sweepColor1[3] = {25,8,0};
int sweepColor2[3] = {0,8,25};

IndAddrLEDStripSweep sweep1(
  &sensor2,
  175,
  410,
  400,
  sweepColor1,
  alertColor,
  &sweepLeds1,
  &alertLeds1
);
IndAddrLEDStripSweep sweep2(
  &sensor2,
  175,
  410,
  400,
  sweepColor2,
  alertColor,
  &sweepLeds2,
  &alertLeds2
);
DualSweepLEDStrip ring(&sweep1, &sweep2, 6, 24);

// instantiate gauge screen
Screen screen(0x3D, &sensor2, 4, 28, 3, 4);


void setup() {
  // gauge assembly time =====================

    // Single Sensor, Boost gauge with alert, OLED and Ring ====
    
      // add the sensor2 to the gauge
      //gauge.add(&sensor1);
      gauge.add(&sensor2);
    
      // add the ring to the gauge
      gauge.add(&ring);
      
      // add the oled screen
      gauge.add(&screen);
      
      // ===================================

  // =========================================
}

void loop() {
  // tick, like in a clock, not like the insect
  gauge.tick();

  delay(100);
}
