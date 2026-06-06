#ifndef STRUCTS_HPP
#define STRUCTS_HPP


/* 
大きく４つの構造体グループに分けて参照渡しする。
1. センサー関連
2. マウス動作関連
3. 制御関連
4. マップ関連
*/

typedef enum
{
    FALSE = 0,
    TRUE = 1,
}Bool;

typedef enum
{
    SEARCH = 0,
    ALL_SEARCH = 1,
}SearchMode;

typedef enum
{
    FRONT = 0,
    RIGHT = 1,
    REAR = 2,
    LEFT = 3,
    SLA_LEFT = 4,
    SLA_RIGHT = 5,
    UNDEFINED,
}LocalDirection;

typedef enum
{
    NORTH = 0,
    EAST = 1,
    SOUTH = 2,
    WEST = 3,
}Direction;

/* === 1. センサー関連 ===
   旧 hardware/sensor/sens_structs.hpp を正本としてここへ統合（案B）。
   hardware 層は SensorData を関知せず生値 getter のみ公開し、
   mouse 層（wall_sensor / interrupt）がこれらへ詰め替える。 */

typedef enum
{
    NOWALL = 0,
    WALL = 1,
    UNKNOWN = 2,
}WallState;

typedef struct
{
    int f = 0;  //front
    int fl = 0; //front left
    int fr = 0; //front right
    int l = 0;  //left
    int r = 0;  //right
    int b = 0;  //back
}SensorDir;    //sensor direction data

typedef struct
{
    bool l = false; //front
    bool fl = false;    //front left
    bool fr = false;    //front right
    bool r = false; //left
}WallExist;  //wall exist data

typedef struct
{
    SensorDir val;  //sensor value
    SensorDir d_val;    //sensor value difference
    SensorDir p_val;    //sensor value past
    SensorDir error;    //sensor value error
    SensorDir ref;  //sensor value reference
    SensorDir th_wall;  //wall threshold value
    SensorDir th_control;   //control threshold value
    SensorDir th_pillar;    //pillar threshold value
    SensorDir ref_pillar;    //pillar control threshold value
    WallExist exist; //wall true or false
    WallExist control_enable;  //control true or false
    WallExist pillar_detected;  //pillar detected flag
    SensorDir pillar_error;  //pillar error value
    bool control;  //enable or disable
    SensorDir centor_front;  //center value
    SensorDir center_right;
    SensorDir center_left;
    SensorDir center_rear;
}WallSensorData;  //wall sensor data

typedef struct
{
    float yaw = 0; //gyro yaw
    float yaw_new = 0; //gyro yaw new
    float ref = 0; //gyro reference
    float degree = 0;
    float radian = 0;
}Gyro;    //gyro data

typedef struct
{
    float x = 0.0;  // IMUのX方向オフセット [m]（前後方向）
    float y = 0.0;  // IMUのY方向オフセット [m]（左右方向）
    float z = 0.0;  // IMUのZ方向オフセット [m]（上下方向）
}SensorOffset;

typedef struct
{
    float y_ref = 0.0;         // Y軸加速度のバイアス
    SensorOffset offset;    // センサオフセット位置（回転中心からの距離）
}Accel;   //accelerometer data

typedef struct
{
    unsigned int angle = 0;
    SensorDir data;
    SensorDir locate;
    SensorDir p_locate;
    SensorDir diff_pulse;
    SensorDir diff_p_pulse;
}EncoderData;     //encoder data

typedef struct
{
    WallSensorData wall;
    Gyro gyro;
    Accel accel;
    EncoderData enc;
    float battery_voltage = 4.0;
}SensorData;   //sensor data

typedef struct
{
    float vel = 0;  //velocity
    float ang_vel = 0;  //angular velocity
    float deg = 0;  //degree
    float rad = 0;  //radian
    float vel_error = 0; //error
    float ang_error = 0;    //angular error
    float acc = 0;  //acceleration
    float ang_acc = 0;  //angular acceleration
    float ang_jerk = 0; // angular jerk (rad/s^3)
    float len = 0;   //length
    float len_half = 0; //length half
    float wall_val = 0; //wall value
    float wall_error = 0;   //wall error
    float pillar_error = 0; //pillar error (for PD control)
    float alpha = 0.1;    //相補フィルタ用
    LocalDirection flag;
}MotionParam;  //motion parameter



