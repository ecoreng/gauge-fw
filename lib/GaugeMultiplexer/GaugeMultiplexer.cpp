#include "Gauge.h"
#include "GaugeMultiplexer.h"
#include <Adafruit_NeoPixel.h>
#include <Bounce2.h>


GaugeMultiplexer::GaugeMultiplexer(
    Bounce* debounce,
    Adafruit_NeoPixel* strip,
    int transitionColor[3],
    int maxTransitionTicks
    ) {
    this->displayedGauge = 0;
    this->maxTransitionTicks = maxTransitionTicks;
    this->debounce = debounce;
    this->strip = strip;
    this->transitionColor = transitionColor;
}

void GaugeMultiplexer::setDisplayedGauge(int displayGauge) {
    this->displayedGauge = displayGauge;
}

void GaugeMultiplexer::addGauge(CompositeGauge* gauge) {
    this->gauges.push_back(gauge);
    gauge->init();
}

void GaugeMultiplexer::init(void) {

}

void GaugeMultiplexer::displayNextGauge(void) {
    int totalGauges = this->gauges.size();
    this->currentTransitionTicks = 0;
    if (this->displayedGauge < totalGauges - 1) {
        this->setDisplayedGauge(this->displayedGauge + 1);
    } else {
        this->setDisplayedGauge(0);
    }
}

void GaugeMultiplexer::tick(void) {
    this->debounce->update();
    if (this->debounce->fell()) {
        this->displayNextGauge();
        return;
    }
    if (this->currentTransitionTicks < this->maxTransitionTicks) {
        this->doGaugeTransition();
        this->currentTransitionTicks++;
        return;
    }
    CompositeGauge* displayedGauge = this->gauges.at(this->displayedGauge);
    displayedGauge->tick();
}

void GaugeMultiplexer::doGaugeTransition(void) {
    for(int i = 0; i < this->strip->numPixels(); i++) {
      this->strip->setPixelColor(i, 0, 0, 0);
    }
    this->strip->setPixelColor(
        this->displayedGauge,
        this->transitionColor[0],
        this->transitionColor[1],
        this->transitionColor[2]
    );
    this->strip->show();
}