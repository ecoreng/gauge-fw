#ifndef DATASOURCE_H
#define DATASOURCE_H
        
#include <vector>
#include <Gauge.h>
#include <SoftwareSerial.h>
#include <elm327.h>
#include <BME280.h>


using namespace std;

/**
 * The reader function for the data source
 */
typedef int (*readerFunc)(char);

/**
 * Default analog input reader for arduino
 */
int readAnalog(char location);

/**
 * Abstract DataSource
 */
class DataSource {
public:
    DataSource();

    // initialization routines
    virtual void init(void) = 0;

    // return the raw measurement (usually 0-1023)
    virtual int raw(void) = 0;

    // return the real measurement
    virtual String format(void) = 0;

    // return the unit of the real measurement
    virtual String unit(void) = 0;
};


/**
 * Abstract Analog Sensor 
 */
class AnalogSensor : public DataSource {
protected:
    char location;
    word measurement;
    readerFunc *reader;    
    void read();
public:
#ifdef V33
    static const word V_RESOLUTION_INV = 310; // ~(1024 / 3.3)
#else
    static const byte V_RESOLUTION_INV = 204; // ~(1024 / 5)
#endif
    AnalogSensor(char location);
    int raw(void);
    void setReader(readerFunc *reader);
};

/**
 * Li Po battery charge level data source
 */
class LiPoBatteryCharge : public AnalogSensor, public GaugeComponent {
protected:
    long toPercent();
    int minLevel;
    int maxLevel;
public:
    LiPoBatteryCharge(
        char pin,
        int minLevel,
        int maxLevel
    );
    String format(void);
    String unit(void);
    void tick(void);
    void init(void);
};


/**
 * Barometer interface
 */
class Barometer {
public:
    virtual void tick(void) = 0;
    virtual float toKpaAbs() = 0;
};


/**
 * Abstract PressureSensor
 * 
 * Holds physics constants for pressure
 */
class PressureSensor {
public:
    Barometer* barometer;
    bool barometerSet = false;
    // divide these 2 by 10
    static const word ONE_ATM_KPA = 1013;
    static const byte ONE_ATM_PSI = 147;
    void setBarometer(Barometer* barometer);
    virtual float toKpaAbs() = 0;
    float correctPressure(float uncorrectedPressure);
};


/**
 * Base class for analog pressure sensors
 */
class AnalogPressureSensor : public PressureSensor, public AnalogSensor, public GaugeComponent, public Barometer {
protected:
    char adcValueOffset = 0;
    float error = 0;
public:
    float toKpaAbs();
    float toKpaRel();
    float toPsiAbs();
    float toPsiRel();
    AnalogPressureSensor(char pin, char adcValueOffset = 0, float error = 0);
    String format(void);
    String unit(void);
    void tick(void);
    void init(void);
    virtual char getMilliVoltPerKpa() = 0;
    virtual char getKpaOffset() {
        return 0;
    };
};


/**
 * GM 3 Bar Map Sensor
 * (0-3 bar - 0-300 kpa - 0-44.1psi [absolute])
 * 
 * An analog pressure sensor, which can be composed into a gauge
 */
class GM3BarMapSensor : public AnalogPressureSensor {
public:
    static const byte mV_PER_KPA = 16;
    static const byte KPA_OFFSET_AT_ZERO_V = 4;

    GM3BarMapSensor(char location);    
    GM3BarMapSensor(char location, byte adcValueOffset);
    GM3BarMapSensor(char location, byte adcValueOffset, float error);
    char getMilliVoltPerKpa();
    char getKpaOffset();
};


/**
 * Test sensor that goes up to 'maxLevel' then switches direction until it reaches 'minLevel'
 *  then back up again, at a rate of 'speed' per tick
 *  
 *  Useful to simulate sensors with software only ;)
 */
class TestSensor : public DataSource, public GaugeComponent {
    boolean direction = 1; // 1 up; 0 down;
    word measurement = 0;
    word minLevel;
    word maxLevel;
    byte speed;
public:
    TestSensor(word minLevel, word maxLevel, byte speed);
    void read();
    void tick(void);
    void init(void);
    String format(void);
    String unit(void);
    int raw(void);
};


/**
 * Static test sensor
 */
class StaticTestSensor : public DataSource, public GaugeComponent {
    word measurement = 0;
public:
    StaticTestSensor(word measurement);
    void read();
    void tick(void);
    void init(void);
    String format(void);
    String unit(void);
    int raw(void);
};


/**
 * BMP280 sensor (temp, barometric pressure)
 */
class BMP280Sensor : public DataSource, public GaugeComponent, public PressureSensor, public Barometer {
    BME280* instance;
    word measurement = 0;
    float kpaMeasurement = 0;
    float minKpa = 67.727;
    float maxKpa = 135.455;
    bool initialized = false;
public:
    BMP280Sensor(BME280* instance);
    void read();
    void tick(void);
    void init(void);
    String format(void);
    String unit(void);
    int raw(void);
    float toKpaAbs();
    void awaitReadyState();
};


template <class D, class M>
class OBD2Source : public DataSource, public GaugeComponent {
protected:
    D *obd2Driver;
    String command;
    String value = "";
    String unitValue = "";
    int rawValue = 0;
public:
    OBD2Source(D *obd2Driver, String command);
    void init(void);
    int raw(void);
    String format(void);
    void tick(void);
    String unit(void);    
};

#endif
