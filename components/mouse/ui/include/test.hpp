#ifndef TEST_HPP
#define TEST_HPP

//#include <iostream>
#include "ui.hpp"
#include "embedded_signals.hpp"

// フルサイズ信号ファイルのインクルード（signals コンポーネント）
#if __has_include("fullsize_signals.hpp")
    #include "fullsize_signals.hpp"
    #define FULLSIZE_SIGNALS_AVAILABLE
    #ifdef CONFIG_SPIRAM_USE
        #define FULLSIZE_SIGNALS_PSRAM_AVAILABLE
    #endif
#endif

class Test : public UI
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

class Test2 : public UI
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

class Test3 : public UI
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

class Test4 : public UI
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

class Test5 : public UI
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

class Test6 : public UI
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

class Test7 : public UI
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

class PerformanceTest : public UI
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

// システム同定実験用のテストクラス
class SystemIdentificationTest : public UI
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

        // システム同定用のメソッド
        void run_translation_identification();
        void run_rotation_identification();

        // ファイル読み込み関数（オプション）
        bool load_signal_from_file(const char* filename, float** signal_left, float** signal_right, int* num_samples);
        void free_signal_data(float* signal_left, float* signal_right);
        void run_identification_from_file(const char* filename, const char* experiment_type);

        // 埋め込み信号を使用した実験関数
        void run_embedded_translation_identification();
        void run_embedded_rotation_identification();

        // フルサイズ信号を使用した実験関数
        void run_fullsize_translation_identification();
        void run_fullsize_rotation_identification();

        // M系列信号データ（テスト用短縮版）
        static constexpr int TRANSLATION_SAMPLES = 100;
        static constexpr int ROTATION_SAMPLES = 100;
        static constexpr int SAMPLING_PERIOD_MS = 1;  // 1ms サンプリング周期

        // M系列信号データの宣言（実装で定義）
        static const float translation_signal_left[100];
        static const float translation_signal_right[100];
        static const float rotation_signal_left[100];
        static const float rotation_signal_right[100];
};

#endif // TEST_HPP