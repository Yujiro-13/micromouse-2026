#ifndef SEARCH_HPP
#define SEARCH_HPP

//#include <iostream>
#include "ui.hpp"

class Search : public UI
{
    public:
        void ptr_by_sensor(SensorData *_sens) override;
        void ptr_by_motion(MotionValues *_val) override;
        void ptr_by_control(Control *_control) override;
        void ptr_by_map(MazeMap *_map) override;
        void set_device_driver(std::shared_ptr<Drivers> driver) override;
        void main_task() override;
        void ref_by_motion(Adachi &_adachi) override;
    private:
        SensorData *sens;    
        MotionValues *val;
        Control *control;
        MazeMap *map;
        Adachi motion;
};

class AllSearch : public UI
{
    public:
        void ptr_by_sensor(SensorData *_sens) override;
        void ptr_by_motion(MotionValues *_val) override;
        void ptr_by_control(Control *_control) override;
        void ptr_by_map(MazeMap *_map) override;
        void set_device_driver(std::shared_ptr<Drivers> driver) override;
        void main_task() override;
        void ref_by_motion(Adachi &_adachi) override;
    private:
        SensorData *sens;    
        MotionValues *val;
        Control *control;
        MazeMap *map;
        Adachi motion;
};

#endif // SEARCH_HPP