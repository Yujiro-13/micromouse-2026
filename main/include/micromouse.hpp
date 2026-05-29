#ifndef MICROMOUSE_HPP
#define MICROMOUSE_HPP

#include "interrupt.hpp"

void run_micromouse(std::shared_ptr<Drivers> driver, SensorData *sens);

#endif