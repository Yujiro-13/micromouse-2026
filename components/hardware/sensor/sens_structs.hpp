#ifndef SENS_STRUCT_HPP
#define SENS_STRUCT_HPP

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
    float BatteryVoltage = 4.0;
}SensorData;   //sensor data

#endif // SENS_STRUCT_HPP