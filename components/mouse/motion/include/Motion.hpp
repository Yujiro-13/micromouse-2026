#ifndef MOTION_HPP
#define MOTION_HPP

#include <iostream>
#include <cstddef>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>    // freertos以下のファイルをインクルードしたい場合、必ず先にFreeRTOS.hをインクルードする
#include "Base_func.hpp"
#include "files.hpp"

// 壁センサの列挙型（距離推定用・4センサ個別）
enum class WallSensor {
    FL,  // 前左センサ（距離推定 + 姿勢角推定用）
    FR,  // 前右センサ（距離推定 + 姿勢角推定用）
    L,   // 左壁センサ（距離推定用）
    R    // 右壁センサ（距離推定用）
};

class Motion : public Micromouse
{
    public:
        Motion();
        ~Motion();
        void ptr_by_sensor(t_sens_data *sens) override;
        void ptr_by_motion(t_mouse_motion_val *val) override;
        void ptr_by_control(t_control *control) override;
        void ptr_by_map(t_map *map) override;
        void set_device_driver(std::shared_ptr<t_drivers> driver) override;
        void GetSemphrHandle(SemaphoreHandle_t *_on_logging);
        void run();
        void run2();
        void run_half();
        void turn_left();
        void turn_right();
        void turn_half();
        void stop();
        void stop2();
        void back();
        void slalom_left();
        void slalom_right();
        void slalom_time(uint8_t dir_flag, uint32_t accel_ms, uint32_t const_ms, uint32_t decel_ms);
    void slalom_jerk(uint8_t dir_flag, float jerk, const uint32_t *phase_ms, size_t phase_count);
        void check_enkaigei();
        void turn_left_2();
        void turn_right_2();
        void wall_check();
        void adjust_pid(const char* gain, float *pid, float step, uint8_t mode_num);
        void set_pid_gain();
        void adjust_wall_threshold(const char* threshold, uint16_t *th_value, uint8_t step, uint8_t mode_num);
        void set_wall_threshold();
        void offset();
        void offset2();
        void calibrate_wall_th();
        void fast_straight(uint8_t straight_count);
        void fast_stop(uint8_t straight_count);
        void CheckMotorDuty(float duty_l, float duty_r, uint32_t time);
        float CalcVelocity(float dis, float vel, float acc);
        void DetectDeadZone(float step_size = 0.001, float max_duty = 0.1, uint32_t update_rate = 1, uint32_t settle_time = 1000);
        void DetectSaturationRegion(float start_duty = 0.1, float step_size = 0.05, float max_duty = 0.8, uint32_t update_rate = 1, uint32_t settle_time = 2000);

        // システム同定実験用の関数
        void ApplySystemIdentificationSignal(const float* signal_left, const float* signal_right, int num_samples, int sampling_period_ms);
        void RunTranslationIdentification(const float* signal_left, const float* signal_right, int num_samples, int sampling_period_ms);
        void RunRotationIdentification(const float* signal_left, const float* signal_right, int num_samples, int sampling_period_ms);

        // 加速度再計算関数
        float RecalculateAcceleration(float current_position, float target_position, float current_velocity, float control_period = 0.001);
        float RecalculateAngularAcceleration(float current_angle, float target_angle, float current_angular_velocity, float control_period = 0.001);

        // 壁センサ距離推定用の関数
        void MeasureWallSensorDistance(uint32_t duration_ms = 5000);
        void CalibrateWallSensorDistance();
        float ConvertSensorValueToDistance(uint16_t sensor_value, WallSensor sensor);
        void TestWallDistanceConversion();

        
    protected:
        t_sens_data *sens;
        t_mouse_motion_val *val;
        t_control *control;
        t_map *map;
        SemaphoreHandle_t *on_logging;

        std::shared_ptr<NeoPixel> np;
        std::shared_ptr<MPU6500> imu;
        std::shared_ptr<PCA9632> led;
        std::shared_ptr<Buzzer> bz;
        std::shared_ptr<Motor> mot;
        std::shared_ptr<MA730> encL;
        std::shared_ptr<MA730> encR;
        std::shared_ptr<ADS7066> adc;

    private:
        uint8_t len_count = 0;
        float local_rad = 0.0;
        

};

#endif // MOTION_HPP