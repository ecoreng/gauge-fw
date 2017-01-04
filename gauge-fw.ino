#include <Adafruit_NeoPixel.h>
#include <ArduinoSTL.h>

#include <Wire.h>
#include <SSD1306Ascii.h>
#include <SSD1306AsciiWire.h>

using namespace std;


/**
 * Abstract Analog Sensor 
 */
class AnalogSensor {
    protected:
        char analogPin;
        int measurement;
        void read () {
            measurement = analogRead(this->analogPin);
        }
    public:
        static const unsigned char V_RESOLUTION_INV = 204; // (1023 / 5)
        AnalogSensor (char pin) {
            this->analogPin = pin;
        }
        int raw(void) {
            return this->measurement;
        }
        virtual String format() = 0;
        virtual String unit() = 0;
};


/**
 * Abstract PressureSensor
 * 
 * Holds physics constants for pressure
 */
class PressureSensor {
  public:
    // divide these 2 by 10
    static const unsigned short ONE_ATM_KPA = 1013;
    static const unsigned char ONE_ATM_PSI = 147;
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
 * MPX4250AP sensor
 * 
 * An analog pressure sensor, which can be composable into a gauge
 */
class MPX4250_Sensor : public PressureSensor, public AnalogSensor, public GaugeComponent {
    protected:
        char percentDegradation;
        
        float toKpaAbs() {
            return (float)this->measurement / 
            ((float)AnalogSensor::V_RESOLUTION_INV) / 
            ((float)MPX4250_Sensor::mV_PER_KPA / 1000) * 
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
        static const unsigned char mV_PER_KPA = 20;
        
        MPX4250_Sensor (char pin, char percentDegradation = 0) : 
          AnalogSensor(pin), 
          GaugeComponent() 
        {
            this->percentDegradation = percentDegradation;
        }

        String format(){
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
        
        String unit(){
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
 * NeoPixelRing
 * 
 * @todo: split this into an abstract NeoPixelRing class and below's task
 * @todo: rename this to SingleSensorAlertingRing
 * 
 * A ring of rgb leds that is aware of an analog sensor measurement, and
 *  illuminates the a certain amount of leds, based on its min and max levels
 *  additionally has "alert leds" which are activated when alertLevel is reached
 */
class NeoPixelRing : public GaugeComponent, public Adafruit_NeoPixel {
  protected:
    AnalogSensor *sensor;
    bool currentlyAlerting = false;
  public:
    vector<int> *sweepLeds;
    vector<int> *alertLeds;
    int minLevel = 0;
    int maxLevel = 1;
    int alertLevel = 2;
    int *baseColor;
    int *alertColor;
        
    NeoPixelRing(
      AnalogSensor *sensor,
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
        this->sensor = sensor;
        this->minLevel = minLevel;
        this->maxLevel = maxLevel;
        this->alertLevel = alertLevel;
        this->baseColor = baseColor;
        this->alertColor = alertColor;
        this->sweepLeds = sweepLeds;
        this->alertLeds = alertLeds;
    }

    void init(void) {
        begin();
        show();  
    }

    bool isAlert() {
      return sensor->raw() > this->alertLevel;
    }
    
    void tick(void) {
      
      // calculate how many leds should be lit, by calculating the ranges
      int relativeLevel = sensor->raw() - this->minLevel;
      int sweepRange = this->maxLevel - this->minLevel;
      float percentileLevel = relativeLevel / (float)sweepRange;
      int howManyLeds = percentileLevel * this->sweepLeds->size() - 1;
      
      int ledKey = 0;
      for (vector<int>::iterator it = this->sweepLeds->begin(); it != this->sweepLeds->end(); ++it) {
        if (ledKey < howManyLeds) {
          setPixelColor(*it, baseColor[0], baseColor[1], baseColor[2]);
        } else {
          setPixelColor(*it, 0, 0, 0);
        }
        ledKey++;
      }

      // check if new reading triggered alert
      if (this->isAlert()) {
       // set state to "alerting"
       this->currentlyAlerting = true;

       // set all alerting leds to the alert color
       for (vector<int>::iterator it = this->alertLeds->begin(); it != this->alertLeds->end(); ++it) {
          setPixelColor(*it, alertColor[0], alertColor[1], alertColor[2]);
       }
      } else {
        // alert threshold not crossed,
        //  now check if we were alerting @ the past tick
        if (currentlyAlerting) {
          // yes.. so we dont need to alert any more, set state to NOT alerting
          this->currentlyAlerting = false;
          
          // turn off all alert leds
          for (vector<int>::iterator it = this->alertLeds->begin(); it != this->alertLeds->end(); ++it) {
            setPixelColor(*it, 0, 0, 0);
          }
        }
      }

      // display all
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
    unsigned char address;
    AnalogSensor *sensor;
    unsigned char measurementX;
    unsigned char measurementY;
    unsigned char unitX;
    unsigned char unitY;
  public:
    Screen(
      unsigned char address,
      AnalogSensor *sensor,
      unsigned char measurementX = 0,
      unsigned char measurementY = 0,
      unsigned char unitY = 0
    ) :
    SSD1306AsciiWire() {
      this->address = address;
      this->sensor = sensor;
      this->measurementX = measurementX;
      this->measurementY = measurementY;
      this->unitY = unitY;
      Wire.begin();
    }

    void init(void){
      begin(&Adafruit128x64, this->address);
      setFont(System5x7);
      set2X();
      clear();
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
MPX4250_Sensor sensor(0, 10);


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
NeoPixelRing ring(
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

// instantiate gauge screen
Screen screen(0x3D, &sensor, 28, 3, 4);


void setup() {

  // gauge assembly time =====================

    // Single Sensor, Boost gauge with alert, OLED and Ring ====
    
      // add the sensor to the gauge
      gauge.add(&sensor);
    
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
