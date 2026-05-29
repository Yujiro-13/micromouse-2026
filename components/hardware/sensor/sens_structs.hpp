#ifndef SENS_STRUCT_HPP
#define SENS_STRUCT_HPP

typedef enum
{
    NOWALL = 0,
    WALL = 1,
    UNKNOWN = 2,
}t_exist_wall;

typedef struct 
{
    int f = 0;  //front
    int fl = 0; //front left
    int fr = 0; //front right
    int l = 0;  //left
    int r = 0;  //right
    int b = 0;  //back
}t_sens_dir;    //sensor direction data

typedef struct 
{
    bool l = false; //front
    bool fl = false;    //front left
    bool fr = false;    //front right
    bool r = false; //left
}t_wall_exist;  //wall exist data

typedef struct 
{
    t_sens_dir val;  //sensor value
    t_sens_dir d_val;    //sensor value difference
    t_sens_dir p_val;    //sensor value past
    t_sens_dir error;    //sensor value error
    t_sens_dir ref;  //sensor value reference
    t_sens_dir th_wall;  //wall threshold value
    t_sens_dir th_control;   //control threshold value
    t_sens_dir th_pillar;    //pillar threshold value
    t_sens_dir ref_pillar;    //pillar control threshold value
    t_wall_exist exist; //wall true or false
    t_wall_exist control_enable;  //control true or false
    t_wall_exist pillar_detected;  //pillar detected flag
    t_sens_dir pillar_error;  //pillar error value
    bool control;  //enable or disable
    t_sens_dir centor_front;  //center value
    t_sens_dir center_right;
    t_sens_dir center_left;
    t_sens_dir center_rear;
}t_wall_sens;  //wall sensor data

typedef struct 
{
    float yaw = 0; //gyro yaw
    float yaw_new = 0; //gyro yaw new
    float ref = 0; //gyro reference
    float degree = 0;
    float radian = 0;
}t_gyro;    //gyro data

typedef struct 
{
    float x = 0.0;  // IMUのX方向オフセット [m]（前後方向）
    float y = 0.0;  // IMUのY方向オフセット [m]（左右方向）
    float z = 0.0;  // IMUのZ方向オフセット [m]（上下方向）
}t_sensor_offset;

typedef struct 
{
    float y_ref = 0.0;         // Y軸加速度のバイアス
    t_sensor_offset offset;    // センサオフセット位置（回転中心からの距離）
}t_accel;   //accelerometer data

typedef struct 
{
    unsigned int angle = 0;
    t_sens_dir data;
    t_sens_dir locate;
    t_sens_dir p_locate;
    t_sens_dir diff_pulse;
    t_sens_dir diff_p_pulse;    
}t_enc;     //encoder data

typedef struct 
{
    t_wall_sens wall;
    t_gyro gyro;
    t_accel accel;
    t_enc enc;
    float BatteryVoltage = 4.0;
}t_sens_data;   //sensor data

#endif // SENS_STRUCT_HPP