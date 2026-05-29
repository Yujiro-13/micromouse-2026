#ifndef TASK_HPP
#define TASK_HPP

#include "interrupt.hpp"
#include "drivers.hpp"
#include "sens_structs.hpp"

void myTaskInterrupt(void *pvparam);
//void myTaskAdc(void *pvparam);
void myTaskLog(void *pvparam);

#endif