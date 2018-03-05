#ifndef GAUGEMUX_H
 #define GAUGEMUX_H

#include <vector>
#include "Gauge.h"
#include <Adafruit_NeoPixel.h>
#include <Bounce2.h>

using namespace std;

class GaugeMultiplexer {
    int displayedGauge;
    int currentTransitionTicks;
    int maxTransitionTicks;
    Bounce* debounce;
    Adafruit_NeoPixel* strip;
    vector<CompositeGauge*> gauges;
    int* transitionColor;
public:
    GaugeMultiplexer(
        Bounce* debounce,
        Adafruit_NeoPixel* strip,
        int transitionColor[3],
        int maxTransitionTicks = 10
    );
    void addGauge(CompositeGauge* gauge);
    void setDisplayedGauge(int displayGauge);
    void displayNextGauge(void);
    void doGaugeTransition(void);
    void init(void);
    void tick(void);
};

#endif