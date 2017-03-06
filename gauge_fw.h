#ifndef GAUGEFW_H
 #define GAUGEFW_H

#ifdef ESP8266 || defined(ESP32)
 #include <vector>
#else
 #include <ArduinoSTL>
#endif

using namespace std;

/**
 * GaugeComponent Interface
 * 
 * Defines the contract for composable gauge components
 */
class GaugeComponent {
public:
    virtual void tick(void) = 0;
    virtual void init(void) = 0;
};


/**
 * CompositeGauge
 * 
 * A container that takes GaugeComponents, inits them and 
 *  calls tick() on each one at each loop iteration
 *  
 * Always add the sensors before any other component
 */
class CompositeGauge {
    vector<GaugeComponent*> components;
public:
    CompositeGauge(void);
    void add(GaugeComponent *component);
    void init(void);
    void tick(void);
};

#endif
