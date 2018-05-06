#ifndef DISPLAY_H
 #define DISPLAY_H

#include <vector>
#include <Wire.h>
#include <SSD1306Ascii.h>
#include <SSD1306AsciiWire.h>
#include <Adafruit_NeoPixel.h>
#include <Gauge.h>
#include <Datasource.h>

using namespace std;

/**
 * Illumination Strategy Interface
 * 
 * Useful because you might want to display the level in different ways
 */
class IlluminationStrategy {
  public:
    virtual int getFirstLedKeyDiff(int previousLevel, int level);
    virtual int *getIlluminationColor(int currentLed, int level, int *baseColor, int *blankColor) = 0;
};


/**
 * Illuminates all LEDs from the left up to the level indicator
 */
class FullSweepIlluminationStrategy : public IlluminationStrategy {
  public:
    FullSweepIlluminationStrategy();
    int getFirstLedKeyDiff(int previousLevel, int level);
    int *getIlluminationColor(int currentLed, int level, int *baseColor, int *blankColor);
};


/**
 * Illuminates all LEDs from the level indicator all the way to the right
 */
class InverseFullSweepIlluminationStrategy : public IlluminationStrategy {
  public:
    InverseFullSweepIlluminationStrategy();
    
    int *getIlluminationColor(int currentLed, int level, int *baseColor, int *blankColor);
};


/**
 * Illuminates only the level LED and `radio` LEDs before and after that
 */
class LevelOnlyIlluminationStrategy : public IlluminationStrategy {
  protected:
    int radio;
  public:
    LevelOnlyIlluminationStrategy(int radio);
    
    int *getIlluminationColor(int currentLed, int level, int *baseColor, int *blankColor);
};
 


/**
 * Individually Addressable LED Strip SWEEP
 * 
 * Component that defines and operates the sweep of an LED Strip
 *  upon update call, the instance of the LED Strip that needs to
 *  contain this sweep needs to be passed
 */
class Sweep {
  protected:
    DataSource *dataSource;
    bool currentlyAlerting = false;
    IlluminationStrategy *strategy;
    int previousLedCount = 0;
  public:
    vector<int> *sweepLeds;
    vector<int> *alertLeds;
    int minLevel = 0;
    int maxLevel = 1;
    int alertLevel = 2;
    int *baseColor;
    int *alertColor;
    int *blankColor;
    Sweep(
      DataSource *dataSource,
      int minLevel,
      int maxLevel,
      int alertLevel,
      int baseColor[3],
      int alertColor[3],
      int blankColor[3],
      vector<int> *sweepLeds,
      vector<int> *alertLeds,
      IlluminationStrategy *strategy
      );

    virtual void update(Adafruit_NeoPixel *ledStrip);
    virtual bool isAlert();
};


class AlertOverlappedSweep : public Sweep {
  public:
    AlertOverlappedSweep(
      DataSource *dataSource,
      int minLevel,
      int maxLevel,
      int alertLevel,
      int baseColor[3],
      int alertColor[3],
      int blankColor[3],
      vector<int> *sweepLeds,
      vector<int> *alertLeds,
      IlluminationStrategy *strategy
    );
    void update(Adafruit_NeoPixel *ledStrip);
};


/**
 * Multi Sweep, multi sensor LED Strip (aka LED Ring)
 */
class MultiSweepLEDStrip : public GaugeComponent, public Adafruit_NeoPixel {
  protected:
    vector<Sweep*> sweeps;
  public: 
    MultiSweepLEDStrip(
      uint16_t dataPin,
      uint8_t totalLeds
    );
    void addSweep(Sweep* sweep);
    void init(void);
    void tick(void);
};



/**
 * ASCII Only OLED Screen
 */
class AsciiOledScreenWire : public SSD1306AsciiWire {
public:
    byte resetPin;
      byte address;
    DevType const *screenType;
    AsciiOledScreenWire(
      byte address,
      DevType const *screenType,
      byte resetPin
    );
    
    void init(void);
};

/**
 * An I2C OLED Screen with dual measurement, one on top, and anotheer on the bottom
 */
class DualDataSourceScreen : public AsciiOledScreenWire, public GaugeComponent {
    DataSource *topDataSource;
    DataSource *bottomDataSource;
    byte measurementX;
    byte topDataSourceY;
    byte bottomDataSourceY;
  public:
    DualDataSourceScreen(
      DataSource *topDataSource,
      DataSource *bottomDataSource,
      byte measurementX,
      byte address,
      DevType const *screenType,
      byte resetPin
    );

    void init(void);
    
    void tick(void);
};


/**
 * SingleDataSourceScreen
 *  
 * A DataSource aware screen, with positionable measurement and unit
 */
class SingleDataSourceScreen : public AsciiOledScreenWire, public GaugeComponent {
    DataSource *dataSource;
    byte measurementX;
    byte measurementY;
    byte unitY;
  public:
    SingleDataSourceScreen(
      DataSource *dataSource,
      byte address,
      DevType const *screenType,
      byte resetPin = 4,
      byte measurementX = 0,
      byte measurementY = 0,
      byte unitY = 0
    );

    void init(void);
    void tick(void);
};

#endif
