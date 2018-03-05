#include "Datasource.h"
#include "SoftwareSerial.h"


using namespace std;

int readAnalog(char location) {
    return analogRead(location);
};

readerFunc analogReader = &readAnalog;


DataSource::DataSource() {};


AnalogSensor::AnalogSensor(char location) : DataSource() {
    this->location = location;
    this->reader = &analogReader;
}

void AnalogSensor::setReader(readerFunc *reader) {
    this->reader = reader;
}

void AnalogSensor::read() {
    measurement = (*reader)(this->location);
}

int AnalogSensor::raw(void) {
    return this->measurement;
}


LiPoBatteryCharge::LiPoBatteryCharge(
    char pin,
    int minLevel,
    int maxLevel
) : AnalogSensor(pin) {
    this->minLevel = minLevel;
    this->maxLevel = maxLevel;
}

long LiPoBatteryCharge::toPercent() {
    return map(
        this->measurement,
        this->minLevel,
        this->maxLevel,
        0,
        100
    );
}

String LiPoBatteryCharge::format(void) {
    long level = toPercent();
    char charBuf[10];
    String formatted = ltoa(level, charBuf, 10);
    return formatted;
}

String LiPoBatteryCharge::unit(void) {
    return "%";
}

void LiPoBatteryCharge::tick(void) {
    read();
}

void LiPoBatteryCharge::init(void) {}


float PressureSensor::correctPressure(float uncorrectedPressure) {
    return uncorrectedPressure - this->barometer->toKpaAbs();
}


AnalogPressureSensor::AnalogPressureSensor(char location, char adcValueOffset, float error) :
  AnalogSensor(location), GaugeComponent() {
    this->error = error;
    this->adcValueOffset = adcValueOffset;
}

float AnalogPressureSensor::toKpaAbs() {
    float localMeasurement = (
        ((float) this->measurement - this->adcValueOffset) /
        ((float) AnalogSensor::V_RESOLUTION_INV) /
        ((float) this->getMilliVoltPerKpa() / 1000) +
        this->getKpaOffset()) *
        (1 + this->error);
    return localMeasurement;
}

float AnalogPressureSensor::toKpaRel() {
    if (this->barometerSet) {
        return correctPressure(toKpaAbs());
    } else  {
        return toKpaAbs() - ((float) PressureSensor::ONE_ATM_KPA / 10);
    }
}

float AnalogPressureSensor::toPsiAbs() {
    return this->toKpaAbs() *
        ((float) PressureSensor::ONE_ATM_PSI / 10) /
        ((float) PressureSensor::ONE_ATM_KPA / 10);
}

float AnalogPressureSensor::toPsiRel() {
    return this->toKpaRel() *
        ((float) PressureSensor::ONE_ATM_PSI / 10) /
        ((float) PressureSensor::ONE_ATM_KPA / 10);
}

void AnalogPressureSensor::init(void) {}

String AnalogPressureSensor::format(void) {
    float level = roundf(toPsiRel() * 10) / 10;
    char charBuf[10];
    String formatted = dtostrf(level, 5, 1, charBuf);
    return formatted;
}

String AnalogPressureSensor::unit(void) {
    return "psi";
}

void AnalogPressureSensor::tick(void) {
    this->read();
}


void PressureSensor::setBarometer(PressureSensor* barometer) {
    this->barometer = barometer;
    this->barometerSet = true;
}


GM3BarMapSensor::GM3BarMapSensor(
    char location,
    byte adcValueOffset,
    float error
    ) : AnalogPressureSensor(location, adcValueOffset, error) {}

GM3BarMapSensor::GM3BarMapSensor(
    char location,
    byte adcValueOffset
    ) : AnalogPressureSensor(location, adcValueOffset, 0.0125) {}

GM3BarMapSensor::GM3BarMapSensor(
    char location
) : AnalogPressureSensor(location, 0, 0.0125) {}

char GM3BarMapSensor::getMilliVoltPerKpa() {
    return this->mV_PER_KPA;
}

char GM3BarMapSensor::getKpaOffset() {
    return this->KPA_OFFSET_AT_ZERO_V;
}


TestSensor::TestSensor(
    word minLevel,
    word maxLevel,
    byte speed
    ) : DataSource(), GaugeComponent() {
    this->minLevel = minLevel;
    this->maxLevel = maxLevel;
    this->speed = speed;
    this->measurement = minLevel;
}

void TestSensor::tick(void) {
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

void TestSensor::init(void) {}

String TestSensor::format(void) {
    float adjusted = ((float) measurement / 10 - 50);
    char charBuf[10];
    String formatted = dtostrf(adjusted, 5, 1, charBuf);
    return formatted;
};

String TestSensor::unit(void) {
    return "unit";
};

int TestSensor::raw(void) {
    return measurement;
};



StaticTestSensor::StaticTestSensor(
    word measurement
    ): DataSource(), GaugeComponent() {
    this->measurement = measurement;
}

void StaticTestSensor::tick(void) {}

void StaticTestSensor::init(void) {}

String StaticTestSensor::format(void) {
    float adjusted = ((float) measurement / 10 - 50);
    char charBuf[10];
    String formatted = dtostrf(adjusted, 5, 1, charBuf);
    return formatted;
};

String StaticTestSensor::unit(void) {
    return "unit";
};

int StaticTestSensor::raw(void) {
    return measurement;
};


BMP280Sensor::BMP280Sensor(BME280* instance)
 : DataSource(), GaugeComponent(), PressureSensor() {
    this->instance = instance;
}

void BMP280Sensor::init(void) {
    this->initialized = true;
}

void BMP280Sensor::read(void) {
    float pressure = this->instance->pres(BME280::PresUnit_Pa);
    this->kpaMeasurement = pressure / 1000;
    this->measurement = (this->kpaMeasurement - this->minKpa) / 
        (this->maxKpa - this->maxKpa) * 1023;
}

void BMP280Sensor::tick(void) {
    if (!this->initialized) {
        this->init();
        if (!this->initialized) {
            return;
        }
    }
    read();
}

String BMP280Sensor::format(void) {
    float level = toKpaAbs();
    char charBuf[10];
    String formatted = dtostrf(level, 5, 1, charBuf);
    return formatted;
}

String BMP280Sensor::unit(void) {
    return "KPa";
}

int BMP280Sensor::raw(void) {
    return this->measurement;
}

float BMP280Sensor::toKpaAbs() {
    return this->kpaMeasurement;
}

void BMP280Sensor::awaitReadyState() {
    while(!this->instance->begin())
    {
        delay(1000);
    }
}


template <class D, class M>
OBD2Source<D, M>::OBD2Source(
    D* obd2Driver,
    String command
) : DataSource() {
    this->obd2Driver = obd2Driver;
    this->command = command;
}

template <class D, class M>
void OBD2Source<D, M>::init(void) {
    this->obd2Driver->init();
}

template <class D, class M>
void OBD2Source<D, M>::tick(void) {
    M measurement = this->obd2Driver->get(this->command);
    if (!measurement.empty) {
        rawValue = measurement.raw;
        value = measurement.value;
        unitValue = measurement.unit;
    }
}

template <class D, class M>
int OBD2Source<D, M>::raw(void) {
    return rawValue;
}

template <class D, class M>
String OBD2Source<D, M>::format(void) {
    return value;
}

template <class D, class M>
String OBD2Source<D, M>::unit(void) {
    return unitValue;
}
