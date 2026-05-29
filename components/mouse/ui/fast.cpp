
#include "fast.hpp"

void Fast::ptr_by_sensor(t_sens_data *_sens) { sens = _sens; }

void Fast::ptr_by_motion(t_mouse_motion_val *_val) { val = _val; }

void Fast::ptr_by_control(t_control *_control) { control = _control; }

void Fast::ptr_by_map(t_map *_map) { map = _map; }

void Fast::set_device_driver(std::shared_ptr<t_drivers> driver) {}

void Fast::ref_by_motion(Adachi &_adachi) { motion = _adachi; } // ここでのポインタ渡しを忘れていて、InitMazeが正しく行えず、map_readがオーバーフローした

void Fast::main_task() // Task Number 2
{
    // 通常探索->折返し重ね探索
    val->current.rad = 0.0;
    val->sum.len = 0.0;
    map->pos.x = 0;
    map->pos.y = 0;
    map->pos.dir = NORTH;

    // === オドメトリ初期化（スタート位置：セル(0,0)中心） ===
    const float CELL_CENTER = 0.045; // 45mm
    
    // センサベース推定位置を初期化
    control->odom.x_pos = CELL_CENTER;
    control->odom.y_pos = 0.030;
    control->odom.theta = 0.0; // NORTH
    
    // 真値位置も同じく初期化
    control->odom.x_cell_ref = CELL_CENTER;
    control->odom.y_cell_ref = CELL_CENTER;
    control->odom.theta_cell_ref = 0.0;
    
    // 誤差をゼロリセット
    control->odom.x_error = 0.0;
    control->odom.y_error = 0.0;
    control->odom.theta_error = 0.0;
    control->odom.position_error = 0.0;
    control->odom.x_offset = 0.0;
    control->odom.y_offset = 0.0;
    
    // 統計情報をリセット
    control->odom.max_position_error = 0.0;
    control->odom.cumulative_error = 0.0;
    control->odom.error_samples = 0;

    map->flag = SEARCH;
    control->log_flag = TRUE;
    motion.InitMaze();
    map->search_count_flag = TRUE;
    map->search_time = 0;
    motion.search_adachi_sla(map->GOAL_X, map->GOAL_Y);
    control->log_flag = FALSE;

    map_write(map);
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    motion.search_adachi_sla(0, 0);
    map_write(map);
}

void Fast2::ptr_by_sensor(t_sens_data *_sens) { sens = _sens; }

void Fast2::ptr_by_motion(t_mouse_motion_val *_val) { val = _val; }

void Fast2::ptr_by_control(t_control *_control) { control = _control; }

void Fast2::ptr_by_map(t_map *_map) { map = _map; }

void Fast2::set_device_driver(std::shared_ptr<t_drivers> driver) {}

void Fast2::ref_by_motion(Adachi &_adachi) { motion = _adachi; }

void Fast2::main_task() // Task Number 3
{
    // 通常探索->折返し重ね全面探索
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

    map->flag = SEARCH;
    control->log_flag = TRUE;
    motion.InitMaze();
    map->search_count_flag = TRUE;
    map->search_time = 0;
    motion.search_adachi_sla(map->GOAL_X, map->GOAL_Y);
    control->log_flag = FALSE;

    map_write(map);
    vTaskDelay(1000 / portTICK_PERIOD_MS);

    map->flag = ALL_SEARCH;
    motion.search_adachi_sla(0, 0);
    map_write(map);
}

void Fast3::ptr_by_sensor(t_sens_data *_sens) { sens = _sens; }

void Fast3::ptr_by_motion(t_mouse_motion_val *_val) { val = _val; }

void Fast3::ptr_by_control(t_control *_control) { control = _control; }

void Fast3::ptr_by_map(t_map *_map) { map = _map; }

void Fast3::set_device_driver(std::shared_ptr<t_drivers> driver) {}

void Fast3::ref_by_motion(Adachi &_adachi) { motion = _adachi; }

void Fast3::main_task() // Task Number 4
{
    // 通常最短
    val->current.rad = 0.0;
    val->sum.len = 0.0;
    
    // 地図を読み込む
    *map = map_read();
    
    // 手動でロボットをスタート地点に戻した後なので、位置と向きを正しく設定
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

    map->flag = SEARCH;
    control->log_flag = TRUE;
    map->search_count_flag = TRUE;
    map->search_time = 0;
    motion.fast_run_sla(map->GOAL_X, map->GOAL_Y);
    control->log_flag = FALSE;
}

void Fast4::ptr_by_sensor(t_sens_data *_sens) { sens = _sens; }

void Fast4::ptr_by_motion(t_mouse_motion_val *_val) { val = _val; }

void Fast4::ptr_by_control(t_control *_control) { control = _control; }

void Fast4::ptr_by_map(t_map *_map) { map = _map; }

void Fast4::set_device_driver(std::shared_ptr<t_drivers> driver) {}

void Fast4::ref_by_motion(Adachi &_adachi) { motion = _adachi; }

void Fast4::main_task() // Task Number 5
{
    // 最短（既地区間加速）

    val->fast_ref.vel = 0.25;
    val->max.acc = 2.0; // 加減速で分けたほうがいいかも

    val->current.rad = 0.0;
    val->sum.len = 0.0;
    
    // 地図を読み込む
    *map = map_read();
    
    // 手動でロボットをスタート地点に戻した後なので、位置と向きを正しく設定
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

    map->flag = SEARCH;
    control->log_flag = TRUE;
    map->search_count_flag = TRUE;
    map->search_time = 0;
    motion.fast_run_sla2(map->GOAL_X, map->GOAL_Y);
    control->log_flag = FALSE;
}