typedef struct 
{
    MotionParam r;
    MotionParam l;
    MotionParam p;    //past
    MotionParam current;   //current
    MotionParam max;  //max
    MotionParam min;  //min
    MotionParam end;   //end
    MotionParam tar;   //target
    MotionParam sum;   //sum
    MotionParam I;    //integral
    MotionParam sla;  //slalom
    MotionParam sla_jerk; // slalom parameters dedicated for jerk-based control (ang_acc, ang_vel, ang_jerk)
    MotionParam fast_ref; // 最短走行時基準速
    MotionParam fast_high; // 既地区間加速時速
    float start_angle = 0.0;  // 直進開始時の角度保存用
    Bool angle_control_mode = FALSE;  // 角度制御モード（壁制御OFF時に有効）
    uint32_t slalom_jerk_phase_ms[9] = {0};  // slalom_jerk の9フェーズ時間配列 [ms]
    float slalom_jerk_value = 0.0f;  // slalom_jerk の躍度パラメータ [rad/s^3]
    Bool jerk_integration_enabled = FALSE;  // 躍度積分制御の有効フラグ
    float current_jerk = 0.0f;  // 現在の躍度値 [rad/s^3]（Interrupt内で積分用）
    uint32_t phase_timestamp_ms = 0;  // フェーズタイムスタンプ [ms]（Interruptループで更新）
}MotionValues;    //motion value

typedef struct 
{
    float tire_diameter = 0.0142;
    float tire_radius = 0.0071;
    float Rt = 3.11;
    float Kt = 0.0007677;
    float Ke = 0.0000804;
    float truque = 0;
    float revolutions = 0;
    float vBatt = 8.4;
    float m = 0.025;
    float n = 0.2195121;
    float cur = 0;
    float E = 0;
    float V_mot = 0;
}MotorParam;   //motor parameter

typedef struct 
{
    float Kp = 0;   //proportional gain
    float Ki = 0;   //integral gain
    float Kd = 0;   //differential gain
    float N = 0;    //filter coefficient
    float diff = 0; //differential term filter
    float D_operation_amount = 0; //differential operation amount
}PidParam; //pid parameter

typedef struct 
{
    // === センサベース推定位置（連続座標） ===
    float x_pos = 0.0;          // 推定X座標 [m]
    float y_pos = 0.0;          // 推定Y座標 [m]
    float theta = 0.0;          // 推定姿勢角 [rad]
    
    // === セル基準位置（真値） ===
    float x_cell_ref = 0.0;     // セル座標基準X位置 [m]
    float y_cell_ref = 0.0;     // セル座標基準Y位置 [m]
    float theta_cell_ref = 0.0; // セル座標基準姿勢角 [rad]
    
    // === リセット適用オドメトリ（セル補正後） ===
    float x_pos_corrected = 0.0;    // セル補正後X座標 [m]
    float y_pos_corrected = 0.0;    // セル補正後Y座標 [m]
    float theta_corrected = 0.0;    // セル補正後姿勢角 [rad]
    float x_error_corrected = 0.0;  // 補正後X誤差 [m]
    float y_error_corrected = 0.0;  // 補正後Y誤差 [m]
    float theta_error_corrected = 0.0;  // 補正後姿勢角誤差 [rad]
    float position_error_corrected = 0.0; // 補正後位置誤差ノルム [m]
    
    // === 位置誤差（生センサオドメトリ） ===
    float x_error = 0.0;        // X方向誤差 [m] (推定 - 真値)
    float y_error = 0.0;        // Y方向誤差 [m]
    float theta_error = 0.0;    // 姿勢角誤差 [rad]
    float position_error = 0.0; // 位置誤差ノルム [m] (√(x_error² + y_error²))
    
    // === セル内相対位置（推定値） ===
    float x_offset = 0.0;       // セル中心からのX方向オフセット [m]
    float y_offset = 0.0;       // セル中心からのY方向オフセット [m]
    
    // === 速度情報 ===
    float vel_x = 0.0;          // X方向速度 [m/s]
    float vel_y = 0.0;          // Y方向速度 [m/s]
    
    // === 統計情報（誤差追跡用） ===
    float max_position_error = 0.0;  // 最大位置誤差 [m]
    float cumulative_error = 0.0;    // 累積誤差 [m]
    uint32_t error_samples = 0;      // サンプル数
    
    // === 誤差推定（共分散） ===
    float cov_xx = 0.1;         // X位置の分散
    float cov_yy = 0.1;         // Y位置の分散
    float cov_tt = 0.01;        // 角度の分散
}Odometry;    //odometry data

