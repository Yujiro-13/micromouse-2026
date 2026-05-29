#ifndef FAST_HPP
#define FAST_HPP

//#include <iostream>
#include "ui.hpp"


class Fast : public UI
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

class Fast2 : public UI
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

class Fast3 : public UI
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

class Fast4 : public UI
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


#endif // FAST_HPP