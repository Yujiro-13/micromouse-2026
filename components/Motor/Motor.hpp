#ifndef MOTOR_HPP
#define MOTOR_HPP

#include "driver/ledc.h"
#include "driver/gpio.h"
#include "driver/mcpwm_prelude.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <math.h>

class Motor
{
public:
    Motor(gpio_num_t, gpio_num_t, gpio_num_t, gpio_num_t, gpio_num_t, gpio_num_t);
    ~Motor();
    void setMotorSpeed(float, float);
    void setFanSpeed(float);
private:
    gpio_num_t ph_pin_R, en_pin_R, ph_pin_L, en_pin_L, fan_pin;
    float spdR,spdL;
    float fan;
    float t = 0;
};

#endif