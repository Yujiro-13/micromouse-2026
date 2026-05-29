#ifndef INTERRUPT_HPP
#define INTERRUPT_HPP

#include <iostream>
#include <string.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>    // freertos以下のファイルをインクルードしたい場合、必ず先にFreeRTOS.hをインクルードする
#include "esp_flash_spi_init.h"
#include "esp_partition.h"
#include "esp_log.h"
#include "esp_flash.h"
#include "spi_flash_mmap.h"
#include "structs.hpp"
#include "Base_func.hpp"
#include "drivers.hpp"

constexpr int ENC_MAX = 16384;
constexpr int ENC_HALF = 8192;

class Interrupt : public Micromouse{
    public:
        Interrupt();
        ~Interrupt();
        void interrupt();
        void ptr_by_sensor(SensorData *sens) override;
        void ptr_by_motion(MotionValues *val) override;
        void ptr_by_control(Control *control) override;
        void ptr_by_map(MazeMap *map) override;
        void set_device_driver(std::shared_ptr<Drivers> driver) override;
        void get_semphr_handle(SemaphoreHandle_t *_on_logging);
        void reset_i_gain();
        void logging();
    private:
        void calc_target();
        void wall_control();
        void feedback_control();
        void calc_distance();
        void calc_angle();
        float calc_target_accel();
        void estimate_velocity_fusion();  // エンコーダ+IMU融合速度推定
        float compensate_centripetal_acceleration(float accel_y_raw); // 向心加速度補正
        float ff_control_velocity(float r_in); // 速度フィードフォワード制御
        float ff_control_angular_velocity(float r_in); // 角速度フィードフォワード制御
        
        // オドメトリ関連メソッド
        void update_odometry();                    // オドメトリ更新
        void update_cell_reference_position();     // セル基準位置を更新
        void calculate_position_error();           // 位置誤差を計算
        void sync_position_from_cell();            // セル座標から推定位置を同期
        void reset_odometry_to_cell_center();      // セル中心にリセット
        void apply_cell_correction();              // セル位置によるオドメトリ補正を適用
        void calculate_corrected_position_error(); // 補正後位置誤差を計算
        
        SensorData *sens;
        MotionValues *val;
        Control *control;
        MazeMap *map;
        float _accel = 0.0;
        float _vel = 0.0;
        float max_vel = 0.0;
        float max_ang_vel = 0.0;
        float target_acc = 0.0;
        float sum_len = 0.0;

        int64_t start_time = 0;
        int64_t end_time = 0;
        int64_t delta_time = 0;

        // 速度FF制御用配列
        float r[3] = {0.0, 0.0, 0.0};
        float u[3] = {0.0, 0.0, 0.0};
        
        // 角速度FF制御用配列
        float r_ang[3] = {0.0, 0.0, 0.0};
        float u_ang[3] = {0.0, 0.0, 0.0};
        
        // 加速度センサ関連（30ms移動平均用）
        static constexpr int ACCEL_MA_SIZE = 30;  // 30ms分（1ms周期想定）
        float accel_y_buffer[ACCEL_MA_SIZE] = {0.0};
        int accel_buffer_index = 0;
        float accel_y_raw = 0.0;        // 生の加速度値
        float accel_y_filtered = 0.0;   // 移動平均後の加速度値
        
        // 角加速度計算用
        float prev_ang_vel = 0.0;         // 前回の角速度 [rad/s]
        float ang_accel = 0.0;            // 角加速度 [rad/s²]
        float ang_accel_filtered = 0.0;   // フィルタ後の角加速度 [rad/s²]

        std::shared_ptr<NeoPixel> np;
        std::shared_ptr<MPU6500> imu;
        std::shared_ptr<PCA9632> led;
        std::shared_ptr<Buzzer> bz;
        std::shared_ptr<Motor> mot;
        std::shared_ptr<MA730> encL;
        std::shared_ptr<MA730> encR;
        std::shared_ptr<ADS7066> adc;

        SemaphoreHandle_t *on_logging;




};





#endif //INTERRUPT_HPP