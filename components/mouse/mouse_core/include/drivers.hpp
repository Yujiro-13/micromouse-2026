#ifndef DRIVERS_HPP
#define DRIVERS_HPP

#include <iostream>
#include <memory>
#include "neopixel.hpp"
#include "mpu6500.hpp"
#include "pca9632.hpp"
#include "buzzer.hpp"
#include "motor.hpp"
#include "ma730.hpp"
#include "ads7066.hpp"

typedef struct
{
    std::shared_ptr<NeoPixel> np;
    std::shared_ptr<MPU6500> imu;
    std::shared_ptr<PCA9632> led;
    std::shared_ptr<Buzzer> bz;
    std::shared_ptr<Motor> mot;
    std::shared_ptr<MA730> encL;
    std::shared_ptr<MA730> encR;
    std::shared_ptr<ADS7066> adc;
}Drivers;


#endif
