#include "test.hpp"
#include "esp_heap_caps.h"
#include "esp_system.h"

void Test::ptr_by_sensor(SensorData *_sens) { sens = _sens; }

void Test::ptr_by_motion(MotionValues *_val) { val = _val; }

void Test::ptr_by_control(Control *_control) { control = _control; }

void Test::ptr_by_map(MazeMap *_map) { map = _map; }

void Test::set_device_driver(std::shared_ptr<Drivers> driver) {}

void Test::ref_by_motion(Adachi &_adachi) { motion = _adachi; }

void Test::main_task() // Task Number 6
{
    control->log_flag = TRUE;
    //motion.check_enkaigei(); // ok
    motion.check_motor_duty(0.05, -0.05, 1000); // ok
    
    // 不感帯検出テスト
    //motion.detect_dead_zone(0.005, 0.10, 10, 1000); // 0.0 ~ 0.30 まで0.005刻みで10msごとにデューティ更新

    //motion.calibrate_wall_sensor_distance();
    // vel : 0.08 , ang_vel : 0.05

    //motion.calibrate_wall_th();
    
    //motion.back();
    control->log_flag = FALSE;
    std::cout << "Test" << std::endl;
}

void Test2::ptr_by_sensor(SensorData *_sens) { sens = _sens; }

void Test2::ptr_by_motion(MotionValues *_val) { val = _val; }

void Test2::ptr_by_control(Control *_control) { control = _control; }

void Test2::ptr_by_map(MazeMap *_map) { map = _map; }

void Test2::set_device_driver(std::shared_ptr<Drivers> driver) {}

void Test2::ref_by_motion(Adachi &_adachi) { motion = _adachi; }

void Test2::main_task() // Task Number 7
{
    control->log_flag = TRUE;
    
    // 飽和領域検出テスト
   // motion.detect_saturation_region(0.1, 0.05, 0.8, 1, 500); // 0.1 ~ 0.80 まで0.05刻みで1msごとにデューティ更新
    
    
    val->sum.len = 0.0;
    motion.offset2(); // 14mm
    motion.run_half(); // 45mm
    motion.run2();  // 1 90mm
    uint8_t i = 0;
    while (i < 15)
    {
        motion.run2();
        i++;
    }
    
    // motion.run2();  // 2
    // motion.run2();  // 3
    // motion.run2();  // 4
    // motion.run2();  // 5
    // motion.run2();  // 6
    // motion.run2();  // 7
    // motion.run();  // 8
    // motion.run();  // 9
    // motion.run();  // 10
    // motion.run();  // 11
    // motion.run();  // 12
    // motion.run();  // 13
    //motion.run();  // 14
    // motion.slalom_right(); // ok
    
    // === slalom_jerk（躍度制御スラローム）の使用例 ===
    // 9フェーズの時間配列と躍度値は micromouse.cpp で初期化・設定されています。
    // val->slalom_jerk_phase_ms[9]: 各フェーズの継続時間 [ms]
    //   phase[0]: 直進 (20ms)
    //   phase[1]: 躍加速 (15ms)
    //   phase[2]: 定速 (10ms)
    //   phase[3]: 躍減速 (15ms)
    //   phase[4]: 定速 (10ms)
    //   phase[5]: 躍減速 (15ms)
    //   phase[6]: 定速 (10ms)
    //   phase[7]: 躍加速 (15ms)
    //   phase[8]: 直進 (20ms)
    // val->slalom_jerk_value: 角躍度 [rad/s^3]
    // 躍度積分は Interrupt::calc_target() で実行されます。
    
    //motion.slalom_jerk(SLA_LEFT, val->slalom_jerk_value, val->slalom_jerk_phase_ms, 9);
    
    motion.stop(); // 45mm
    motion.turn_half();
    
    control->log_flag = FALSE;
    std::cout << "Test2" << std::endl;
}

void Test3::ptr_by_sensor(SensorData *_sens) { sens = _sens; }

