#ifndef FAST_HPP
#define FAST_HPP

//#include <iostream>
#include "UI.hpp"


class Fast : public UI
{
    public:
        void ptr_by_sensor(t_sens_data *_sens) override;
        void ptr_by_motion(t_mouse_motion_val *_val) override;
        void ptr_by_control(t_control *_control) override;
        void ptr_by_map(t_map *_map) override;
        void set_device_driver(std::shared_ptr<t_drivers> driver) override;
        void main_task() override;
        void ref_by_motion(Adachi &_adachi) override;
    private:
        t_sens_data *sens;    
        t_mouse_motion_val *val;
        t_control *control;
        t_map *map;
        Adachi motion;
};

class Fast2 : public UI
{
    public:
        void ptr_by_sensor(t_sens_data *_sens) override;
        void ptr_by_motion(t_mouse_motion_val *_val) override;
        void ptr_by_control(t_control *_control) override;
        void ptr_by_map(t_map *_map) override;
        void set_device_driver(std::shared_ptr<t_drivers> driver) override;
        void main_task() override;
        void ref_by_motion(Adachi &_adachi) override;
    private:
        t_sens_data *sens;    
        t_mouse_motion_val *val;
        t_control *control;
        t_map *map;
        Adachi motion;
};

class Fast3 : public UI
{
    public:
        void ptr_by_sensor(t_sens_data *_sens) override;
        void ptr_by_motion(t_mouse_motion_val *_val) override;
        void ptr_by_control(t_control *_control) override;
        void ptr_by_map(t_map *_map) override;
        void set_device_driver(std::shared_ptr<t_drivers> driver) override;
        void main_task() override;
        void ref_by_motion(Adachi &_adachi) override;
    private:
        t_sens_data *sens;    
        t_mouse_motion_val *val;
        t_control *control;
        t_map *map;
        Adachi motion;
};

class Fast4 : public UI
{
    public:
        void ptr_by_sensor(t_sens_data *_sens) override;
        void ptr_by_motion(t_mouse_motion_val *_val) override;
        void ptr_by_control(t_control *_control) override;
        void ptr_by_map(t_map *_map) override;
        void set_device_driver(std::shared_ptr<t_drivers> driver) override;
        void main_task() override;
        void ref_by_motion(Adachi &_adachi) override;
    private:
        t_sens_data *sens;    
        t_mouse_motion_val *val;
        t_control *control;
        t_map *map;
        Adachi motion;
};


#endif // FAST_HPP