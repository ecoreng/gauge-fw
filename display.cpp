#include "display.h"
#include <Wire.h>

FullSweepIlluminationStrategy::FullSweepIlluminationStrategy() :
    IlluminationStrategy() {}
    
int* FullSweepIlluminationStrategy::getIlluminationColor(int currentLed, int level, int *baseColor, int *blankColor) {
  return currentLed <= level ? baseColor : blankColor;
}



InverseFullSweepIlluminationStrategy::InverseFullSweepIlluminationStrategy() : IlluminationStrategy() {}

int* InverseFullSweepIlluminationStrategy::getIlluminationColor(int currentLed, int level, int *baseColor, int *blankColor) {
  return currentLed >= level ? baseColor : blankColor;
}



LevelOnlyIlluminationStrategy::LevelOnlyIlluminationStrategy(int radio) : IlluminationStrategy() {
  this->radio = radio;
}

int* LevelOnlyIlluminationStrategy::getIlluminationColor(int currentLed, int level, int *baseColor, int *blankColor) {      
  if (currentLed == level) {
    return baseColor;
  }

  if (currentLed >= (level - radio) && currentLed <= (level + radio)) {
    int adjustedIColor[] = {baseColor[0] / 3, baseColor[1] / 3, baseColor[2] / 3};

    // for some reason this needs to be here
    //  to return the right color for all instances
    delay(0);

    return adjustedIColor;
  }

  return blankColor;
}

 

IndAddrLEDStripSweep::IndAddrLEDStripSweep(
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
) {
    this->dataSource = dataSource;
    this->minLevel = minLevel;
    this->maxLevel = maxLevel;
    this->alertLevel = alertLevel;

    this->baseColor = baseColor;
    this->alertColor = alertColor;
    this->blankColor = blankColor;

    this->sweepLeds = sweepLeds;
    this->alertLeds = alertLeds;
    this->strategy = strategy;
}

void IndAddrLEDStripSweep::update(Adafruit_NeoPixel *ledStrip) {
  // calculate how many leds should be lit, by calculating the ranges
  int relativeLevel = dataSource->raw() - this->minLevel;
  int sweepRange = this->maxLevel - this->minLevel;
  float percentileLevel = relativeLevel / (float)sweepRange;
  int howManyLeds = percentileLevel * this->sweepLeds->size() - 1;

  int ledKey = 0;
  for (vector<int>::iterator it = this->sweepLeds->begin(); it != this->sweepLeds->end(); ++it) {
    int *color = this->strategy->getIlluminationColor(ledKey, howManyLeds, this->baseColor, this->blankColor);        
    ledStrip->setPixelColor(*it, color[0], color[1], color[2]);
    ledKey++;
  }

  // check if new reading triggered alert
  if (this->isAlert()) {
   // set state to "alerting"
   this->currentlyAlerting = true;

   // set all alerting leds to the alert color
   for (vector<int>::iterator it = this->alertLeds->begin(); it != this->alertLeds->end(); ++it) {
      ledStrip->setPixelColor(*it, alertColor[0], alertColor[1], alertColor[2]);
   }
  } else {
    // alert threshold not crossed,
    //  now check if we were alerting @ the past tick
    if (currentlyAlerting) {
      // yes.. so we dont need to alert any more, set state to NOT alerting
      this->currentlyAlerting = false;

      // turn off all alert leds
      for (vector<int>::iterator it = this->alertLeds->begin(); it != this->alertLeds->end(); ++it) {
        ledStrip->setPixelColor(*it, 0, 0, 0);
      }
    }
  }
}

bool IndAddrLEDStripSweep::isAlert() {
  return dataSource->raw() > this->alertLevel;
}



SingleSweepLEDStrip::SingleSweepLEDStrip(
  DataSource *dataSource,
  uint16_t dataPin,
  uint8_t totalLeds,
  int minLevel,
  int maxLevel,
  int alertLevel,
  int baseColor[3],
  int blankColor[3],
  int alertColor[3],
  vector<int> *sweepLeds,
  vector<int> *alertLeds
  ) : GaugeComponent(), Adafruit_NeoPixel(totalLeds, dataPin, NEO_GRB + NEO_KHZ800)
   {
    this->sweep = new IndAddrLEDStripSweep(
      dataSource,
      minLevel,
      maxLevel,
      alertLevel,
      baseColor,
      blankColor,
      alertColor,
      sweepLeds,
      alertLeds,
      new FullSweepIlluminationStrategy()
    );
}

