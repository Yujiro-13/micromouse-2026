#include "search.hpp"
#include "pose_init.hpp" // 自己位置初期化の一元化

void Search::ptr_by_sensor(SensorData *_sens) { sens = _sens; }

void Search::ptr_by_motion(MotionValues *_val) { val = _val; }

void Search::ptr_by_control(Control *_control) { control = _control; }

void Search::ptr_by_map(MazeMap *_map) { map = _map; }

void Search::set_device_driver(std::shared_ptr<Drivers> driver){}

void Search::ref_by_motion(Adachi &_adachi) { motion = _adachi;}

void Search::main_task() // Task Number 0
{
    reset_pose_to_start(control, val, map);
    
    map->flag = SEARCH;
    control->log_flag = TRUE;
    motion.init_maze();
    map->search_count_flag = TRUE;
    map->search_time = 0;
    motion.search_adachi(map->GOAL_X,map->GOAL_Y);
    control->log_flag = FALSE;
    map_write(map);

    vTaskDelay(1000 / portTICK_PERIOD_MS);

    // motion.search_adachi(0,0);
    map_write(map);
    
    //std::cout << "Search" << std::endl;
}

void AllSearch::ptr_by_sensor(SensorData *_sens) { sens = _sens; }

void AllSearch::ptr_by_motion(MotionValues *_val) { val = _val; }

void AllSearch::ptr_by_control(Control *_control) { control = _control; }

void AllSearch::ptr_by_map(MazeMap *_map) { map = _map; }

void AllSearch::set_device_driver(std::shared_ptr<Drivers> driver){}

void AllSearch::ref_by_motion(Adachi &_adachi) { motion = _adachi;}

void AllSearch::main_task() // Task Number 1
{
    /*val->max.acc = 4.0;
    val->max.vel = 0.4;
    val->end.vel = 0.4;*/

    reset_pose_to_start(control, val, map);
    
    map->flag = SEARCH;
    control->log_flag = TRUE;
    motion.init_maze();
    map->search_count_flag = TRUE;
    map->search_time = 0;
    motion.search_adachi2(map->GOAL_X,map->GOAL_Y);
    control->log_flag = FALSE;
    
    map_write(map);

    vTaskDelay(1000 / portTICK_PERIOD_MS);
    motion.search_adachi2(0, 0);
    map_write(map);
    //std::cout << "AllSearch" << std::endl;
}
