#pragma once

// ボード固有のハードウェア配線（GPIO / SPI / I2C ピン割り当て）を集約する。
// ピン番号はこの 1 ファイルのみで管理し、各所へのハードコードを禁止する。

#include <stdint.h>
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "driver/i2c.h"

namespace board {

// ---- 起動時に出力設定する GPIO 群（app_main 冒頭の gpio_config）----
constexpr gpio_num_t kStartupOutputPins[] = {
    GPIO_NUM_10, GPIO_NUM_17, GPIO_NUM_18, GPIO_NUM_21};

// ---- SPI バス: IMU(MPU6500) + 壁センサ ADC(ADS7066) ----
constexpr spi_host_device_t kImuAdcSpiHost = SPI2_HOST;
constexpr gpio_num_t kImuAdcSpiMiso = GPIO_NUM_2;
constexpr gpio_num_t kImuAdcSpiMosi = GPIO_NUM_4;
constexpr gpio_num_t kImuAdcSpiSclk = GPIO_NUM_3;
constexpr gpio_num_t kAdcCs = GPIO_NUM_5;   // ADS7066 チップセレクト
constexpr gpio_num_t kImuCs = GPIO_NUM_1;   // MPU6500 チップセレクト

// ---- SPI バス: 磁気エンコーダ(MA730) 左右 ----
constexpr spi_host_device_t kEncoderSpiHost = SPI3_HOST;
constexpr gpio_num_t kEncoderSpiMosi = GPIO_NUM_9;
constexpr gpio_num_t kEncoderSpiMiso = GPIO_NUM_8;
constexpr gpio_num_t kEncoderSpiSclk = GPIO_NUM_7;
constexpr gpio_num_t kEncoderLeftCs = GPIO_NUM_6;
constexpr gpio_num_t kEncoderRightCs = GPIO_NUM_14;

// ---- I2C バス: LED ドライバ(PCA9632) ----
constexpr i2c_port_t kLedI2cPort = I2C_NUM_0;
constexpr gpio_num_t kLedI2cSda = GPIO_NUM_38;
constexpr gpio_num_t kLedI2cScl = GPIO_NUM_39;
constexpr uint8_t kLedI2cAddr = 0x62;
constexpr uint32_t kLedI2cClockHz = 1000000;

// ---- ブザー ----
constexpr gpio_num_t kBuzzer = GPIO_NUM_15;

// ---- NeoPixel(RGB LED) ----
constexpr gpio_num_t kNeoPixel = GPIO_NUM_13;

// ---- モータドライバ（Motor コンストラクタの引数順に対応）----
constexpr gpio_num_t kMotorPhaseRight = GPIO_NUM_41;
constexpr gpio_num_t kMotorEnableRight = GPIO_NUM_42;
constexpr gpio_num_t kMotorPhaseLeft = GPIO_NUM_45;
constexpr gpio_num_t kMotorEnableLeft = GPIO_NUM_46;
constexpr gpio_num_t kMotorFan = GPIO_NUM_11;
constexpr gpio_num_t kMotorMode = GPIO_NUM_40;

}  // namespace board
