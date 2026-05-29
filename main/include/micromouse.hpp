#ifndef MICROMOUSE_HPP
#define MICROMOUSE_HPP

#include "Interrupt.hpp"

void MICROMOUSE(std::shared_ptr<Drivers> driver, SensorData *sens);

#endif