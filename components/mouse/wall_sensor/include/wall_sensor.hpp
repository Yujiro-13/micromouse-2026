#ifndef WALL_SENSOR_HPP
#define WALL_SENSOR_HPP

#include <memory>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "esp_timer.h"
#include "drivers.hpp"
#include "sens_structs.hpp"

// 壁センサ(ADS7066)のサンプリングを担うクラス。
// 充放電タイミング制御・EMA(指数移動平均)フィルタを内包し、結果を SensorData へ書き込む。
// FreeRTOS タスクからは sampling() を呼ぶ(Interrupt::interrupt()/logging() と同様の使い方)。
class WallSensorSampler
{
public:
    // driver/sens を束縛し、充電完了通知用のセマフォ・ワンショットタイマを生成する。
    // ペリフェラル初期化と同じく、タスク起動前の init フェーズで一度だけ呼ぶ。
    void init(std::shared_ptr<Drivers> driver, SensorData *sens);

    // サンプリングループ本体。FreeRTOS タスクから呼ぶ(戻らない)。
    void sampling();

private:
    // 充電完了タイマのコールバック。セマフォは arg 経由で受け取る(グローバル依存を排除)。
    static void on_charge_completed(void *arg);

    std::shared_ptr<Drivers> driver;
    SensorData *sens = nullptr;
    SemaphoreHandle_t wall_charged = nullptr;
    esp_timer_handle_t charge_timer = nullptr;
};

#endif // WALL_SENSOR_HPP