typedef struct 
{
    PidParam v;    //velocity pid
    PidParam o;    //omega pid
    PidParam d;    //degree pid
    PidParam wall; //wall pid
    PidParam pillar;  //pillar pid
    float Vatt = 0;
    float V_l = 0;
    float V_r = 0;
    float Duty_l = 0;
    float Duty_r = 0;
    float test_Duty_l = 0;
    float test_Duty_r = 0;
    uint64_t time_count = 0;
    Bool flag = FALSE;
    Bool test_flag = FALSE;
    MotorParam mot;
    Odometry odom;
    Bool log_flag = FALSE;
    int64_t start_run_time = 0;
    int64_t end_run_time = 0;
    int64_t delta_run_time = 0;
    int64_t start_search_time = 0;
    int64_t end_search_time = 0;
    int64_t delta_search_time = 0;
    float diff = 0;
    float D_operation_amount = 0;
    
    // 静止状態補償用変数
    float static_friction_compensation_straight = 0.0;  // 静摩擦補償値（直進）
    float static_friction_compensation_turn = 0.0;  // 静摩擦補償値（超信地旋回）
    Bool is_stationary = TRUE;  // 静止状態フラグ
    float stationary_threshold_vel = 0.01;  // 静止判定閾値（速度） [m/s]
    float stationary_threshold_ang_vel = 0.01;  // 静止判定閾値（角速度） [rad/s]
    
    // === オドメトリ補正トリガー用 ===
    bool odom_correction_requested = false;  // 補正リクエストフラグ
    int correction_cell_x = 0;  // 補正時のセルX座標
    int correction_cell_y = 0;  // 補正時のセルY座標
    int correction_dir = 0;     // 補正時の方向 (0=NORTH, 1=EAST, 2=SOUTH, 3=WEST)
}Control; //control parameter


typedef struct 
{
    unsigned char north:2;
    unsigned char east:2;
    unsigned char south:2;
    unsigned char west:2;
    Bool flag;
}WallData;    //wall data

typedef struct
{
    short x = 0;
    short y = 0;
    Direction dir;
}Pos;     //position data

typedef struct 
{
    Pos pos;
    WallData wall[32][32];
    unsigned char size[32][32] = {0};
    uint8_t GOAL_X = 0;
    uint8_t GOAL_Y = 0;
    SearchMode flag;
    Bool search_count_flag = FALSE;
    uint64_t search_time = 0;
    Bool thinking_flag = FALSE;
}MazeMap;     //map data

typedef struct
{
    float speed_Kp = 0;
    float speed_Ki = 0;
    float speed_Kd = 0;
    float ang_vel_Kp = 0;
    float ang_vel_Ki = 0;
    float ang_vel_Kd = 0;
    float wall_Kp = 0;
    float wall_Ki = 0;
    float wall_Kd = 0;
}FilePidGain;  //parameter file

typedef struct
{
    uint16_t th_wall_fl = 0;
    uint16_t th_wall_l = 0;
    uint16_t th_wall_r = 0;
    uint16_t th_wall_fr = 0;
    uint16_t th_control_l = 0;
    uint16_t th_control_r = 0;
    uint16_t ref_l = 0;
    uint16_t ref_r = 0;
}FileWallThreshold;   //wall threshold file

typedef struct
{
    uint16_t front_l = 0;
    uint16_t front_r = 0;
    uint16_t left_fl = 0;
    uint16_t left_fr = 0;
    uint16_t right_fl = 0;
    uint16_t right_fr = 0;
    uint16_t rear_fl = 0;
    uint16_t rear_fr = 0;
}FileCenterSensValue;


#endif // STRUCTS_HPP