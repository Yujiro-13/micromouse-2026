#include "task.hpp"
#include <iostream>
#include <memory>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_rom_sys.h"

static void timer_chargeCompleted(void *arg)
{
    SemaphoreHandle_t wall_charged = static_cast<SemaphoreHandle_t>(arg);
    BaseType_t high_priority_task = pdFALSE;
    xSemaphoreGiveFromISR(wall_charged, &high_priority_task);
    portYIELD_FROM_ISR(high_priority_task);
}

// 壁充電完了通知用のセマフォとワンショットタイマを生成する。
// ペリフェラル初期化と同じく、タスク起動前の init フェーズで一度だけ呼ぶ。
void adc_task_init(AdcTaskContext *ctx)
{
    ctx->wall_charged = xSemaphoreCreateBinary();

    // セマフォはタイマの arg 経由でコールバックへ渡す（グローバル依存を排除）。
    const esp_timer_create_args_t charge_timer_args = {
        .callback = &timer_chargeCompleted,
        .arg = ctx->wall_charged,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "wallCharge",
        .skip_unhandled_events = false,
    };
    ESP_ERROR_CHECK(esp_timer_create(&charge_timer_args, &ctx->charge_timer));
}

void myTaskInterrupt(void *pvpram)
{
    Interrupt *interrupt = static_cast<Interrupt *>(pvpram);
    interrupt->interrupt();
}

void myTaskAdc(void *pvpram)
{
    AdcTaskContext *ctx = static_cast<AdcTaskContext *>(pvpram);
    std::shared_ptr<Drivers> driver = ctx->driver;
    SensorData *sens = ctx->sens;
    // セマフォ/タイマは adc_task_init() が init フェーズで生成済み。
    SemaphoreHandle_t wallCharged = ctx->wall_charged;
    esp_timer_handle_t chargeTimer = ctx->charge_timer;

    ESP_LOGI("ADC", "ADC Task Start");
    driver->led->set(0b1000);

    for (int i = 0; i < 4; i++)
    {
        gpio_set_level(driver->adc->LED[i], 1);
    }
    driver->adc->_off = driver->adc->read_on_the_fly(4);

    driver->led->set(0b1111);

    // センサの設定 (コンデンサ充電時間、放電時間 値のオーバーフロー対策必須（時間設定するか、例外処理追加するか）)　ｒが怪しい
    uint16_t charge_us = 500; // コンデンサへの充電時間
    uint16_t rise_us = 30;    // 放電してからセンサの読み取りを開始するまでの時間

    while (1)
    {
        sens->battery_voltage = driver->adc->battery_voltage();
        for (int i = 0; i < 4; i++)
        {
            if (i > 0) // i = 0 のときは _on が初期化されていないため、読み取りを行わない
            {
                driver->adc->_on = driver->adc->read_on_the_fly(driver->adc->SENS[i]); // read_on_the_fly は 送ったアドレスのひとつ前に送った値を返す
            }
            gpio_set_level(driver->adc->LED[i], 0);
            esp_timer_start_once(chargeTimer, charge_us);
            xSemaphoreTake(wallCharged, portMAX_DELAY);
            gpio_set_level(driver->adc->LED[i], 1);
            esp_rom_delay_us(rise_us);
            if (i > 0) // i = 0 のときは _on が初期化されていないため、読み取りを行わない
            {
                // 何も無いところを見ていると、on,offの値が逆転することがあるため対策
                if (driver->adc->_on - driver->adc->_off > 0) // on, off の差分が正のとき(on時の値のほうが大きいとき)
                {
                    driver->adc->value[i - 1] = driver->adc->_on - driver->adc->_off;
                }
                else
                {
                    driver->adc->value[i - 1] = driver->adc->_off - driver->adc->_on;
                }
            }
            driver->adc->_off = driver->adc->read_on_the_fly(driver->adc->SENS[i]);
        }
        driver->adc->_on = driver->adc->read_on_the_fly(4);
        if (driver->adc->_on - driver->adc->_off > 0) // on, off の差分が正のとき(on時の値のほうが大きいとき)
        {
            driver->adc->value[3] = driver->adc->_on - driver->adc->_off;
        }
        else
        {
            driver->adc->value[3] = driver->adc->_off - driver->adc->_on;
        }

        sens->wall.val.fr = driver->adc->value[0];
        sens->wall.val.r = driver->adc->value[2];
        sens->wall.val.l = driver->adc->value[1];
        sens->wall.val.fl = driver->adc->value[3];

        // === 壁センサローパスフィルタ（指数移動平均） ===
        static float wall_fl_filtered = 0.0;
        static float wall_fr_filtered = 0.0;
        static float wall_l_filtered = 0.0;
        static float wall_r_filtered = 0.0;
        static const float wall_filter_alpha = 0.5; // 指数移動平均の重み（0.0-1.0、小さいほど平滑化が強い）

        // 生の壁センサ値を取得
        float raw_fl = sens->wall.val.fl;
        float raw_fr = sens->wall.val.fr;
        float raw_l = sens->wall.val.l;
        float raw_r = sens->wall.val.r;

        // 指数移動平均（EMAフィルタ）
        wall_fl_filtered = wall_filter_alpha * raw_fl + (1.0 - wall_filter_alpha) * wall_fl_filtered;
        wall_fr_filtered = wall_filter_alpha * raw_fr + (1.0 - wall_filter_alpha) * wall_fr_filtered;
        wall_l_filtered = wall_filter_alpha * raw_l + (1.0 - wall_filter_alpha) * wall_l_filtered;
        wall_r_filtered = wall_filter_alpha * raw_r + (1.0 - wall_filter_alpha) * wall_r_filtered;

        // フィルタ後の値を構造体に書き戻す
        sens->wall.val.fl = (int)wall_fl_filtered;
        sens->wall.val.fr = (int)wall_fr_filtered;
        sens->wall.val.l = (int)wall_l_filtered;
        sens->wall.val.r = (int)wall_r_filtered;

        vTaskDelay(1 / portTICK_PERIOD_MS);
    }
}

void myTaskLog(void *pvpram)
{
    Interrupt *log = static_cast<Interrupt *>(pvpram);
    log->logging();
}