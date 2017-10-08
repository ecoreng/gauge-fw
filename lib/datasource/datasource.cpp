#include "datasource.h"
#include <SoftwareSerial.h>

using namespace std;

int readAnalog(char location) {
    return analogRead(location);
};

readerFunc analogReader = &readAnalog;



DataSource::DataSource() {};

void DataSource::setReader(readerFunc *reader) {
    this->reader = reader;
};



AnalogSensor::AnalogSensor(char location) : DataSource() {
    this->location = location;
    this->reader = &analogReader;
}

void AnalogSensor::read() {
    measurement = (*reader)(this->location);
}

int AnalogSensor::raw(void) {
    return this->measurement;
}



SoftwareSerialSensor::SoftwareSerialSensor(
  SoftwareSerial *serialConnection,
  String unitName = "",
  int baudRate = 9600
  ) : GaugeComponent(), DataSource() {
    this->serialConnection = serialConnection;
    this->unitName = unitName;
    this->baudRate = baudRate;
}

String SoftwareSerialSensor::format(void) {
  float adjusted = (measurement);
  char charBuf[10];
  String formatted = dtostrf(adjusted, 5, 1, charBuf);
  return formatted;
}

String SoftwareSerialSensor::unit(void) {
  return unitName;
}

void SoftwareSerialSensor::tick(void) {
  this->read();
}

int SoftwareSerialSensor::raw(void) {
  return measurement;
};

void SoftwareSerialSensor::read() {
  // this is not the right way of handling this
  //  we should create a buffer, and read until we find \n
  //  then, use that as a value
  // Doing it this way actually gets cutoff mid value :(
  if (this->serialConnection->available()) {
    String preMeasurement = this->serialConnection->readStringUntil('\n');
    measurement = preMeasurement.toInt();
  }
}
void SoftwareSerialSensor::init(void) {
  this->serialConnection->begin(baudRate);
  delay(1000);
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

void TestSensor::read(void) {}

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



MPXSensor::MPXSensor(char pin, char adcValueOffset, float error) :
  AnalogSensor(pin), GaugeComponent() {
    this->error = error;
    this->adcValueOffset = adcValueOffset;
}

float MPXSensor::toKpaAbs() {
    float localMeasurement = (
        ((float) this->measurement - this->adcValueOffset) /
        ((float) AnalogSensor::V_RESOLUTION_INV) /
        ((float) this->getMilliVoltPerKpa() / 1000) +
        this->getKpaOffset()) *
        (1 + this->error);
    return localMeasurement;
}

float MPXSensor::toKpaRel() {
    return toKpaAbs() - ((float) PressureSensor::ONE_ATM_KPA / 10);
}

float MPXSensor::toPsiAbs() {
    return this->toKpaAbs() *
        ((float) PressureSensor::ONE_ATM_PSI / 10) /
        ((float) PressureSensor::ONE_ATM_KPA / 10);
}

float MPXSensor::toPsiRel() {
    return this->toKpaRel() *
        ((float) PressureSensor::ONE_ATM_PSI / 10) /
        ((float) PressureSensor::ONE_ATM_KPA / 10);
}

void MPXSensor::init(void) {}

String MPXSensor::format(void) {
    float level = toPsiRel();
    char charBuf[10];
    String formatted = dtostrf(level, 5, 1, charBuf);
    return formatted;
}

String MPXSensor::unit(void) {
    return "psi";
}

void MPXSensor::tick(void) {
    this->read();
}



MPX4250Sensor::MPX4250Sensor(
    char pin,
    byte adcValueOffset,
    float error
    ) : MPXSensor(pin, adcValueOffset, error) {}

char MPX4250Sensor::getMilliVoltPerKpa() {
    return this->mV_PER_KPA;
}

char MPX4250Sensor::getKpaOffset() {
    return this->KPA_OFFSET_AT_ZERO_V;
}



MPX5500Sensor::MPX5500Sensor(
    char pin,
    byte adcValueOffset,
    float error
    ) : MPXSensor(pin, adcValueOffset, error) {}

char MPX5500Sensor::getMilliVoltPerKpa() {
    return this->mV_PER_KPA;
}

String MPX5500Sensor::format(void) {
    float level = toPsiAbs();
    char charBuf[10];
    String formatted = dtostrf(level, 5, 1, charBuf);
    return formatted;
}

