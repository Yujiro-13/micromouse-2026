
#include "interrupt.hpp"
#include "fast.hpp"
#include "log.hpp"
#include "search.hpp"
#include "test.hpp"
#include "adachi.hpp"
#include "include/micromouse.hpp"
#include "sens_structs.hpp"
#include <functional>
#include "task.hpp"

std::vector<std::shared_ptr<UI>> ui;

void run_micromouse(std::shared_ptr<Drivers> driver, SensorData *sens);
void set_interface();
void call_task(UI *task, Adachi &motion);
void set_param(Micromouse *task, SensorData *_sen, MotionValues *_val, Control *_control, MazeMap *_map);
void mode_select(uint8_t *_mode_num, Adachi &adachi, SensorData *sens, MotionValues *val, Control *control, MazeMap *map);

/*void myTaskInterrupt(void *pvpram)
{
    Interrupt *interrupt = static_cast<Interrupt *>(pvpram);
    interrupt->interrupt();
}*/

/*void myTaskAdc(void *pvpram)
{
    ADS7066 *adc = static_cast<ADS7066 *>(pvpram);
    adc->adc_loop();
}*/

/*void myTaskLog(void *pvpram)
{
    Interrupt *log = static_cast<Interrupt *>(pvpram);
    log->logging();
}*/

/* 基本的に全ての処理のをここにまとめ、mainで呼び出す。 */

