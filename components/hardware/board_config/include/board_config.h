#pragma once

// ボード固有のハードウェア配線（GPIO / SPI / I2C ピン割り当て）を集約する。
// 各定数は「このファイルにソース定義した既定値」と「menuconfig(CONFIG_*) の値」の
// 両方を持ち、どちらを採用するかは menuconfig のマスタースイッチで切り替える:
//   menuconfig: "reRoMouse Board Hardware" -> "Use menuconfig values for board pins"
//     OFF (CONFIG_BOARD_PINS_FROM_MENUCONFIG=n, 既定) … 下記 BOARD_PICK 第2引数の
//                                                       ソース定義値を使う。
//     ON  (=y)                                       … CONFIG_* (第1引数) を使う。
//   既定値はどちらも同一なので、切り替えても現行基板の配線は変わらない。
// ピン番号のハードコードはこの 1 ファイルに閉じること（各所への直書きは禁止）。

#include <stdint.h>
#include "sdkconfig.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "driver/i2c.h"

// 採用元の切り替え。第1引数=menuconfig値, 第2引数=board_config.h ソース定義値。
#if CONFIG_BOARD_PINS_FROM_MENUCONFIG
#define BOARD_PICK(cfg, fallback) (cfg)
#else
#define BOARD_PICK(cfg, fallback) (fallback)
#endif

