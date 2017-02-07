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
      float adjusted = ((float)measurement / 10 - 50);
      char charBuf[10];
      String formatted = dtostrf(adjusted, 5, 1, charBuf);     
      return formatted;
    };
    
    String unit(void){
      return "unit";
    };
    int raw(void){
      return measurement;  
    };
};

/**
 * Base class for MPX{xxxx} family of pressure sensors
 */
class MPXSensor : public PressureSensor, public AnalogSensor, public GaugeComponent {
  protected:
    char percentDegradation;
    
    float toKpaAbs() {
      return (float)this->measurement / 
        ((float)AnalogSensor::V_RESOLUTION_INV) / 
        ((float)this->mV_PER_KPA / 1000) * 
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
    static const byte mV_PER_KPA = 0;  
    MPXSensor (char pin, char percentDegradation = 0) : 
      AnalogSensor(pin), 
      GaugeComponent() 
    {
      this->percentDegradation = percentDegradation;
    }

    String format(void){
      float level = toPsiRel();
      char charBuf[10];
      String formatted = dtostrf(level, 5, 1, charBuf);     
      return formatted;
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
 * MPX4250AP sensor
 * (0-250 kpa [absolute])
 * 
 * An analog pressure sensor, which can be composable into a gauge
 */
class MPX4250Sensor : public MPXSensor {
  public:
    static const byte mV_PER_KPA = 20;
    MPX4250Sensor (char pin, char percentDegradation = 0) : 
      MPXSensor(pin, percentDegradation) {}
};

/**
 * MPX5500DP sensor
 * (0-500 kpa [differential])
 * 
 * An analog pressure sensor, which can be composable into a gauge
 */
class MPX5500Sensor : public MPXSensor {
  public:
    static const byte mV_PER_KPA = 9;
    MPX5500Sensor (char pin, char percentDegradation = 0) : 
      MPXSensor(pin, percentDegradation) {}
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
 * Illumination Strategy Interface
 * 
 * Useful because you might want to display the level in different ways
 */
class IlluminationStrategy {
  public:
    virtual int *getIlluminationColor(int currentLed, int level, int *baseColor, int *blankColor) = 0;
};

/**
 * Illuminates all LEDs from the left up to the level indicator
 */
class FullSweepIlluminationStrategy : public IlluminationStrategy {
  public:
    FullSweepIlluminationStrategy() : IlluminationStrategy() {}
    
    int *getIlluminationColor(int currentLed, int level, int *baseColor, int *blankColor) {
      return currentLed <= level ? baseColor : blankColor;
    }
};

/**
 * Illuminates all LEDs from the level indicator all the way to the right
 */
class InverseFullSweepIlluminationStrategy : public IlluminationStrategy {
  public:
    InverseFullSweepIlluminationStrategy() : IlluminationStrategy() {}
    
    int *getIlluminationColor(int currentLed, int level, int *baseColor, int *blankColor) {
      return currentLed >= level ? baseColor : blankColor;
    }
};

/**
 * Illuminates only the level LED and `radio` LEDs before and after that
 */
class LevelOnlyIlluminationStrategy : public IlluminationStrategy {
  protected:
    int radio;
  public:
    LevelOnlyIlluminationStrategy(
      int radio
    ) : IlluminationStrategy() {
      this->radio = radio;
    }
    
    int *getIlluminationColor(int currentLed, int level, int *baseColor, int *blankColor) {      
      if (currentLed == level) {
        return baseColor;
      }
      
      if (currentLed >= (level - radio) && currentLed <= (level + radio)) {
        int adjustedIColor[] = {baseColor[0] / 3, baseColor[1] / 3, baseColor[2] / 3};

        // for some reason this needs to be here
        //  to return the right color for all instances
        delay(0);
        
        return adjustedIColor;
      }
      
      return blankColor;
    }
};
 


/**
 * Individually Addressable LED Strip SWEEP
 * 
 * Component that defines and operates the sweep of an LED Strip
 *  upon update call, the instance of the LED Strip that needs to
 *  contain this sweep needs to be passed
 */
class IndAddrLEDStripSweep {
  protected:
    Sensor *sensor;
    bool currentlyAlerting = false;
    IlluminationStrategy *strategy;
  public:
    vector<int> *sweepLeds;
    vector<int> *alertLeds;
    int minLevel = 0;
    int maxLevel = 1;
    int alertLevel = 2;
    int *baseColor;
    int *alertColor;
    int *blankColor;
    IndAddrLEDStripSweep(
      Sensor *sensor,
      int minLevel,
      int maxLevel,
      int alertLevel,
      int baseColor[3],
      int alertColor[3],
      int blankColor[3],
      vector<int> *sweepLeds,
      vector<int> *alertLeds,
      IlluminationStrategy *strategy
      ) {
        this->sensor = sensor;
        this->minLevel = minLevel;
        this->maxLevel = maxLevel;
        this->alertLevel = alertLevel;

        this->baseColor = baseColor;
        this->alertColor = alertColor;
        this->blankColor = blankColor;
        
        this->sweepLeds = sweepLeds;
        this->alertLeds = alertLeds;
        this->strategy = strategy;
      }

    void update(Adafruit_NeoPixel *ledStrip) {
      // calculate how many leds should be lit, by calculating the ranges
      int relativeLevel = sensor->raw() - this->minLevel;
      int sweepRange = this->maxLevel - this->minLevel;
      float percentileLevel = relativeLevel / (float)sweepRange;
      int howManyLeds = percentileLevel * this->sweepLeds->size() - 1;

      int ledKey = 0;
      for (vector<int>::iterator it = this->sweepLeds->begin(); it != this->sweepLeds->end(); ++it) {
        int *color = this->strategy->getIlluminationColor(ledKey, howManyLeds, this->baseColor, this->blankColor);        
        ledStrip->setPixelColor(*it, color[0], color[1], color[2]);
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
      int blankColor[3],
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
          blankColor,
          alertColor,
          sweepLeds,
          alertLeds,
          new FullSweepIlluminationStrategy()
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
 * An ssd1306 controlled screen that can only display ascii chars and shows 2 sensor 
 * measurements at the same time, one on the top half, and the other on the bottom half
 */
class DualSensorScreen : public GaugeComponent, public SSD1306AsciiWire {
    byte address;
    Sensor *topSensor;
    Sensor *bottomSensor;
    byte resetPin;
    byte measurementX;
    DevType *screenType;
    byte topSensorY;
    byte bottomSensorY;
  public:
    DualSensorScreen(
      byte address,
      DevType *screenType,
      Sensor *topSensor,
      Sensor *bottomSensor,
      byte resetPin = 4,
      byte measurementX = 0
    ) :
    SSD1306AsciiWire() {
      this->address = address;
      this->topSensor = topSensor;
      this->bottomSensor = bottomSensor;
      this->resetPin = resetPin;
      this->measurementX = measurementX;
      this->screenType = screenType;
      this->topSensorY = (((float)this->screenType->lcdHeight / 3) - 20) / 7;
      this->bottomSensorY = (((float)this->screenType->lcdHeight * 2 / 3) - 8 ) / 7;
      Wire.begin();
    }

    void init(void){
      SSD1306Ascii::reset(this->resetPin);
      begin(this->screenType, this->address);
      clear();
      setFont(X11fixed7x14B);
      set2X();
    }
    
    void tick(void) {
      setCol(this->measurementX);
      setRow(this->topSensorY);
      print(this->topSensor->format());
      set1X();
      setRow(this->topSensorY + 1);
      print(this->topSensor->unit());
      set2X();

      setCol(this->measurementX);
      setRow(this->bottomSensorY);
      print(this->bottomSensor->format());
      set1X();
      setRow(this->bottomSensorY + 1);
      print(this->bottomSensor->unit());
      set2X();
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
    DevType *screenType;
    Sensor *sensor;
    byte resetPin;
    byte measurementX;
    byte measurementY;
    byte unitY;
  public:
    Screen(
      byte address,
      DevType *screenType,
      Sensor *sensor,
      byte resetPin = 4,
      byte measurementX = 0,
      byte measurementY = 0,
      byte unitY = 0
    ) :
    SSD1306AsciiWire() {
      this->address = address;
      this->screenType = screenType;
      this->sensor = sensor;
      this->resetPin = resetPin;
      this->measurementX = measurementX;
      this->measurementY = measurementY;
      this->unitY = unitY;
      Wire.begin();
    }

    void init(void){      
      SSD1306Ascii::reset(this->resetPin);
      begin(this->screenType, this->address);
      clear();
      setFont(X11fixed7x14B);
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
MPX4250Sensor sensor1(0, 10);
TestSensor sensor2(175,440,11);


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
    &sensor2,    // sensor
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

vector<int> sweepLeds1 = {22,23, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
vector<int> alertLeds1 = {10};
vector<int> sweepLeds2 = {21,20,19,18,17,16,15,14,13,12,11};
vector<int> alertLeds2 = {11};

// ... this is the RGB color of the alert leds
int alertColor[3] = {255,0,0};
// ... this is the RGB color of the level display leds
int sweepColor1[3] = {100,3,0};
int sweepColor2[3] = {0,8,25};
int blankColor[3] = {0,0,0};

LevelOnlyIlluminationStrategy illumination(0);
FullSweepIlluminationStrategy fullSweepIllumination;
//InverseFullSweepIlluminationStrategy illumination;
IndAddrLEDStripSweep sweep1(
  &sensor1,
  175,
  410,
  400,
  sweepColor1,
  alertColor,
  blankColor,
  &sweepLeds1,
  &alertLeds1,
  &fullSweepIllumination
);
IndAddrLEDStripSweep sweep2(
  &sensor2,
  175,
  410,
  400,
  sweepColor2,
  alertColor,
  blankColor,
  &sweepLeds2,
  &alertLeds2,
  &illumination
);
DualSweepLEDStrip ring(&sweep1, &sweep2, 6, 24);


// instantiate gauge screen
//Screen screen(0x3D, &Adafruit128x64, &sensor2, 4, 15, 3, 4);
//Screen screen(0x3C, &Adafruit128x32, &sensor2, 4, 15, 0, 1);
DualSensorScreen screen(0x3D, &Adafruit128x64, &sensor1, &sensor2, 4, 15);

void setup() {
  // gauge assembly time =====================

    // Single Sensor, Boost gauge with alert, OLED and Ring ====
    
      // add the sensor2 to the gauge
      gauge.add(&sensor1);
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

  delay(1);
}
