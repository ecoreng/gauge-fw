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
  std::vector<GaugeComponent*> comps;
  public:
    CompositeGauge (void){}
    
    void add(GaugeComponent * component){
      this->comps.push_back(component);
    }
    
    void tick(void) {
      for (std::vector<GaugeComponent*>::iterator it = this->comps.begin(); it != this->comps.end(); ++it) {
        (*it)->tick();
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
    int *baseColor;
    int *alertColor;
    
    NeoPixelRing(
      AnalogSensor &sensor,
      int dataPin,
      int minLevel,
      int maxLevel,
      int alertLevel,
      int baseColor[3],
      int alertColor[3],
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
        ring = Adafruit_NeoPixel(totalLeds, dataPin, NEO_GRB + NEO_KHZ800);
        ring.begin();
        ring.show();
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
      // calculate how many leds should be lit, by calculating the ranges
      int relativeLevel = sensor.raw() - this->minLevel;
      int sweepRange = this->maxLevel - this->minLevel;
      float percentileLevel = relativeLevel / (float)sweepRange;
      int howManyLeds = percentileLevel * this->sweepLeds.size() - 1;
      std::vector<int>::iterator it = this->sweepLeds.begin();
      for (int ledKey = 0; ledKey < this->sweepLeds.size(); ledKey++) {
        if (ledKey < howManyLeds) {
          this->ring.setPixelColor(*it, baseColor[0], baseColor[1], baseColor[2]);
        } else {
          this->ring.setPixelColor(*it, 0, 0, 0);
        }
        it++;
      }
      
      // check if new reading triggered alert
      if (this->isAlert()) {
       this->currentlyAlerting = true;
       for (std::vector<int>::iterator it = this->alertLeds.begin(); it != this->alertLeds.end(); ++it) {
          this->ring.setPixelColor(*it, alertColor[0], alertColor[1], alertColor[2]);
       }
      } else {
        if (currentlyAlerting) {
          this->currentlyAlerting = false;
          for (std::vector<int>::iterator it = this->alertLeds.begin(); it != this->alertLeds.end(); ++it) {
            this->ring.setPixelColor(*it, 0, 0, 0);
          }
        }
      }
      this->ring.show();
    }
};


// definition of a gauge container for our elements
CompositeGauge gauge;


void setup() {


  Serial.begin(9600);

  // gauge assembly time =====================

  // Boost gauge with OLED and Ring ====

  // definition of the type of sensor
  MPX4250_Sensor sensor(0, 0.1);
  
  // add the sensor to the geuge
  gauge.add(&sensor);

  // definition of the ring
  // ... these leds are available for display of regular level
  int sweepLeds[] = {15,16,17,18,19,20,21,22,23,0,1,2,3,4,5,6,7};
  // ... these leds are alert leds
  int alertLeds[] = {8,9,10,11,12,13,14};
  // ... this is the RGB color of the alert leds
  int alertColor[] = {255,0,0};
  // ... this is the RGB color of the level display leds
  int sweepColor[] = {255,80,0};

  // add the ring to the gauge
  gauge.add(new NeoPixelRing(sensor, 6, 175, 410, 400, sweepColor, alertColor, sweepLeds, alertLeds, 24));

  // todo: add the oled screen
  //gauge.add(&screen);

  // ===================================

  // =========================================
}

void loop() {
  Serial.write("tick\n");
 
  // tick, like in a clock, not like the insect
  gauge.tick();

  delay(500);
  
}
