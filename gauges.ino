#include <Adafruit_SSD1306.h>
#include <Adafruit_NeoPixel.h>
#include <Adafruit_GFX.h>
#include <gfxfont.h>
#include <ArduinoSTL.h>

// analog sensor abstract?
class AnalogSensor {
    protected:
        int analogPin;
        int measurement;
        int read () {
            this->measurement = analogRead(this->analogPin);
        }
    public:
        AnalogSensor (int pin) {
            this->analogPin = pin;
        }
        int raw(void) {
            return this->measurement;
        }
        virtual String format ();
        virtual String getUnit();
};

// gauge component interface?
class GaugeComponent{
    public:
      virtual void tick(void) = 0;
};

// concretion of an analog sensor that is a gauge component,
//  specifically modelled after the MPX4250AP sensor
//  todo: send physics constants to abstract class for a "pressure/boost" sensor
class MPX4250_Sensor : public AnalogSensor, public GaugeComponent {
    protected:
        float degradation;
        
        float toKpaAbs() {
            return this->measurement / 1023.0 * 5 / MPX4250_Sensor::SENSOR_KPA_PER_V * (1 + this->degradation);
        }
        
        float toKpaRel() {
            return this->toKpaAbs() - MPX4250_Sensor::ATM_KPA;
        }
        
        float toPsiRel() {
            return this->toKpaRel() / MPX4250_Sensor::ATM_KPA * MPX4250_Sensor::ATM_PSI;
        }
    public:
        static const float SENSOR_KPA_PER_V = 0.02;
        static const float ATM_KPA = 101.325;
        static const float ATM_PSI = 14.6959;
        
        MPX4250_Sensor (int pin, float degradation = 0) : AnalogSensor(pin), GaugeComponent() {
            this->degradation = degradation;
        }
        
        String format () {
            String concat = "";
            if (this->measurement > 0) {
              concat = " ";
            }
            return concat + this->toPsiRel();
        }
        
        String getUnit() {
            // needs to be switchable
            return "Psi";
        }
        
        void tick(void) {
            this->read();
        }
};

// gauge container that accepts multiple "GaugeComponent" elements
//  which we are going to iterate and "tick"
class CompositeGauge{
  GaugeComponent *components[] = {};
  int totalComponents;
  public:
    CompositeGauge (void){}
    
    void add(GaugeComponent * component){
      this->components[this->totalComponents++] = component;
    }
    
    void tick(void) {
      for(int i = 0; i < totalComponents; i++){
          this->components[i]->tick();
        }
    }
};

// a ring of rgb leds, that is aware of an analog sensor measurement, and
//  illuminates the a certain amount of leds, based on its min and max levels
//  additionally has "alert leds" which are activated when alertLevel is reached
class NeoPixelRing : public GaugeComponent {
  protected:
    AnalogSensor &sensor;
    Adafruit_NeoPixel ring;
    bool currentlyAlerting = false;
  public:
    std::vector<int> sweepLeds;
    std::vector<int> alertLeds;
    int minLevel = 0;
    int maxLevel = 1;
    int alertLevel = 2;
    int dataPin = 4;
    int baseColor;
    int alertColor;
    
    NeoPixelRing(
      AnalogSensor &sensor,
      int dataPin,
      int minLevel,
      int maxLevel,
      int alertLevel,
      int *baseColor,
      int *alertColor,
      int *sweepLeds,
      int *alertLeds,
      int totalLeds
      ) : GaugeComponent() {
        this->sensor = sensor;
        this->dataPin = dataPin;
        this->minLevel = minLevel;
        this->maxLevel = maxLevel;
        this->baseColor = baseColor;
        this->alertColor = alertColor;
        this->alertLevel = alertLevel;
        this->setSweep(sweepLeds);
        this->setAlert(alertLeds);
        
        // init ring
        this->ring = Adafruit_NeoPixel(totalLeds, dataPin, NEO_GRB + NEO_KHZ800);
        this->ring.begin();
        this->ring.show();
    }
    
    bool isAlert() {
      return sensor.raw() > this->alertLevel;
    }

    void setSweep(int leds[]) {
      this->sweepLeds.insert(this->sweepLeds.begin(), leds);
    }

    void setAlert(int leds[]) {
      this->alertLeds.insert(this->alertLeds.begin(), leds);
    }
    
    void tick(void) {
      // get reading from sensor
      // calculate how many leds should be lit, by calculating the ranges
      // check if new reading triggered alert
    }
};

// definition of the type of sensor
MPX4250_Sensor sensor(0);

// definition of the ring
// ... these leds are available for display of regular level
int sweepLeds[] = {15,16,17,18,19,20,21,22,23,0,1,2,3,4,5,6,7};
// ... these leds are alert leds
int alertLeds[] = {8,9,10,11,12,13,14};
// ... this is the RGB color of the alert leds
int alertColor[] = {255,0,0};
// ... this is the RGB color of the level display leds
int sweepColor[] = {255,80,0};
// instantiate this ring
NeoPixelRing ring(sensor, 4, 1, 2, 3, sweepColor, alertColor, sweepLeds, alertLeds, 24);

// definition of a gauge container for our elements
CompositeGauge gauge;


void setup() {
  // gauge assembly time =====================

  // Boost gauge with OLED and Ring ====
  // add the sensor to the geuge
  gauge.add(&sensor);

  // add the ring to the gauge
  gauge.add(&ring);

  // todo: add the oled screen
  // gauge.add(&screen);

  // ===================================

  // =========================================
}

void loop() {
 
  // tick, like in a clock, not like the insect
  gauge.tick();
  
}