void run_micromouse(std::shared_ptr<Drivers> driver, SensorData *sens)
{
    // printf("start run_micromouse\n");

    /* 構造体のインスタンス生成 */
    //SensorData sens;
    MotionValues val;
    Control control;
    MazeMap map;
    FilePidGain pid_gain;
    FileWallThreshold wall_threshold;
    FileCenterSensValue center_sens_val;

    printf("finish struct\n");

    /* ログ取得用ハンドルの設定 */
    SemaphoreHandle_t on_logging = xSemaphoreCreateBinary();

    /* ポインタの設定・構造体の共有 */

    // 制御系
    Interrupt interrupt;
    interrupt.set_device_driver(driver);
    printf("finish set device\n");
    interrupt.ptr_by_sensor(sens);
    interrupt.ptr_by_motion(&val);
    interrupt.ptr_by_control(&control);
    interrupt.ptr_by_map(&map);
    printf("finish pass pointer\n");
    interrupt.get_semphr_handle(&on_logging);

    printf("finish interrupt struct\n");

    // モーション系
    Adachi motion;
    motion.set_device_driver(driver);
    motion.ptr_by_sensor(sens);
    motion.ptr_by_motion(&val);
    motion.ptr_by_control(&control);
    motion.ptr_by_map(&map);
    motion.get_semphr_handle(&on_logging);

    printf("finish motion struct\n");

    // センサ系
    driver->adc->share_sensor_data(sens);
    driver->imu->share_sensor_data(sens);
    driver->encR->share_sensor_data(sens);
    driver->encL->share_sensor_data(sens);

    printf("finish sensor struct\n");

    /* パラメータの設定 */
    //pid_gain = read_file_pid();
    //wall_threshold = read_file_wall_th();
    //center_sens_val = read_file_center_sens_val();

    // 距離
    val.tar.len = 0.09;
    val.tar.len_half = 0.045;

    // 角度
    val.tar.rad = M_PI/2.0;

    // 速度
    //val.tar.acc = 0.5;
    val.max.acc = 2.5;
    //val.tar.vel = 0.3;
    val.max.vel = 0.25;
    val.min.vel = 0.04;
    val.end.vel = 0.25;

    // 角速度
    //val.tar.ang_acc = 0.0;
    val.max.ang_acc = M_PI*60.0;
    //al.tar.ang_vel = 0.0;
    val.max.ang_vel = M_PI*3.0;
    val.min.ang_vel = M_PI/4.0;
    val.end.ang_vel = 0.0;

    // スラロームパラメータ
    val.sla.ang_acc = 110.0;
    val.sla.ang_vel = 9.0;
    // 躍度パラメータ（角躍度: rad/s^3）
    // sla.ang_jerk はスラローム設計上の望ましい躍度制限、
    // max.ang_jerk は車体の物理制約に基づく上限として設定します。
    val.sla.ang_jerk = 3880.0; // 初期値：sla.ang_acc * 10 の目安
    val.max.ang_jerk = val.max.ang_acc * 5.0; // 機体上の最大躍度（適宜チューニング）
    

    // スラローム（躍度制御用）専用パラメータを既存のスラローム値に合わせて初期化
    // ここで別の初期値を設定すると、通常スラロームと躍度制御時の挙動を分離できます。
    val.sla_jerk.ang_acc = 143.6;
    val.sla_jerk.ang_vel = 9.331;
    val.sla_jerk.ang_jerk = val.sla.ang_jerk;

    // slalom_jerk の時間配列を初期化
    // ※ phase[0]（前距離）と phase[8]（後距離）は距離ベース管理のため、
    //    以下の配列値は使用されません（phase[1]～phase[7]のみ使用）。
    // phase[0]: 直進（距離管理：PRE_DISTANCE）
    // phase[1]: 躍加速            (15ms)
    // phase[2]: 定速              (10ms)
    // phase[3]: 躍減速            (15ms)
    // phase[4]: 定速              (10ms)
    // phase[5]: 躍減速            (15ms)
    // phase[6]: 定速              (10ms)
    // phase[7]: 躍加速            (15ms)
    // phase[8]: 直進（距離管理：FOL_DISTANCE）
    const uint32_t default_phase_ms[9] = {0, 37, 28, 37, 66, 37, 28, 37, 0};
    for (int i = 0; i < 9; ++i)
    {
        val.slalom_jerk_phase_ms[i] = default_phase_ms[i];
    }
    val.slalom_jerk_value = 3880.0f; // 躍度 [rad/s^3]（初期値：保守的な値）
    
    // 躍度積分制御フラグの初期化
    val.jerk_integration_enabled = FALSE;
    val.current_jerk = 0.0f;
    val.phase_timestamp_ms = 0;

    // 速度制御
    //control.v.Kp = pid_gain.speed_Kp;
    //control.v.Ki = pid_gain.speed_Ki;
    //control.v.Kd = pid_gain.speed_Kd;
    control.v.Kp = 20.26; // 20~30 10でもいいかも
    control.v.Ki = 353.3; // 100
    control.v.Kd = -0.003271;
    control.v.N = 1105;
    // 角速度制御
    //control.o.Kp = pid_gain.ang_vel_Kp;
    //control.o.Ki = pid_gain.ang_vel_Ki;
    //control.o.Kd = pid_gain.ang_vel_Kd;
    control.o.Kp = 0.20098; // 0.3,0.4でもあり
    control.o.Ki = 20.293; // 30
    control.o.Kd = 0.0;
    control.o.N = 0.0;
    // 高すぎるかもスピンしやすい

    // 壁制御
    //control.wall.Kp = pid_gain.wall_Kp;
    //control.wall.Ki = pid_gain.wall_Ki;
    //control.wall.Kd = pid_gain.wall_Kd;
    control.wall.Kp = 0.0003; //0.0001
    control.wall.Ki = 0.0;
    control.wall.Kd = 0.0000001;

    // 柱制御
    control.pillar.Kp = 0.0003;
    control.pillar.Ki = 0.0;
    control.pillar.Kd = 0.0000001;

    // 角度制御（壁制御OFF時に使用）
    control.d.Kp = 0.001;  // 角度維持用のPゲイン（要調整）
    control.d.Ki = 0.0;  // 角度維持用のIゲイン
    control.d.Kd = 0.0;  // 角度維持用のDゲイン


    // 静摩擦補償
    control.static_friction_compensation_straight = 0.08;  // 静止状態から直進加速時の補償Duty値（要調整）
    control.static_friction_compensation_turn = 0.08;  // 静止状態から超信地旋回時の補償Duty値（要調整）
    control.stationary_threshold_vel = 0.005;  // 静止判定の速度閾値 [m/s]
    control.stationary_threshold_ang_vel = 0.01;  // 静止判定の角速度閾値 [rad/s]

    // 壁センサ閾値
    //sens.wall.th_wall.fl = wall_threshold.th_wall_fl;
    //sens.wall.th_wall.fr = wall_threshold.th_wall_fr;
    //sens.wall.th_wall.l = wall_threshold.th_wall_l;
    //sens.wall.th_wall.r = wall_threshold.th_wall_r;
    //sens.wall.th_control.l = wall_threshold.th_control_l;
    //sens.wall.th_control.r = wall_threshold.th_control_r;
    //sens.wall.ref.l = wall_threshold.ref_l;
    //sens.wall.ref.r = wall_threshold.ref_r;
    sens->wall.th_wall.fl = 1650; //2000
    sens->wall.th_wall.fr = 1660; //2000
    sens->wall.th_wall.l = 4000;  //4000
    sens->wall.th_wall.r = 4000;  //4000
    sens->wall.th_pillar.l = 2000; // 柱検出の閾値。壁制御の閾値より大きく、これより大きいと柱と判断する。
    sens->wall.th_pillar.r = 2000;
    sens->wall.th_control.l = 9050; // 壁制御が入るか否かの閾値。これより大きいと壁制御が有効化。なるべく大きい値に設定するのが望ましい
    sens->wall.th_control.r = 7050;
    sens->wall.ref_pillar.l = 3750; // 柱制御の目標値
    sens->wall.ref_pillar.r = 3600;
    sens->wall.ref.l = 10550; // 壁制御の目標値。壁から離れるほど値が小さく、近づくほど値が大きい。壁から離れてほしいときは小さく設定。
    sens->wall.ref.r = 8150;

    // 3612
    // 3769

    //10550
    //10150

    // ゴール座標
    map.GOAL_X = 13;
    map.GOAL_Y = 16;

    ADS7066 *adc = driver->adc.get();

    printf("finish parameter\n"); // ここまでOK
    // タスク優先順位 1 ~ 25    25が最高優先度
    xTaskCreatePinnedToCore(myTaskInterrupt,
                            "interrupt", 8192, &interrupt, configMAX_PRIORITIES - 1, NULL, APP_CPU_NUM);
    printf("finish interrupt task\n");
    //xTaskCreatePinnedToCore(myTaskAdc,
                            //"adc", 8192, &adc, configMAX_PRIORITIES - 2, NULL, APP_CPU_NUM);
    xTaskCreatePinnedToCore(myTaskLog,
                            "log", 8192, &interrupt, configMAX_PRIORITIES - 3, NULL, APP_CPU_NUM);

    //xTaskCreatePinnedToCore(myTaskNeoPixel,
                            //"nepixel", 8192, &driver, configMAX_PRIORITIES - 24, NULL, APP_CPU_NUM); // driver ごと渡すにはサイズが大きすぎるかも
    //printf("finish task\n");

    /*char buffer[512];
    vTaskList(buffer);
    printf("Task execution statistics:\n%s", buffer);*/

    uint8_t mode = 0;
    uint16_t time_count = 0;
    const int MODE_MAX = 0b1111;
    const int MODE_MIN = 0;
    control.flag = FALSE;

    /* メインループ */
    //printf("start main loop\n");
    while (1)
    {
        
        driver->led->set(mode + 1);

        /*vTaskList(buffer);
        printf("Task execution statistics:\n%s", buffer);*/

        if (sens->wall.val.fl + sens->wall.val.l + sens->wall.val.r + sens->wall.val.fr > 100000)
        {

            driver->led->set(0b1111);
            sens->gyro.ref = driver->imu->surveybias(2000);
            sens->accel.y_ref = driver->imu->surveybias_accel_y(2000);
            
            // IMUセンサオフセット位置の設定（回転中心からの距離）
            // x = 15.036mm, y = 21.044mm, z = 0mm
            sens->accel.offset.x = 0.015036;  // [m] 前方向
            sens->accel.offset.y = 0.021044;  // [m] 右方向
            sens->accel.offset.z = 0.0;       // [m] 上方向
            
            mode_select(&mode, motion, sens, &val, &control, &map);
            control.flag = FALSE;
        }
        /*if (time_count > 500)
        {
            driver->led->set(0b1111);
            sens.gyro.ref = driver->imu->surveybias(2000);
            sens.accel.y_ref = driver->imu->surveybias_accel_y(2000);
            mode_select(&mode, motion, &sens, &val, &control, &map);
            control.flag = FALSE;
            time_count = 0;
        }*/

        if (val.current.vel > 0.04)
        {
            if (mode >= MODE_MAX)
            {
                mode = MODE_MIN;
            }
            else
            {
                mode++;
            }
            time_count = 0;
            vTaskDelay(pdMS_TO_TICKS(500));
        }
        if (val.current.vel < -0.04)
        {
            if (mode <= MODE_MIN)
            {
                mode = MODE_MAX;
            }
            else
            {
                mode--;
            }
            time_count = 0;
            vTaskDelay(pdMS_TO_TICKS(500));
        }

        /* ログ出力したいときは、以下のprintfを全てコメントアウトしておく*/
        //printf("mode: %d\n", mode); //OK
        //printf("time: %d\n", control.time_count); OK
        //printf("vel: %f\n", val.current.vel); OK
        //printf("rad: %f\n", val.current.rad); OK
        //printf("battery_voltage: %f\n", sens->battery_voltage); //OK
        //printf("sens.wall.val.fl: %d  sens.wall.val.l: %d  sens.wall.val.r: %d  sens.wall.val.fr: %d\n", sens->wall.val.fl, sens->wall.val.l, sens->wall.val.r, sens->wall.val.fr); //OK
        //printf("time:%d  mode:%d  flag:%d  Duty_L:%lf  Duty_R:%lf  Batt:%lf\n", time_count, mode, control.flag ,control.Duty_l, control.Duty_r, sens.battery_voltage);
        time_count++;
        vTaskDelay(10/portTICK_PERIOD_MS);
    }
    //vTaskDelay(pdMS_TO_TICKS(10));
}

