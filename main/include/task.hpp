#ifndef TASK_HPP
#define TASK_HPP

#include <memory>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "esp_timer.h"
#include "interrupt.hpp"
#include "drivers.hpp"
#include "sens_structs.hpp"

// 壁センサ ADC タスクへ渡すコンテキスト。
// driver/sens はアプリ側(main)が保持する実体を指す（タスクはそこへ読み書きする）。
// wall_charged / charge_timer は adc_task_init() が初期化フェーズで生成する。
struct AdcTaskContext
{
    std::shared_ptr<Drivers> driver;
    SensorData *sens;
    SemaphoreHandle_t wall_charged = nullptr;
    esp_timer_handle_t charge_timer = nullptr;
};

void myTaskInterrupt(void *pvparam);
// ADC タスクが使うセマフォ/タイマを生成する。タスク起動前に init フェーズで呼ぶこと。
void adc_task_init(AdcTaskContext *ctx);
void myTaskAdc(void *pvparam);
void myTaskLog(void *pvparam);

#endif