#ifndef MICROMOUSE_HPP
#define MICROMOUSE_HPP

#include "Interrupt.hpp"

void MICROMOUSE(std::shared_ptr<t_drivers> driver, t_sens_data *sens);

#endif