void Test3::ptr_by_motion(MotionValues *_val) { val = _val; }

void Test3::ptr_by_control(Control *_control) { control = _control; }

void Test3::ptr_by_map(MazeMap *_map) { map = _map; }

void Test3::set_device_driver(std::shared_ptr<Drivers> driver) {}

void Test3::ref_by_motion(Adachi &_adachi) { motion = _adachi; }

void Test3::main_task() // Task Number 8
{
    val->current.rad = 0.0;
    val->current.vel = 0.0;
    val->current.len = 0.0;

    control->log_flag = TRUE;

    motion.turn_left_2(); // ok
    motion.turn_left_2();
    motion.turn_left_2();
    motion.turn_left_2();
    motion.turn_left_2();
    motion.turn_left_2();
    motion.turn_left_2();
    motion.turn_left_2();

    /*motion.offset2(); // 14mm
    motion.run_half(); // 45mm
    motion.run();  // 1 90mm
    motion.run();  // 2
    motion.run();  // 3
    motion.run();  // 4
    motion.run();  // 5
    motion.run();  // 6
    motion.run();  // 7
    motion.slalom_right(); // ok
    motion.run();  // 1 90mm
    motion.run();  // 2
    motion.run();  // 3
    motion.run();  // 4
    motion.run();  // 5
    motion.run();  // 6
    motion.run();  // 7
    motion.stop(); // ok
    motion.turn_half();*/

    control->log_flag = FALSE;
    std::cout << "Test3" << std::endl;
}

void Test4::ptr_by_sensor(SensorData *_sens) { sens = _sens; }

void Test4::ptr_by_motion(MotionValues *_val) { val = _val; }

void Test4::ptr_by_control(Control *_control) { control = _control; }

void Test4::ptr_by_map(MazeMap *_map) { map = _map; }

void Test4::set_device_driver(std::shared_ptr<Drivers> driver) {}

void Test4::ref_by_motion(Adachi &_adachi) { motion = _adachi; }

void Test4::main_task() // Task Number 9
{
    val->current.rad = 0.0;
    val->current.vel = 0.0;
    val->current.len = 0.0;

    control->log_flag = TRUE;
    motion.turn_right_2(); // ok
    motion.turn_right_2();
    motion.turn_right_2();
    motion.turn_right_2();
    motion.turn_right_2();
    motion.turn_right_2();
    motion.turn_right_2();
    motion.turn_right_2();


    /*motion.offset2(); // 14mm
    motion.run_half(); // 45mm
    motion.run();  // 1 90mm
    motion.run();  // 2
    motion.run();  // 3
    motion.run();  // 4
    motion.run();  // 5
    motion.run();  // 6
    motion.run();  // 7
    motion.slalom_left(); // ok
    motion.run();  // 1 90mm
    motion.run();  // 2
    motion.run();  // 3
    motion.run();  // 4
    motion.run();  // 5
    motion.run();  // 6
    motion.run();  // 7
    motion.stop(); // ok
    motion.turn_half();*/

    control->log_flag = FALSE;
    std::cout << "Test4" << std::endl;
}

void Test5::ptr_by_sensor(SensorData *_sens) { sens = _sens; }

void Test5::ptr_by_motion(MotionValues *_val) { val = _val; }

void Test5::ptr_by_control(Control *_control) { control = _control; }

void Test5::ptr_by_map(MazeMap *_map) { map = _map; }

void Test5::set_device_driver(std::shared_ptr<Drivers> driver) {}

void Test5::ref_by_motion(Adachi &_adachi) { motion = _adachi; }

