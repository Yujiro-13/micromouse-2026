#ifndef BASE_FUNC_HPP
#define BASE_FUNC_HPP

#include <iostream>
#include <vector>
#include <memory>
#include "esp_timer.h"
#include "structs.hpp"
#include "drivers.hpp"


struct Micromouse
{
    virtual void ptr_by_sensor(SensorData *_sens) = 0;
    virtual void ptr_by_motion(MotionValues *_val) = 0;
    virtual void ptr_by_control(Control *_control) = 0;
    virtual void ptr_by_map(MazeMap *_map) = 0;
    virtual void set_device_driver(std::shared_ptr<Drivers> driver) = 0;
};


#endif // BASE_FUNC_HPP