namespace board {

// ---- 起動時に出力設定する GPIO 群（app_main 冒頭の gpio_config）----
// 値が負(-1 = 未使用)の要素は呼び出し側でスキップすること。
constexpr gpio_num_t kStartupOutputPins[] = {
    static_cast<gpio_num_t>(BOARD_PICK(CONFIG_BOARD_STARTUP_OUTPUT_GPIO0, 10)),
    static_cast<gpio_num_t>(BOARD_PICK(CONFIG_BOARD_STARTUP_OUTPUT_GPIO1, 17)),
    static_cast<gpio_num_t>(BOARD_PICK(CONFIG_BOARD_STARTUP_OUTPUT_GPIO2, 18)),
    static_cast<gpio_num_t>(BOARD_PICK(CONFIG_BOARD_STARTUP_OUTPUT_GPIO3, 21))};

// ---- SPI バス: IMU(MPU6500) + 壁センサ ADC(ADS7066) ----
#if CONFIG_BOARD_PINS_FROM_MENUCONFIG && CONFIG_BOARD_IMU_ADC_SPI_HOST_SPI3
constexpr spi_host_device_t kImuAdcSpiHost = SPI3_HOST;
#else
constexpr spi_host_device_t kImuAdcSpiHost = SPI2_HOST;  // menuconfig:SPI2 / source:SPI2
#endif
constexpr gpio_num_t kImuAdcSpiMiso = static_cast<gpio_num_t>(BOARD_PICK(CONFIG_BOARD_IMU_ADC_SPI_MISO_GPIO, 2));
constexpr gpio_num_t kImuAdcSpiMosi = static_cast<gpio_num_t>(BOARD_PICK(CONFIG_BOARD_IMU_ADC_SPI_MOSI_GPIO, 4));
constexpr gpio_num_t kImuAdcSpiSclk = static_cast<gpio_num_t>(BOARD_PICK(CONFIG_BOARD_IMU_ADC_SPI_SCLK_GPIO, 3));
constexpr gpio_num_t kAdcCs = static_cast<gpio_num_t>(BOARD_PICK(CONFIG_BOARD_ADC_CS_GPIO, 5));  // ADS7066 チップセレクト
constexpr gpio_num_t kImuCs = static_cast<gpio_num_t>(BOARD_PICK(CONFIG_BOARD_IMU_CS_GPIO, 1));  // MPU6500 チップセレクト

// ---- SPI バス: 磁気エンコーダ(MA730) 左右 ----
#if CONFIG_BOARD_PINS_FROM_MENUCONFIG && CONFIG_BOARD_ENCODER_SPI_HOST_SPI2
constexpr spi_host_device_t kEncoderSpiHost = SPI2_HOST;
#else
constexpr spi_host_device_t kEncoderSpiHost = SPI3_HOST;  // menuconfig:SPI3 / source:SPI3
#endif
constexpr gpio_num_t kEncoderSpiMosi = static_cast<gpio_num_t>(BOARD_PICK(CONFIG_BOARD_ENCODER_SPI_MOSI_GPIO, 9));
constexpr gpio_num_t kEncoderSpiMiso = static_cast<gpio_num_t>(BOARD_PICK(CONFIG_BOARD_ENCODER_SPI_MISO_GPIO, 8));
constexpr gpio_num_t kEncoderSpiSclk = static_cast<gpio_num_t>(BOARD_PICK(CONFIG_BOARD_ENCODER_SPI_SCLK_GPIO, 7));
constexpr gpio_num_t kEncoderLeftCs = static_cast<gpio_num_t>(BOARD_PICK(CONFIG_BOARD_ENCODER_LEFT_CS_GPIO, 6));
constexpr gpio_num_t kEncoderRightCs = static_cast<gpio_num_t>(BOARD_PICK(CONFIG_BOARD_ENCODER_RIGHT_CS_GPIO, 14));

// ---- I2C バス: LED ドライバ(PCA9632) ----
constexpr i2c_port_t kLedI2cPort = static_cast<i2c_port_t>(BOARD_PICK(CONFIG_BOARD_LED_I2C_PORT, 0));
constexpr gpio_num_t kLedI2cSda = static_cast<gpio_num_t>(BOARD_PICK(CONFIG_BOARD_LED_I2C_SDA_GPIO, 38));
constexpr gpio_num_t kLedI2cScl = static_cast<gpio_num_t>(BOARD_PICK(CONFIG_BOARD_LED_I2C_SCL_GPIO, 39));
constexpr uint8_t kLedI2cAddr = static_cast<uint8_t>(BOARD_PICK(CONFIG_BOARD_LED_I2C_ADDR, 0x62));
constexpr uint32_t kLedI2cClockHz = static_cast<uint32_t>(BOARD_PICK(CONFIG_BOARD_LED_I2C_CLOCK_HZ, 1000000));

// ---- ブザー ----
constexpr gpio_num_t kBuzzer = static_cast<gpio_num_t>(BOARD_PICK(CONFIG_BOARD_BUZZER_GPIO, 15));

// ---- NeoPixel(RGB LED) ----
constexpr gpio_num_t kNeoPixel = static_cast<gpio_num_t>(BOARD_PICK(CONFIG_BOARD_NEOPIXEL_GPIO, 13));

// ---- モータドライバ（Motor コンストラクタの引数順に対応）----
constexpr gpio_num_t kMotorPhaseRight = static_cast<gpio_num_t>(BOARD_PICK(CONFIG_BOARD_MOTOR_PHASE_RIGHT_GPIO, 41));
constexpr gpio_num_t kMotorEnableRight = static_cast<gpio_num_t>(BOARD_PICK(CONFIG_BOARD_MOTOR_ENABLE_RIGHT_GPIO, 42));
constexpr gpio_num_t kMotorPhaseLeft = static_cast<gpio_num_t>(BOARD_PICK(CONFIG_BOARD_MOTOR_PHASE_LEFT_GPIO, 45));
constexpr gpio_num_t kMotorEnableLeft = static_cast<gpio_num_t>(BOARD_PICK(CONFIG_BOARD_MOTOR_ENABLE_LEFT_GPIO, 46));
constexpr gpio_num_t kMotorFan = static_cast<gpio_num_t>(BOARD_PICK(CONFIG_BOARD_MOTOR_FAN_GPIO, 11));
constexpr gpio_num_t kMotorMode = static_cast<gpio_num_t>(BOARD_PICK(CONFIG_BOARD_MOTOR_MODE_GPIO, 40));

}  // namespace board