void Test5::main_task() // Task Number 10
{
    control->log_flag = TRUE;
    //motion.set_pid_gain();       // ok
    //motion.set_wall_threshold(); // ok
    val->sum.len = 0.0;
    val->current.rad = 0.0;
    val->current.vel = 0.0;

    motion.offset2(); // 14mm
    motion.run_half(); // 45mm
    motion.slalom_jerk(SLA_LEFT, val->slalom_jerk_value, val->slalom_jerk_phase_ms, 9);
    motion.slalom_jerk(SLA_LEFT, val->slalom_jerk_value, val->slalom_jerk_phase_ms, 9);
    motion.slalom_jerk(SLA_LEFT, val->slalom_jerk_value, val->slalom_jerk_phase_ms, 9);
    motion.slalom_jerk(SLA_LEFT, val->slalom_jerk_value, val->slalom_jerk_phase_ms, 9);
    motion.slalom_jerk(SLA_LEFT, val->slalom_jerk_value, val->slalom_jerk_phase_ms, 9);
    motion.slalom_jerk(SLA_LEFT, val->slalom_jerk_value, val->slalom_jerk_phase_ms, 9);
    motion.slalom_jerk(SLA_LEFT, val->slalom_jerk_value, val->slalom_jerk_phase_ms, 9);
    motion.slalom_jerk(SLA_LEFT, val->slalom_jerk_value, val->slalom_jerk_phase_ms, 9);
    motion.stop(); // ok
    motion.turn_half();
    control->log_flag = FALSE;
    //std::cout << "Test" << std::endl;
}

void Test6::ptr_by_sensor(SensorData *_sens) { sens = _sens; }

void Test6::ptr_by_motion(MotionValues *_val) { val = _val; }

void Test6::ptr_by_control(Control *_control) { control = _control; }

void Test6::ptr_by_map(MazeMap *_map) { map = _map; }

void Test6::set_device_driver(std::shared_ptr<Drivers> driver) {}

void Test6::ref_by_motion(Adachi &_adachi) { motion = _adachi; }

void Test6::main_task() // Task Number 11
{
    val->current.rad = 0.0;
    val->current.len = 0.0;
    control->log_flag = TRUE;
    motion.offset2(); // 14mm
    motion.run_half(); // 45mm
    motion.slalom_right();  // 1
    motion.slalom_left();  // 2
    motion.slalom_right();  // 3
    motion.slalom_left();  // 4
    motion.slalom_right();  // 5
    // motion.run2();  // 6
    // motion.run2();  // 7
    // motion.run2();  // 8
    // motion.run2();  // 9
    // motion.run2();  // 10
    // motion.run2();  // 11
    // motion.run2();  // 12
    // motion.run2();  // 13
    //motion.run2();  // 14
    motion.stop(); // OK
    // motion.back();
    motion.turn_half();
    control->log_flag = FALSE;
    std::cout << "Test2" << std::endl;
}

void Test7::ptr_by_sensor(SensorData *_sens) { sens = _sens; }

void Test7::ptr_by_motion(MotionValues *_val) { val = _val; }

void Test7::ptr_by_control(Control *_control) { control = _control; }

void Test7::ptr_by_map(MazeMap *_map) { map = _map; }

void Test7::set_device_driver(std::shared_ptr<Drivers> driver) {}

void Test7::ref_by_motion(Adachi &_adachi) { motion = _adachi; }

void Test7::main_task() // Task Number 12
{
    // control->log_flag = TRUE;
    motion.wall_check(); // OK
    control->log_flag = FALSE;
    std::cout << "Test3" << std::endl;
}

void PerformanceTest::ptr_by_sensor(SensorData *_sens) { sens = _sens; }

void PerformanceTest::ptr_by_motion(MotionValues *_val) { val = _val; }

void PerformanceTest::ptr_by_control(Control *_control) { control = _control; }

void PerformanceTest::ptr_by_map(MazeMap *_map) { map = _map; }

void PerformanceTest::set_device_driver(std::shared_ptr<Drivers> driver) {}

void PerformanceTest::ref_by_motion(Adachi &_adachi) { motion = _adachi; }

