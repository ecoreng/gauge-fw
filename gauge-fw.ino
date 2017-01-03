#include <Adafruit_NeoPixel.h>
#include <ArduinoSTL.h>

using namespace std;

// analog sensor abstract?
class AnalogSensor {
    protected:
        int analogPin;
        int measurement;
        void read () {
            measurement = analogRead(this->analogPin);
        }
    public:
        static const int V_RESOLUTION = 5/1023;
        AnalogSensor (int pin) {
            this->analogPin = pin;
        }
        int raw(void) {
            return this->measurement;
        }
};

class PressureSensor : public AnalogSensor {
  public:
    static const float ONE_ATM_KPA = 101.325;
    static const float ONE_ATM_PSI = 14.6959;
    PressureSensor(int pin) : AnalogSensor(pin) {}
};

// gauge component interface?
class GaugeComponent{
    public:
      virtual void tick(void) = 0;
      virtual void init(void) = 0;
};

// concretion of an analog sensor that is a gauge component,
//  specifically modelled after the MPX4250AP sensor
//  todo: send physics constants to abstract class for a "pressure/boost" sensor
class MPX4250_Sensor : public PressureSensor, public GaugeComponent {
    protected:
        float degradation;
        
        float toKpaAbs() {
            return this->measurement * (AnalogSensor::V_RESOLUTION) / MPX4250_Sensor::SENSOR_KPA_PER_V * (1 + this->degradation);
        }
        
        float toKpaRel() {
            return this->toKpaAbs() - PressureSensor::ONE_ATM_KPA;
        }
        
        float toPsiRel() {
            return this->toKpaRel() / PressureSensor::ONE_ATM_KPA * PressureSensor::ONE_ATM_PSI;
        }
    public:
        static const float SENSOR_KPA_PER_V = 0.02;
        
        MPX4250_Sensor (int pin, float degradation = 0) : PressureSensor(pin), GaugeComponent() {
            this->degradation = degradation;
        }
        
        void tick(void) {
            this->read();
        }

        void init(void) {}
};

// gauge container that accepts multiple "GaugeComponent" elements
//  which we are going to iterate and "tick"
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

// a ring of rgb leds, that is aware of an analog sensor measurement, and
//  illuminates the a certain amount of leds, based on its min and max levels
//  additionally has "alert leds" which are activated when alertLevel is reached
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
    bool inited = false;
    
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


// definition of a global gauge container for our elements
CompositeGauge gauge;


// definition of the type of sensor
MPX4250_Sensor sensor(0, 0.1);


// define some variables that we'll later reuse to describe our ring
// ... these leds are available for display of regular level
vector<int> sweepLeds = {15,16,17,18,19,20,21,22,23,0,1,2,3,4,5,6,7};
// ... these leds are alert leds
vector<int> alertLeds = {8,9,10,11,12,13,14};
// ... this is the RGB color of the alert leds
int alertColor[3] = {255,0,0};
// ... this is the RGB color of the level display leds
int sweepColor[3] = {25,8,0};

// definition of the outer light ring
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


void setup() {
  
  // gauge assembly time =====================

    // Boost gauge with OLED and Ring ====
    
      // add the sensor to the gauge
      gauge.add(&sensor);
    
      // add the ring to the gauge
      gauge.add(&ring);
      
      // todo: add the oled screen
      // gauge.add(&screen);
    
      // initiation routine
      gauge.init();
      
      // ===================================

  // =========================================
}

void loop() {
  // tick, like in a clock, not like the insect
  gauge.tick();

  delay(100);
}
