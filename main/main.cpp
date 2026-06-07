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
#include "task.hpp"
#include "wall_sensor.hpp"
#include "files.hpp"
#include "board_config.h"



SensorData sens;

std::shared_ptr<Drivers> driver = std::make_shared<Drivers>();

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
        if (pin < 0) continue;  // -1 = 未使用スロット（menuconfig で無効化）
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
    driver->led->set(0b1100);

    // 壁センササンプラ。タスク存続中ずっと参照されるため static で寿命を確保する。
    // タイマ/セマフォの生成(init)は他ペリフェラルと同じく init フェーズで行う。
    static WallSensorSampler wall_sensor;
    wall_sensor.init(driver, &sens);
    xTaskCreatePinnedToCore(myTaskAdc,
                            "adc", 8192, &wall_sensor, configMAX_PRIORITIES - 2, NULL, APP_CPU_NUM);
}

extern "C" void app_main(void)
{
    init_hardware();

    init_files();
    
    while (1)
    {
        // h = driver->imu->accel_z() * 360;
        //driver->np->set_hsv({h, 100, 10}, 0, 1);
        driver->np->set_hsv({240, 100, 100}, 0, 1);
        driver->np->show();
        //driver->np->gaming_mouse();
        // printf("BAT : %f\n", sens.battery_voltage);
        // printf("sens.wall.val.fl:%d  sens.wall.val.l:%d  sens.wall.val.r:%d  sens.wall.val.fr:%d\n", sens.wall.val.fl, sens.wall.val.l, sens.wall.val.r, sens.wall.val.fr);
        //   printf("driver->adc->off:%d\n", driver->adc->_off);
        run_micromouse(driver, &sens);

        //driver->mot->set_motor_speed((0.2), (0.2));

        

        // printf("Z : %ld\n", h);
        /*
        driver->mot->set_motor_speed(1.0 * sin(t), 1.0 * sin(t));
        t = t + 0.01;
        if (t > 2 * M_PI)
            t = 0.0;
        */

        //printf("gyro_z : %f\n", driver->imu->gyro_z());
        // printf("ang_vel : %f\n", driver->imu->gyro_z() * (M_PI / 180.0));
        //rad += driver->imu->gyro_z() * (M_PI / 180.0) / 1000.0 *100;// 1tick が100ms周期になっているため *100
        //printf("rad : %f\n", rad);

        /*
        h = driver->encL->read_angle();
        h1 = driver->encR->read_angle();

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
