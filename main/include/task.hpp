#ifndef TASK_HPP
#define TASK_HPP

#include <memory>
#include "interrupt.hpp"
#include "drivers.hpp"
#include "sens_structs.hpp"

// 壁センサ ADC タスクへ渡すコンテキスト。
// driver/sens はアプリ側(main)が保持する実体を指す（タスクはそこへ読み書きする）。
struct AdcTaskContext
{
    std::shared_ptr<Drivers> driver;
    SensorData *sens;
};

void myTaskInterrupt(void *pvparam);
void myTaskAdc(void *pvparam);
void myTaskLog(void *pvparam);

#endif