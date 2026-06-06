#ifndef ADS7066_HPP
#define ADS7066_HPP

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sdkconfig.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_log.h"
//#include "esp_timer.h"
#include <cstring>
#include <iostream>

class ADS7066
{
public:
    ADS7066(spi_host_device_t, gpio_num_t);
    ~ADS7066();
    uint16_t read_one_shot(int8_t);
    uint16_t read_on_the_fly(int8_t channelID = -1);

    float battery_voltage();
    //void WallSensor();
    //void adc_sensing();

    uint16_t _on, _off;
    // FR L R FL
    uint16_t value[4];
    //uint16_t charge_us = 60;
    //uint16_t rise_us = 15;
    const uint8_t SENS[4] = {0, 6, 1, 7}; // OLD : FR L R FL  NEW : FR FL R L
    const gpio_num_t LED[4] = {GPIO_NUM_10, GPIO_NUM_18, GPIO_NUM_17, GPIO_NUM_21}; // OLD : FR L R FL  NEW : FL FR L R



private:
    spi_device_handle_t _spi;
    spi_bus_config_t bus_adc;
    esp_err_t err;
    spi_device_interface_config_t dev_adc;
    uint8_t SEQ_MODE;

    void write_register(uint8_t adrs, uint8_t data);
    uint8_t read_register(uint8_t adrs);
    esp_err_t change_seq_mode(uint8_t mode);

    //esp_timer_handle_t charge_timer;
    //static void timer_chargeCompleted(void *arg);

};

#endif