void SingleSweepLEDStrip::init(void) {
    begin();
    show();
}
    
void SingleSweepLEDStrip::tick(void) {
  this->sweep->update(this);
  show();
}



DualSweepLEDStrip::DualSweepLEDStrip(
  IndAddrLEDStripSweep *sweep1,
  IndAddrLEDStripSweep *sweep2,
  uint16_t dataPin,
  uint8_t totalLeds
) : GaugeComponent(), Adafruit_NeoPixel(totalLeds, dataPin, NEO_GRB + NEO_KHZ800)
   {
    this->sweep1 = sweep1;
    this->sweep2 = sweep2;
}

void DualSweepLEDStrip::init(void) {
    begin();
    show();
}
    
void DualSweepLEDStrip::tick(void) {
    this->sweep1->update(this);
    this->sweep2->update(this);
    show();
}



I2CScreen::I2CScreen() {
    // how to check if wire has already been started? 
    //if (TWCR == 0) {
    //    Wire.begin();
    //}
}
    
void I2CScreen::init(void) {}



AsciiOledScreen::AsciiOledScreen(
    byte address,
    DevType const *screenType,
    byte resetPin
    ) : SSD1306AsciiWire(), I2CScreen() {
    this->address = address;
    this->resetPin = resetPin;
    this->screenType = screenType;
}
    
void AsciiOledScreen::init(void){
    I2CScreen::init();
    SSD1306Ascii::reset(this->resetPin);
    begin(this->screenType, this->address);
    clear();
    setFont(X11fixed7x14B);
}



DualDataSourceScreen::DualDataSourceScreen(
  DataSource *topDataSource,
  DataSource *bottomDataSource,
  byte measurementX,
  byte address,
  DevType const *screenType,
  byte resetPin
) : AsciiOledScreen(address, screenType, resetPin) {
  this->topDataSource = topDataSource;
  this->bottomDataSource = bottomDataSource;
  this->measurementX = measurementX;
  this->topDataSourceY = (((float)screenType->lcdHeight / 3) - 20) / 7;
  this->bottomDataSourceY = (((float)screenType->lcdHeight * 2 / 3) - 8 ) / 7;
}

void DualDataSourceScreen::init(void){
  AsciiOledScreen::init();
}
    
void DualDataSourceScreen::tick(void) {
  set2X();
  setCol(this->measurementX);
  setRow(this->topDataSourceY);
  print(this->topDataSource->format());
  setFont(font5x7);
  set1X();
  setRow(this->topDataSourceY + 2);
  print(this->topDataSource->unit());
  setFont(X11fixed7x14B);

  set2X();
  setCol(this->measurementX);
  setRow(this->bottomDataSourceY);
  print(this->bottomDataSource->format());
  setFont(font5x7);
  set1X();
  setRow(this->bottomDataSourceY + 2);
  print(this->bottomDataSource->unit());
  setFont(X11fixed7x14B);
  home();
}



SingleDataSourceScreen::SingleDataSourceScreen(
  byte address,
  DevType const *screenType,
  DataSource *dataSource,
  byte resetPin,
  byte measurementX,
  byte measurementY,
  byte unitY
) : AsciiOledScreen(address, screenType, resetPin) {
  this->dataSource = dataSource;
  this->measurementX = measurementX;
  this->measurementY = measurementY;
  this->unitY = unitY;
}

void SingleDataSourceScreen::init(void){
    AsciiOledScreen::init();
    set2X();
}
    
void SingleDataSourceScreen::tick(void) {
  setCol(this->measurementX);
  setRow(this->measurementY);
  print(this->dataSource->format());
  setFont(font5x7);
  set1X();
  setRow(this->unitY);
  print(this->dataSource->unit());
  setFont(X11fixed7x14B);
  set2X();
  home();
}