void PerformanceTest::main_task() // Task Number 14 (Performance Test)
{
    printf("=== Starting Performance Test ===\n");

    // 迷路の初期化
    motion.init_maze();
    map->pos.x = 0;
    map->pos.y = 0;
    map->pos.dir = NORTH;
    map->flag = SEARCH;

    printf("Maze initialized. Starting benchmark...\n");

    // パフォーマンステストを実行
    motion.performance_test();

    printf("=== Performance Test Completed ===\n");
    std::cout << "PerformanceTest" << std::endl;
}

// システム同定用M系列信号データ（テスト用短縮版 - 100サンプル）
const float SystemIdentificationTest::translation_signal_left[100] = {
    -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0,
    1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0,
    -1.0, -1.0, 1.0, 1.0, -1.0, 1.0, -1.0, 1.0, 1.0, -1.0,
    1.0, -1.0, -1.0, 1.0, 1.0, -1.0, -1.0, -1.0, 1.0, -1.0,
    1.0, 1.0, -1.0, 1.0, -1.0, -1.0, 1.0, -1.0, 1.0, -1.0,
    -1.0, 1.0, 1.0, -1.0, 1.0, 1.0, 1.0, -1.0, -1.0, 1.0,
    -1.0, 1.0, -1.0, 1.0, 1.0, -1.0, 1.0, -1.0, -1.0, -1.0,
    1.0, -1.0, 1.0, 1.0, -1.0, 1.0, 1.0, 1.0, -1.0, -1.0,
    -1.0, -1.0, 1.0, 1.0, 1.0, 1.0, -1.0, 1.0, -1.0, 1.0,
    1.0, -1.0, -1.0, -1.0, -1.0, 1.0, -1.0, -1.0, 1.0, 1.0
};

const float SystemIdentificationTest::translation_signal_right[100] = {
    -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0,
    1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0,
    -1.0, -1.0, 1.0, 1.0, -1.0, 1.0, -1.0, 1.0, 1.0, -1.0,
    1.0, -1.0, -1.0, 1.0, 1.0, -1.0, -1.0, -1.0, 1.0, -1.0,
    1.0, 1.0, -1.0, 1.0, -1.0, -1.0, 1.0, -1.0, 1.0, -1.0,
    -1.0, 1.0, 1.0, -1.0, 1.0, 1.0, 1.0, -1.0, -1.0, 1.0,
    -1.0, 1.0, -1.0, 1.0, 1.0, -1.0, 1.0, -1.0, -1.0, -1.0,
    1.0, -1.0, 1.0, 1.0, -1.0, 1.0, 1.0, 1.0, -1.0, -1.0,
    -1.0, -1.0, 1.0, 1.0, 1.0, 1.0, -1.0, 1.0, -1.0, 1.0,
    1.0, -1.0, -1.0, -1.0, -1.0, 1.0, -1.0, -1.0, 1.0, 1.0
};

const float SystemIdentificationTest::rotation_signal_left[100] = {
    -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0,
    1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0,
    -1.0, -1.0, 1.0, 1.0, -1.0, 1.0, -1.0, 1.0, 1.0, -1.0,
    1.0, -1.0, -1.0, 1.0, 1.0, -1.0, -1.0, -1.0, 1.0, -1.0,
    1.0, 1.0, -1.0, 1.0, -1.0, -1.0, 1.0, -1.0, 1.0, -1.0,
    -1.0, 1.0, 1.0, -1.0, 1.0, 1.0, 1.0, -1.0, -1.0, 1.0,
    -1.0, 1.0, -1.0, 1.0, 1.0, -1.0, 1.0, -1.0, -1.0, -1.0,
    1.0, -1.0, 1.0, 1.0, -1.0, 1.0, 1.0, 1.0, -1.0, -1.0,
    -1.0, -1.0, 1.0, 1.0, 1.0, 1.0, -1.0, 1.0, -1.0, 1.0,
    1.0, -1.0, -1.0, -1.0, -1.0, 1.0, -1.0, -1.0, 1.0, 1.0
};

