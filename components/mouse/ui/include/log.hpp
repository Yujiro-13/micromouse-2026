#ifndef LOG_HPP
#define LOG_HPP

#include <fstream>
#include <iostream>
#include <string>
#include "esp_flash_spi_init.h"
#include "esp_partition.h"
#include "esp_log.h"
#include "esp_flash.h"
#include "spi_flash_mmap.h"
#include "esp_vfs.h"
#include "esp_vfs_fat.h"
#include "esp_system.h"
#include "UI.hpp"

class Log : public UI
{
    public:
        void ptr_by_sensor(SensorData *_sens) override;
        void ptr_by_motion(MotionValues *_val) override;
        void ptr_by_control(Control *_control) override;
        void ptr_by_map(MazeMap *_map) override;
        void set_device_driver(std::shared_ptr<Drivers> driver) override;
        void main_task() override;
        void ref_by_motion(Adachi &_adachi) override;
        void log_print();
    private:
        SensorData *sens;    
        MotionValues *val;
        Control *control;
        MazeMap *map;
        Adachi motion;
};

class Log1 : public UI
{
    public:
        void ptr_by_sensor(SensorData *_sens) override;
        void ptr_by_motion(MotionValues *_val) override;
        void ptr_by_control(Control *_control) override;
        void ptr_by_map(MazeMap *_map) override;
        void set_device_driver(std::shared_ptr<Drivers> driver) override;
        void main_task() override;
        void ref_by_motion(Adachi &_adachi) override;
        void log_print();
        void map_print();
        void map_output_txt();
    private:
        SensorData *sens;    
        MotionValues *val;
        Control *control;
        MazeMap *map;
        Adachi motion;
};

#endif // LOG_HPP