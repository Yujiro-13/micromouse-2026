#include <iostream>
#include <memory>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "driver/spi_master.h"
#include "driver/i2c.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_vfs.h"
#include "esp_vfs_fat.h"
#include "esp_heap_caps.h"
#include "esp_timer.h"
#include "structs.hpp"
#include "drivers.hpp"
#include "micromouse.hpp"
#include "files.hpp"
#include "board_config.h"



t_sens_data sens;

std::shared_ptr<t_drivers> driver = std::make_shared<t_drivers>();


static SemaphoreHandle_t wallCharged;

static void timer_chargeCompleted(void *arg)
{
    BaseType_t _highPriorityTask = pdFALSE;
    xSemaphoreGiveFromISR(wallCharged, &_highPriorityTask);
    portYIELD_FROM_ISR(_highPriorityTask);
}

void myTaskAdc(void *pvpram)
{
    // pvpram が指す既存の ADS7066 ポインタを std::shared_ptr に変換
    /*ADS7066* raw_adc_ptr = static_cast<ADS7066 *>(pvpram);
    std::shared_ptr<ADS7066> adc(raw_adc_ptr);
    driver->led->set(0b1010);

    // t_drivers 構造体を作成し、adc ポインタを設定
    std::shared_ptr<t_drivers> driver = std::make_shared<t_drivers>();
    driver->adc = adc;*/

    ESP_LOGI("ADC", "ADC Task Start");
    driver->led->set(0b1000);

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
    driver->led->set(0b1111);

    // センサの設定 (コンデンサ充電時間、放電時間 値のオーバーフロー対策必須（時間設定するか、例外処理追加するか）)　ｒが怪しい
    uint16_t charge_us = 500; // コンデンサへの充電時間
    uint16_t rise_us = 30;    // 放電してからセンサの読み取りを開始するまでの時間

    while (1)
    {
        sens.BatteryVoltage = driver->adc->BatteryVoltage();
        for (int i = 0; i < 4; i++)
        {
            if (i > 0) // i = 0 のときは _on が初期化されていないため、読み取りを行わない
            {
                driver->adc->_on = driver->adc->readOnTheFly(driver->adc->SENS[i]); // readOnTheFly は 送ったアドレスのひとつ前に送った値を返す
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
            driver->adc->_off = driver->adc->readOnTheFly(driver->adc->SENS[i]);
        }
        driver->adc->_on = driver->adc->readOnTheFly(4);
        if (driver->adc->_on - driver->adc->_off > 0) // on, off の差分が正のとき(on時の値のほうが大きいとき)
        {
            driver->adc->value[3] = driver->adc->_on - driver->adc->_off;
        }
        else
        {
            driver->adc->value[3] = driver->adc->_off - driver->adc->_on;
        }

        sens.wall.val.fr = driver->adc->value[0];
        sens.wall.val.r = driver->adc->value[2];
        sens.wall.val.l = driver->adc->value[1];
        sens.wall.val.fl = driver->adc->value[3];
        
        // === 壁センサローパスフィルタ（指数移動平均） ===
        static float wall_fl_filtered = 0.0;
        static float wall_fr_filtered = 0.0;
        static float wall_l_filtered = 0.0;
        static float wall_r_filtered = 0.0;
        static const float wall_filter_alpha = 0.5; // 指数移動平均の重み（0.0-1.0、小さいほど平滑化が強い）
        
        // 生の壁センサ値を取得
        float raw_fl = sens.wall.val.fl;
        float raw_fr = sens.wall.val.fr;
        float raw_l = sens.wall.val.l;
        float raw_r = sens.wall.val.r;
        
        // 指数移動平均（EMAフィルタ）
        wall_fl_filtered = wall_filter_alpha * raw_fl + (1.0 - wall_filter_alpha) * wall_fl_filtered;
        wall_fr_filtered = wall_filter_alpha * raw_fr + (1.0 - wall_filter_alpha) * wall_fr_filtered;
        wall_l_filtered = wall_filter_alpha * raw_l + (1.0 - wall_filter_alpha) * wall_l_filtered;
        wall_r_filtered = wall_filter_alpha * raw_r + (1.0 - wall_filter_alpha) * wall_r_filtered;
        
        // フィルタ後の値を構造体に書き戻す
        sens.wall.val.fl = (int)wall_fl_filtered;
        sens.wall.val.fr = (int)wall_fr_filtered;
        sens.wall.val.l = (int)wall_l_filtered;
        sens.wall.val.r = (int)wall_r_filtered;

        vTaskDelay(1 / portTICK_PERIOD_MS);
    }
}

// ペリフェラル（GPIO/SPI/I2C/各ドライバ）の初期化とセンサタスク起動。
// 初期化順序はハードウェア依存があるため変更しないこと。
static void init_hardware(void)
{
    gpio_config_t io_conf = {};
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    uint64_t startup_output_mask = 0;
    for (gpio_num_t pin : board::kStartupOutputPins)
    {
        startup_output_mask |= (1ULL << pin);
    }
    io_conf.pin_bit_mask = startup_output_mask;
    io_conf.pull_down_en = GPIO_PULLDOWN_ENABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;

    ESP_ERROR_CHECK(gpio_config(&io_conf));

    // IMU SPIバスの設定
    spi_bus_config_t bus_imu_adc;
    memset(&bus_imu_adc, 0, sizeof(bus_imu_adc));
    bus_imu_adc.miso_io_num = board::kImuAdcSpiMiso;
    bus_imu_adc.mosi_io_num = board::kImuAdcSpiMosi;
    bus_imu_adc.sclk_io_num = board::kImuAdcSpiSclk;
    bus_imu_adc.quadwp_io_num = -1;
    bus_imu_adc.quadhd_io_num = -1;

    ESP_ERROR_CHECK(spi_bus_initialize(board::kImuAdcSpiHost, &bus_imu_adc, SPI_DMA_CH_AUTO));

    driver->adc = std::make_shared<ADS7066>(board::kImuAdcSpiHost, board::kAdcCs);
    driver->imu = std::make_shared<MPU6500>(board::kImuAdcSpiHost, board::kImuCs);

    // Encoder SPIバスの設定
    spi_bus_config_t bus_enc;
    memset(&bus_enc, 0, sizeof(bus_enc));
    bus_enc.mosi_io_num = board::kEncoderSpiMosi;
    bus_enc.miso_io_num = board::kEncoderSpiMiso;
    bus_enc.sclk_io_num = board::kEncoderSpiSclk;
    bus_enc.quadwp_io_num = -1;
    bus_enc.quadhd_io_num = -1;
    bus_enc.max_transfer_sz = 4;
    bus_enc.flags = SPICOMMON_BUSFLAG_MASTER;
    bus_enc.intr_flags = 0;

    ESP_ERROR_CHECK(spi_bus_initialize(board::kEncoderSpiHost, &bus_enc, SPI_DMA_DISABLED));

    driver->encL = std::make_shared<MA730>(board::kEncoderSpiHost, board::kEncoderLeftCs, 1);
    driver->encR = std::make_shared<MA730>(board::kEncoderSpiHost, board::kEncoderRightCs, 0);

    // LED driver I2Cバスの設定
    i2c_config_t led_conf;
    memset(&led_conf, 0, sizeof(led_conf));
    led_conf.mode = I2C_MODE_MASTER;
    led_conf.sda_io_num = board::kLedI2cSda;
    led_conf.scl_io_num = board::kLedI2cScl;
    led_conf.sda_pullup_en = GPIO_PULLUP_DISABLE;
    led_conf.scl_pullup_en = GPIO_PULLUP_DISABLE;
    led_conf.master.clk_speed = board::kLedI2cClockHz;
    led_conf.clk_flags = 0;

    ESP_ERROR_CHECK(i2c_param_config(board::kLedI2cPort, &led_conf));
    ESP_ERROR_CHECK(i2c_driver_install(board::kLedI2cPort, I2C_MODE_MASTER, 0, 0, 0));

    driver->led = std::make_shared<PCA9632>(board::kLedI2cPort, board::kLedI2cAddr);

    driver->led->set(0b1111);

    // Buzzer GPIOの設定
    driver->bz = std::make_shared<Buzzer>(board::kBuzzer);
    static Buzzer::buzzer_score_t pc98[] = {
        {2000, 100}, {1000, 100}};
    driver->bz->play_melody(pc98, 2);

    // NeoPixel GPIOの設定
    driver->np = std::make_shared<NeoPixel>(board::kNeoPixel, 1);
    driver->np->set_hsv({0, 0, 0}, 0, 1);
    driver->np->show();

    // Motor driver Fan Moter GPIOの設定
    driver->mot = std::make_shared<Motor>(board::kMotorPhaseRight, board::kMotorEnableRight,
                                          board::kMotorPhaseLeft, board::kMotorEnableLeft,
                                          board::kMotorFan, board::kMotorMode);

    driver->led->set(0b1110);
    ADS7066 *adc = driver->adc.get();
    driver->led->set(0b1100);

    xTaskCreatePinnedToCore(myTaskAdc,
                            "adc", 8192, adc, configMAX_PRIORITIES - 2, NULL, APP_CPU_NUM);
}

extern "C" void app_main(void)
{
    init_hardware();

    init_files();
    
    while (1)
    {
        // h = driver->imu->accelZ() * 360;
        //driver->np->set_hsv({h, 100, 10}, 0, 1);
        driver->np->set_hsv({240, 100, 100}, 0, 1);
        driver->np->show();
        //driver->np->gaming_mouse();
        // printf("BAT : %f\n", sens.BatteryVoltage);
        // printf("sens.wall.val.fl:%d  sens.wall.val.l:%d  sens.wall.val.r:%d  sens.wall.val.fr:%d\n", sens.wall.val.fl, sens.wall.val.l, sens.wall.val.r, sens.wall.val.fr);
        //   printf("driver->adc->off:%d\n", driver->adc->_off);
        MICROMOUSE(driver, &sens);

        //driver->mot->setMotorSpeed((0.2), (0.2));

        

        // printf("Z : %ld\n", h);
        /*
        driver->mot->setMotorSpeed(1.0 * sin(t), 1.0 * sin(t));
        t = t + 0.01;
        if (t > 2 * M_PI)
            t = 0.0;
        */

        //printf("gyroZ : %f\n", driver->imu->gyroZ());
        // printf("ang_vel : %f\n", driver->imu->gyroZ() * (M_PI / 180.0));
        //rad += driver->imu->gyroZ() * (M_PI / 180.0) / 1000.0 *100;// 1tick が100ms周期になっているため *100
        //printf("rad : %f\n", rad);

        /*
        h = driver->encL->readAngle();
        h1 = driver->encR->readAngle();

        float WheelAngle_L = 2.0 * M_PI * h / 16384.0;
        float WheelAngle_R = 2.0 * M_PI * h1 / 16384.0;

        float WeeelDegree_L = WheelAngle_L * 180.0 / M_PI;
        float WeeelDegree_R = WheelAngle_R * 180.0 / M_PI;

        printf(">L:%ld\n", h);
        //printf(">R:%ld\n", h1);
        //printf("L:%f    R:%f\n", WheelAngle_L, WheelAngle_R);
        // printf("L:%f    R:%f\n", WeeelDegree_L, WeeelDegree_R);
        */
        //  ESP_LOGI("MAIN", "MAIN LOOP");
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
