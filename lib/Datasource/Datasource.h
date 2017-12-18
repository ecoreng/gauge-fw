#ifndef DATASOURCE_H
#define DATASOURCE_H
        
#include <vector>
#include <Gauge.h>
#include <SoftwareSerial.h>
#include <elm327.h>


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
 * Base class for MPX{xxxx} family of pressure sensors
 */
class AnalogPressureSensor : public PressureSensor, public AnalogSensor, public GaugeComponent {
protected:
    char adcValueOffset = 0;
    float error = 0;
    float toKpaAbs();
    float toKpaRel();
    float toPsiAbs();
    float toPsiRel();
    
public:
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


template <class D, class M>
class OBD2Source : public DataSource, public GaugeComponent {
protected:
    D *obd2Driver;
    String command;
    String value;
    String unitValue;
    int rawValue;
public:
    OBD2Source(D *obd2Driver, String command);
    void init(void);
    int raw(void);
    String format(void);
    void tick(void);
    String unit(void);    
};

#endif
