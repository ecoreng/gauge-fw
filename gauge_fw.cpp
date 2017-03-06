#include "gauge_fw.h"

CompositeGauge::CompositeGauge(void) {}

void CompositeGauge::init(void) {}

void CompositeGauge::add(GaugeComponent *component) {
    this->components.push_back(component);
    component->init();
}

void CompositeGauge::tick(void) {
    vector<GaugeComponent*>::iterator it;
    for (it = this->components.begin(); it != this->components.end(); ++it) {
        (*it)->tick();
    }
}
