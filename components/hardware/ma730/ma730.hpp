#ifndef MA730_HPP
#define MA730_HPP

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sdkconfig.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_log.h"
#include <cstring>

#define READ_COMMAND 0b010
#define WRITE_COMMAND 0b100
#define ADRS_Rotation_direction 0b01001 // 回転方向逆転のアドレス ここの7bit目を1にすると回転方向が逆になる
#define RESOLUTION_MAX 16384
#define RESORUTION_HALF 8192
#define FCUTOFF_HZ_6000 51
#define FCUTOFF_HZ_3000 68
#define FCUTOFF_HZ_370 119 



class MA730
{
    public:
    MA730(spi_host_device_t bus, gpio_num_t cs, uint8_t ccw);
    ~MA730();

    uint16_t read_angle();
    //void show_angle();

private:
    uint16_t read();
    uint16_t operate_registers(const uint8_t command, const uint8_t address, const uint8_t data);
    uint8_t read_register(const uint8_t address, const uint8_t data);
    uint8_t write_register(const uint8_t address, const uint8_t data);
    void set_filter(const uint8_t fw_value);
    esp_err_t ret;
    esp_err_t err;
    spi_transaction_t cmd;
    spi_device_handle_t _spi;
    spi_bus_config_t bus_enc;
    spi_device_interface_config_t dev_enc;
    //gpio_num_t _cs;

};

#endif