void set_interface()
{
    /* クラスのポインタを配列に保持*/

    ui.push_back(std::make_shared<Search>()); // 0
    ui.push_back(std::make_shared<AllSearch>()); // 1
    ui.push_back(std::make_shared<Fast>()); // 2
    ui.push_back(std::make_shared<Fast2>()); // 3
    ui.push_back(std::make_shared<Fast3>()); // 4
    ui.push_back(std::make_shared<Fast4>()); // 5
    ui.push_back(std::make_shared<Test>()); // 6
    ui.push_back(std::make_shared<Test2>()); // 7
    ui.push_back(std::make_shared<Test3>()); // 8
    ui.push_back(std::make_shared<Test4>()); // 9
    ui.push_back(std::make_shared<Test5>()); // 10
    ui.push_back(std::make_shared<Test6>()); // 11
    ui.push_back(std::make_shared<Test7>()); // 12
    ui.push_back(std::make_shared<Log>()); // 13
    ui.push_back(std::make_shared<Log1>()); // 14
    ui.push_back(std::make_shared<SystemIdentificationTest>()); // 15 (System Identification Test)
    // ui.push_back(std::make_shared<PerformanceTest>()); // 16 (Performance Test) - 一時的にコメントアウト

    //std::cout << "set_interface" << std::endl;
}

void call_task(UI *task, Adachi &motion)
{
    task->ref_by_motion(motion);
    task->main_task();
    //std::cout << "call_task" << std::endl;
}

void set_param(Micromouse *task, SensorData *_sen, MotionValues *_val, Control *_control, MazeMap *_map)
{
    task->ptr_by_sensor(_sen);
    task->ptr_by_motion(_val);
    task->ptr_by_control(_control);
    task->ptr_by_map(_map);
    //std::cout << "set_param" << std::endl;
}

void mode_select(uint8_t *_mode_num, Adachi &adachi, SensorData *sens, MotionValues *val, Control *control, MazeMap *map)
{
    set_interface();
    set_param(ui[*_mode_num].get(), sens, val, control, map);
    call_task(ui[*_mode_num].get(), adachi);
    //std::cout << "mode_select" << std::endl;
}