const float SystemIdentificationTest::rotation_signal_right[100] = {
    1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0,
    -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0,
    1.0, 1.0, -1.0, -1.0, 1.0, -1.0, 1.0, -1.0, -1.0, 1.0,
    -1.0, 1.0, 1.0, -1.0, -1.0, 1.0, 1.0, 1.0, -1.0, 1.0,
    -1.0, -1.0, 1.0, -1.0, 1.0, 1.0, -1.0, 1.0, -1.0, 1.0,
    1.0, -1.0, -1.0, 1.0, -1.0, -1.0, -1.0, 1.0, 1.0, -1.0,
    1.0, -1.0, 1.0, -1.0, -1.0, 1.0, -1.0, 1.0, 1.0, 1.0,
    -1.0, 1.0, -1.0, -1.0, 1.0, -1.0, -1.0, -1.0, 1.0, 1.0,
    1.0, 1.0, -1.0, -1.0, -1.0, -1.0, 1.0, -1.0, 1.0, -1.0,
    -1.0, 1.0, 1.0, 1.0, 1.0, -1.0, 1.0, 1.0, -1.0, -1.0
};

void SystemIdentificationTest::ptr_by_sensor(SensorData *_sens) { sens = _sens; }

void SystemIdentificationTest::ptr_by_motion(MotionValues *_val) { val = _val; }

void SystemIdentificationTest::ptr_by_control(Control *_control) { control = _control; }

void SystemIdentificationTest::ptr_by_map(MazeMap *_map) { map = _map; }

void SystemIdentificationTest::set_device_driver(std::shared_ptr<Drivers> driver) {}

void SystemIdentificationTest::ref_by_motion(Adachi &_adachi) { motion = _adachi; }

void SystemIdentificationTest::main_task() // Task Number 15 (System Identification Test)
{
    printf("=== System Identification Test ===\n");

    // コンパイル時設定の確認
    printf("Build configuration:\n");
    #ifdef FULLSIZE_SIGNALS_AVAILABLE
    printf("✓ Full size signals: AVAILABLE\n");
    #else
    printf("✗ Full size signals: NOT AVAILABLE\n");
    #endif

    #ifdef CONFIG_SPIRAM_USE
    printf("✓ PSRAM support: ENABLED\n");
    #else
    printf("⚠ PSRAM support: DISABLED\n");
    #endif

    // ログを有効にして実験データを記録
    control->log_flag = TRUE;

    printf("\nRunning Full Size Translation Model Identification\n");
    printf("Using signals from MATLAB-generated data\n");

    // フルサイズ並進モデル実験を実行
    run_fullsize_translation_identification();

    // 必要に応じて回転モデル実験も実行
    // run_fullsize_rotation_identification();

    control->log_flag = FALSE;

    printf("=== System Identification Test Completed ===\n");
    printf("Log data has been recorded for analysis.\n");
    printf("Use MATLAB tools in Mcode/ directory to analyze the results.\n");

    std::cout << "SystemIdentificationTest" << std::endl;
}

void SystemIdentificationTest::run_translation_identification()
{
    printf("--- Starting Translation Model Identification ---\n");
    printf("Robot will perform parallel wheel motion for system identification.\n");
    printf("Duration: %d seconds (100 samples @ 1ms)\n", 100 * SAMPLING_PERIOD_MS / 1000);

    // 初期状態をリセット
    val->sum.len = 0.0;
    val->current.rad = 0.0;

    // Motion.cppの関数を使用して並進モデル実験を実行
    motion.run_translation_identification(translation_signal_left, translation_signal_right, 100, SAMPLING_PERIOD_MS);

    printf("Translation identification completed.\n");
}

void SystemIdentificationTest::run_rotation_identification()
{
    printf("--- Starting Rotation Model Identification ---\n");
    printf("Robot will perform differential wheel motion for system identification.\n");
    printf("Duration: %d seconds (100 samples @ 1ms)\n", 100 * SAMPLING_PERIOD_MS / 1000);

    // 初期状態をリセット
    val->sum.len = 0.0;
    val->current.rad = 0.0;

    // Motion.cppの関数を使用して回転モデル実験を実行
    motion.run_rotation_identification(rotation_signal_left, rotation_signal_right, 100, SAMPLING_PERIOD_MS);

    printf("Rotation identification completed.\n");
}

