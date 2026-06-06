#include "search.hpp"

void Search::ptr_by_sensor(SensorData *_sens) { sens = _sens; }

void Search::ptr_by_motion(MotionValues *_val) { val = _val; }

void Search::ptr_by_control(Control *_control) { control = _control; }

void Search::ptr_by_map(MazeMap *_map) { map = _map; }

void Search::set_device_driver(std::shared_ptr<Drivers> driver){}

void Search::ref_by_motion(Adachi &_adachi) { motion = _adachi;}

void Search::main_task() // Task Number 0
{
    val->current.rad = 0.0;
    val->sum.len = 0.0;
    map->pos.x = 0;
    map->pos.y = 0;
    map->pos.dir = NORTH;
    
    // === オドメトリ初期化 ===
    // 実際のロボット位置（センサベース）とセル基準位置（真値）は異なる
    const float ROBOT_START_X = 0.045; // 45mm - ロボット実際のX位置
    const float ROBOT_START_Y = 0.030; // 30mm - ロボット実際のY位置（セル中心より15mm手前）
    const float CELL_CENTER_X = 0.045; // 45mm - セル(0,0)中心X
    const float CELL_CENTER_Y = 0.045; // 45mm - セル(0,0)中心Y
    
    // センサベース推定位置を実際のロボット位置で初期化
    control->odom.x_pos = ROBOT_START_X;
    control->odom.y_pos = ROBOT_START_Y;
    control->odom.theta = 0.0; // NORTH
    
    // 補正後オドメトリはセル中心位置で初期化（真値として扱う）
    control->odom.x_pos_corrected = CELL_CENTER_X;
    control->odom.y_pos_corrected = CELL_CENTER_Y;
    control->odom.theta_corrected = 0.0;
    
    // 注意: 誤差計算はMATLAB側で実施（ログリソース削減のため）
    
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

    val->current.rad = 0.0;
    val->sum.len = 0.0;
    map->pos.x = 0;
    map->pos.y = 0;
    map->pos.dir = NORTH;
    
    // === オドメトリ初期化 ===
    // 実際のロボット位置（センサベース）とセル基準位置（真値）は異なる
    const float ROBOT_START_X = 0.045; // 45mm - ロボット実際のX位置
    const float ROBOT_START_Y = 0.030; // 30mm - ロボット実際のY位置（セル中心より15mm手前）
    const float CELL_CENTER_X = 0.045; // 45mm - セル(0,0)中心X
    const float CELL_CENTER_Y = 0.045; // 45mm - セル(0,0)中心Y
    
    // センサベース推定位置を実際のロボット位置で初期化
    control->odom.x_pos = ROBOT_START_X;
    control->odom.y_pos = ROBOT_START_Y;
    control->odom.theta = 0.0; // NORTH
    
    // 補正後オドメトリはセル中心位置で初期化（真値として扱う）
    control->odom.x_pos_corrected = CELL_CENTER_X;
    control->odom.y_pos_corrected = CELL_CENTER_Y;
    control->odom.theta_corrected = 0.0;
    
    // 注意: 誤差計算はMATLAB側で実施（ログリソース削減のため）
    
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
