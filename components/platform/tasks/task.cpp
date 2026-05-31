#include "task.hpp"
#include "interrupt.hpp"
#include "wall_sensor.hpp"

// 各タスクは通常クラスのメソッドを FreeRTOS タスク用にラップするのみ。
// 実装本体は対応クラス(Interrupt / WallSensorSampler)側にカプセル化されている。

void myTaskInterrupt(void *pvparam)
{
    Interrupt *interrupt = static_cast<Interrupt *>(pvparam);
    interrupt->interrupt();
}

void myTaskAdc(void *pvparam)
{
    WallSensorSampler *wall_sensor = static_cast<WallSensorSampler *>(pvparam);
    wall_sensor->sampling();
}

void myTaskLog(void *pvparam)
{
    Interrupt *log = static_cast<Interrupt *>(pvparam);
    log->logging();
}