bool SystemIdentificationTest::load_signal_from_file(const char* filename, float** signal_left, float** signal_right, int* num_samples)
{
    FILE* file = fopen(filename, "r");
    if (!file) {
        printf("Error: Cannot open file %s\n", filename);
        return false;
    }

    // ヘッダー行をスキップ
    char header[256];
    if (!fgets(header, sizeof(header), file)) {
        printf("Error: Cannot read header from %s\n", filename);
        fclose(file);
        return false;
    }

    // ファイルサイズを推定してデータ行数を計算
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    // ヘッダーを再度スキップ
    fgets(header, sizeof(header), file);

    // 概算行数を計算（1行約30バイトと仮定）
    int estimated_lines = (file_size - strlen(header)) / 30;

    // メモリ確保
    *signal_left = (float *)malloc(estimated_lines * sizeof(float));
    *signal_right = (float*)malloc(estimated_lines * sizeof(float));

    if (!*signal_left || !*signal_right) {
        printf("Error: Memory allocation failed\n");
        if (*signal_left) free(*signal_left);
        if (*signal_right) free(*signal_right);
        fclose(file);
        return false;
    }

    int count = 0;
    char line[128];

    printf("Loading signal data from %s...\n", filename);

    while (fgets(line, sizeof(line), file) && count < estimated_lines) {
        float time, left, right;
        if (sscanf(line, "%f,%f,%f", &time, &left, &right) == 3) {
            (*signal_left)[count] = left;
            (*signal_right)[count] = right;
            count++;

            // 進捗表示（1000サンプルごと）
            if (count % 1000 == 0) {
                printf("Loaded %d samples...\n", count);
            }
        }
    }

    fclose(file);
    *num_samples = count;

    printf("Successfully loaded %d samples from %s\n", count, filename);
    return true;
}

void SystemIdentificationTest::free_signal_data(float* signal_left, float* signal_right)
{
    if (signal_left) {
        free(signal_left);
    }
    if (signal_right) {
        free(signal_right);
    }
}

void SystemIdentificationTest::run_identification_from_file(const char* filename, const char* experiment_type)
{
    printf("--- Starting %s from file: %s ---\n", experiment_type, filename);

    float* file_signal_left = nullptr;
    float* file_signal_right = nullptr;
    int file_num_samples = 0;

    if (!load_signal_from_file(filename, &file_signal_left, &file_signal_right, &file_num_samples)) {
        printf("Failed to load signal from file: %s\n", filename);
        return;
    }

    printf("Successfully loaded %d samples from file\n", file_num_samples);
    printf("Experiment duration: %.1f seconds\n", file_num_samples * 0.001);

    // 初期状態をリセット
    val->sum.len = 0.0;
    val->current.rad = 0.0;

    // Motion.cppの関数を使用して実験を実行
    if (strstr(experiment_type, "Translation") != nullptr) {
        motion.run_translation_identification(file_signal_left, file_signal_right, file_num_samples, 1);
    } else if (strstr(experiment_type, "Rotation") != nullptr) {
        motion.run_rotation_identification(file_signal_left, file_signal_right, file_num_samples, 1);
    }

    // メモリ解放
    free_signal_data(file_signal_left, file_signal_right);

    printf("%s from file completed.\n", experiment_type);
}

void SystemIdentificationTest::run_embedded_translation_identification()
{
    printf("--- Starting Translation Model with Embedded Signals (1000 samples) ---\n");
    printf("Using signals generated from MATLAB translation_input.txt\n");
    printf("Experiment duration: 1.0 seconds\n");

    // 初期状態をリセット
    val->sum.len = 0.0;
    val->current.rad = 0.0;

    // 埋め込み信号を使用して実験を実行
    motion.run_translation_identification(
        system_identification_signals::translation_signal_left_1000,
        system_identification_signals::translation_signal_right_1000,
        system_identification_signals::FULL_SCALE_SAMPLES,
        system_identification_signals::SAMPLING_PERIOD_MS
    );

    printf("Embedded translation identification completed.\n");
}

