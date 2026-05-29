#ifndef DRIVERS_HPP
#define DRIVERS_HPP

#include <iostream>
#include <memory>
#include "NeoPixel.hpp"
#include "MPU6500.hpp"
#include "PCA9632.hpp"
#include "Buzzer.hpp"
#include "Motor.hpp"
#include "MA730.hpp"
#include "ADS7066.hpp"

typedef struct
{
    std::shared_ptr<NeoPixel> np;
    std::shared_ptr<MPU6500> imu;
    std::shared_ptr<PCA9632> led;
    std::shared_ptr<BUZZER> bz;
    std::shared_ptr<Motor> mot;
    std::shared_ptr<MA730> encL;
    std::shared_ptr<MA730> encR;
    std::shared_ptr<ADS7066> adc;
}t_drivers;


#endif
