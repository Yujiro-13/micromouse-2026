#include "Task.hpp"
#include <iostream>
#include <memory>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include "esp_timer.h"

//static SemaphoreHandle_t wallCharged;

/*static void timer_chargeCompleted(void *arg)
{
    BaseType_t _highPriorityTask = pdFALSE;
    xSemaphoreGiveFromISR(wallCharged, &_highPriorityTask);
    portYIELD_FROM_ISR(_highPriorityTask);
}*/

void myTaskInterrupt(void *pvpram)
{
    Interrupt *interrupt = static_cast<Interrupt *>(pvpram);
    interrupt->interrupt();
}

/*void myTaskAdc(void *pvpram)
{
    // pvpram が指す既存の ADS7066 ポインタを std::shared_ptr に変換
    ADS7066* raw_adc_ptr = static_cast<ADS7066 *>(pvpram);
    std::shared_ptr<ADS7066> adc(raw_adc_ptr);

    // t_drivers 構造体を作成し、adc ポインタを設定
    std::shared_ptr<t_drivers> driver = std::make_shared<t_drivers>();
    driver->adc = adc;

    esp_timer_handle_t chargeTimer;

    for (int i = 0; i < 4; i++)
    {
        gpio_set_level(driver->adc->LED[i], 1);
    }
    driver->adc->_off = driver->adc->readOnTheFly(4);

    esp_timer_create_args_t chargeTimerSetting = {
        .callback = &timer_chargeCompleted,
        .name = "wallCharge"};
    esp_timer_handle_t chargeTimer;
    ESP_ERROR_CHECK(esp_timer_create(&chargeTimerSetting, &chargeTimer));

    wallCharged = xSemaphoreCreateBinary();

    uint16_t charge_us = 60;
    uint16_t rise_us = 15;

    std::shared_ptr<t_sens_data> sens = std::make_shared<t_sens_data>();

    while(1)
    {
        sens->BatteryVoltage = driver->adc->BatteryVoltage();
        for (int i = 0; i < 4; i++)
        {
            if (i > 0)
            {
                driver->adc->_on = driver->adc->readOnTheFly(driver->adc->SENS[i]);
                gpio_set_level(driver->adc->LED[i], 0);
                esp_timer_start_once(chargeTimer, charge_us);
                xSemaphoreTake(wallCharged, portMAX_DELAY);
                gpio_set_level(driver->adc->LED[i], 1);
                esp_rom_delay_us(rise_us);
                if (i > 0)
                    driver->adc->value[i - 1] = driver->adc->_on - driver->adc->_off;
                driver->adc->_off = driver->adc->readOnTheFly(driver->adc->SENS[i]);
            }
                
        }
        driver->adc->_on = driver->adc->readOnTheFly(4);
        driver->adc->value[3] = driver->adc->_on - driver->adc->_off;

        sens->wall.val.fr = driver->adc->value[0];
        sens->wall.val.l = driver->adc->value[2];
        sens->wall.val.r = driver->adc->value[1];
        sens->wall.val.fl = driver->adc->value[3];

        vTaskDelay(1 /portTICK_PERIOD_MS);
    }
}*/

void myTaskLog(void *pvpram)
{
    Interrupt *log = static_cast<Interrupt *>(pvpram);
    log->logging();
}