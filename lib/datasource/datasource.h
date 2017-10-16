#ifndef DATASOURCE_H
#define DATASOURCE_H
        
#if defined(ESP8266) || defined(ESP32)
 #include <vector>
#else
 #include <ArduinoSTL.h>
#endif
#include "gauge_fw.h"
#include "Arduino.h"
#include <SoftwareSerial.h>
#include "elm327.h"


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
 * Serial Source
 */
class SoftwareSerialSensor : public GaugeComponent, public DataSource {
protected:
    SoftwareSerial *serialConnection;
    String unitName;
    int measurement = 0;
    int baudRate;
public:
    SoftwareSerialSensor(
      SoftwareSerial *serialConnection, 
      String unitName,
      int baudRate
    );
    void read();
    void tick(void);
    void init(void);
    String format(void);
    String unit(void);
    int raw(void);
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
 * Base class for MPX{xxxx} family of pressure sensors
 */
class MPXSensor : public PressureSensor, public AnalogSensor, public GaugeComponent {
protected:
    char adcValueOffset = 0;
    float error = 0;
    float toKpaAbs();
    float toKpaRel();
    float toPsiAbs();
    float toPsiRel();
    
public:
    MPXSensor(char pin, char adcValueOffset = 0, float error = 0);
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
 * MPX4250AP sensor
 * (0-250 kpa [absolute])
 * 
 * An analog pressure sensor, which can be composable into a gauge
 */
class MPX4250Sensor : public MPXSensor {
public:
#ifdef V33
    static const byte mV_PER_KPA = 13;
    static const byte KPA_OFFSET_AT_ZERO_V = 13;
#else
    static const byte mV_PER_KPA = 20;
    static const byte KPA_OFFSET_AT_ZERO_V = 20;
#endif
    
    MPX4250Sensor(char pin, byte adcValueOffset = 0, float error = 0.015);
    char getMilliVoltPerKpa();
    char getKpaOffset();
};


/**
 * MPX5500DP sensor
 * (0-500 kpa [differential])
 * 
 * An analog pressure sensor, which can be composable into a gauge
 */
class MPX5500Sensor : public MPXSensor {
public:
#ifdef V33
    static const byte mV_PER_KPA = 6;
#else
    static const byte mV_PER_KPA = 9;
#endif
    MPX5500Sensor(char pin, byte adcValueOffset = 0, float error = 0.0025);

    char getMilliVoltPerKpa();
    String format(void);
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