void SystemIdentificationTest::run_embedded_rotation_identification()
{
    printf("--- Starting Rotation Model with Embedded Signals (1000 samples) ---\n");
    printf("Using signals generated from MATLAB rotation_input.txt\n");
    printf("Experiment duration: 1.0 seconds\n");

    // 初期状態をリセット
    val->sum.len = 0.0;
    val->current.rad = 0.0;

    // 埋め込み信号を使用して実験を実行
    motion.run_rotation_identification(
        system_identification_signals::rotation_signal_left_1000,
        system_identification_signals::rotation_signal_right_1000,
        system_identification_signals::FULL_SCALE_SAMPLES,
        system_identification_signals::SAMPLING_PERIOD_MS
    );

    printf("Embedded rotation identification completed.\n");
}

void SystemIdentificationTest::run_fullsize_translation_identification()
{
    printf("--- Starting Full Size Translation Model Identification ---\n");

    // デバッグ出力
    #ifdef FULLSIZE_SIGNALS_AVAILABLE
    printf("✓ FULLSIZE_SIGNALS_AVAILABLE is defined\n");
    #ifdef FULLSIZE_SIGNALS_PSRAM_AVAILABLE
    printf("✓ PSRAM support available\n");
    #else
    printf("⚠ PSRAM support not available, but continuing with fullsize signals\n");
    #endif
    #else
    printf("✗ FULLSIZE_SIGNALS_AVAILABLE is NOT defined\n");
    #endif

    #ifdef FULLSIZE_SIGNALS_AVAILABLE
    printf("Using fullsize signals from fullsize_signals.hpp\n");
    printf("Samples: %d\n", full_size_signals::TRANSLATION_SAMPLES);
    printf("Duration: %.1f seconds\n", full_size_signals::TRANSLATION_EXPERIMENT_DURATION_SEC);
    printf("Sampling period: %d ms\n", full_size_signals::SAMPLING_PERIOD_MS);

    // 初期状態をリセット
    val->sum.len = 0.0;
    val->current.rad = 0.0;

    // フルサイズ信号を使用して実験を実行
    motion.run_translation_identification(
        full_size_signals::translation_signal_left_45900,
        full_size_signals::translation_signal_right_45900,
        full_size_signals::TRANSLATION_SAMPLES,
        full_size_signals::SAMPLING_PERIOD_MS
    );

    printf("Full size translation identification completed.\n");
    #else
    printf("Full size signals not available. Using embedded signals instead.\n");
    run_embedded_translation_identification();
    #endif
}

void SystemIdentificationTest::run_fullsize_rotation_identification()
{
    printf("--- Starting Full Size Rotation Model Identification ---\n");

    #ifdef FULLSIZE_SIGNALS_AVAILABLE
    printf("Using fullsize signals from fullsize_signals.hpp\n");
    printf("Samples: %d\n", full_size_signals::ROTATION_SAMPLES);
    printf("Duration: %.1f seconds\n", full_size_signals::ROTATION_EXPERIMENT_DURATION_SEC);
    printf("Sampling period: %d ms\n", full_size_signals::SAMPLING_PERIOD_MS);

    // 初期状態をリセット
    val->sum.len = 0.0;
    val->current.rad = 0.0;

    // フルサイズ信号を使用して実験を実行
    motion.run_rotation_identification(
        full_size_signals::rotation_signal_left_45900,
        full_size_signals::rotation_signal_right_45900,
        full_size_signals::ROTATION_SAMPLES,
        full_size_signals::SAMPLING_PERIOD_MS
    );

    printf("Full size rotation identification completed.\n");
    #else
    printf("Full size signals not available. Using embedded signals instead.\n");
    run_embedded_rotation_identification();
    #endif
}



