#include "Motion.hpp"
#include <algorithm>

#define MODE_MAX 15
#define MODE_MIN 0
#define SECTION 0.09
#define SECTION_HALF 0.045
#define TURN_HALF M_PI
#define TURN_QUARTER M_PI / 2.0
#define OFFSET_DISTANCE 0.014
#define FRONT_WALL_LIMIT_FL 25100
#define FRONT_WALL_LIMIT_FR 14600
#define DONE 1
#define NOT_YET 0
#define PRE_DISTANCE 0.0049
#define FOL_DISTANCE 0.0054

static Buzzer::buzzer_score_t pc98[] = {{2000, 100}, {1000, 100}};
static Buzzer::buzzer_score_t pc98_2[] = {{1000, 100}, {2000, 100}};

Motion::Motion()
{ /*std::cout << "Motion" << std::endl;*/
}

Motion::~Motion() {}

void Motion::ptr_by_sensor(SensorData *_sens) { sens = _sens; }

void Motion::ptr_by_motion(MotionValues *_val) { val = _val; }

void Motion::ptr_by_control(Control *_control) { control = _control; }

void Motion::ptr_by_map(MazeMap *_map) { map = _map; }

void Motion::set_device_driver(std::shared_ptr<Drivers> driver)
{
    np = driver->np;
    imu = driver->imu;
    led = driver->led;
    bz = driver->bz;
    mot = driver->mot;
    encL = driver->encL;
    encR = driver->encR;
    adc = driver->adc;
    // std::cout << "set_device_driver" << std::endl;
}

void Motion::get_semphr_handle(SemaphoreHandle_t *_on_logging) { on_logging = _on_logging; }

void Motion::run()
{
    // control->delta_run_time = 0;
    // control->start_run_time = esp_timer_get_time();

    sens->wall.control = TRUE; // 壁制御ON

    val->I.vel_error = 0.0;
    val->I.ang_error = 0.0;
    val->I.wall_error = 0.0;

    val->tar.ang_acc = 0.0;
    val->tar.ang_vel = 0.0;
    val->current.flag = FRONT; // 直進モードに設定

    val->current.len = 0.0;
    val->tar.acc = val->max.acc;
    val->tar.len = SECTION;

    // 角度制御の準備（壁制御がFALSEの場合に使用）
    val->start_angle = val->current.rad;
    val->angle_control_mode = TRUE;

    control->flag = TRUE; // 制御ON

    // led->set(0b0000);

    while (((val->tar.len - 0.01) - val->current.len) > (((val->tar.vel) * (val->tar.vel) - (val->end.vel) * (val->end.vel)) / (2.0 *
                                                                                                                                val->tar.acc)))
    {
        vTaskDelay(1 / portTICK_PERIOD_MS);
    }

    // std::cout << "##### deceleration #####" << std::endl;
    // val->tar.acc = -(val->max.acc);

    while ((val->tar.len) > val->current.len)
    {
        if (val->tar.vel >= val->max.vel)
        {
            val->tar.acc = 0;
            val->tar.vel = val->max.vel;
        }
        vTaskDelay(1 / portTICK_PERIOD_MS);
    }

    // val->tar.vel = val->tar.vel;
    val->tar.acc = 0.0;
    // np->set_hsv({0, 100, 100}, 0, 1);
    // np->show();

    // control->flag = FALSE;
    // led->set(0b1111);

    // control->end_run_time = esp_timer_get_time();
    // control->delta_run_time = control->end_run_time - control->start_run_time;

    // === オドメトリ補正トリガー ===
    // run完了後、現在のセル座標で補正を要求
    control->odom_correction_requested = true;
    control->correction_cell_x = map->pos.x;
    control->correction_cell_y = map->pos.y;
    control->correction_dir = map->pos.dir;

    // std::cout << "run" << std::endl;
}

void Motion::run2()
{
    // === オドメトリ補正トリガー ===
    // run2完了後、現在のセル座標で補正を要求
    control->odom_correction_requested = true;
    control->correction_cell_x = map->pos.x;
    control->correction_cell_y = map->pos.y;
    control->correction_dir = map->pos.dir;

    map->thinking_flag = FALSE;

    sens->wall.control = TRUE; // 壁制御ON

    val->I.vel_error = 0.0;
    val->I.ang_error = 0.0;
    val->I.wall_error = 0.0;

    val->tar.ang_acc = 0.0;
    val->tar.ang_vel = 0.0;
    val->current.flag = FRONT; // 直進モードに設定

    val->current.len = 0.0;
    val->tar.acc = val->max.acc;
    val->tar.len = SECTION;
    // val->tar.vel = 0.0;

    bool l_wall_check = sens->wall.exist.l;
    bool r_wall_check = sens->wall.exist.r;
    bool hosei_flag = FALSE;

    // 角度制御の準備（壁制御がFALSEの場合に使用）
    val->start_angle = val->current.rad;
    val->angle_control_mode = TRUE;

    control->flag = TRUE; // 制御ON

    while (((val->tar.len - 0.01) - val->current.len) > (((val->tar.vel) * (val->tar.vel) - (val->end.vel) * (val->end.vel)) / (2.0 *
                                                                                                                                val->tar.acc)))
    {
        // 壁あり->壁なし
        if (sens->wall.exist.l == FALSE && l_wall_check == TRUE && hosei_flag == FALSE)
        {
            bz->play_melody(pc98, 2);
            val->current.len = 0.052;
            hosei_flag = TRUE;
        }

        if (sens->wall.exist.r == FALSE && r_wall_check == TRUE && hosei_flag == FALSE)
        {
            bz->play_melody(pc98, 2);
            val->current.len = 0.054;
            hosei_flag = TRUE;
        }

        // 壁なし->壁あり
        if (sens->wall.exist.l == TRUE && l_wall_check == FALSE && hosei_flag == FALSE)
        {
            bz->play_melody(pc98_2, 2);
            val->current.len = 0.038; // 補正後の距離を伸ばしたい場合は、値を小さく
            hosei_flag = TRUE;
        }

        if (sens->wall.exist.r == TRUE && r_wall_check == FALSE && hosei_flag == FALSE)
        {
            bz->play_melody(pc98_2, 2);
            val->current.len = 0.040;
            hosei_flag = TRUE;
        }

        if (val->tar.len - 0.01 <= val->current.len)
        {
            break;
        }
        vTaskDelay(1 / portTICK_PERIOD_MS);
    }

    // std::cout << "##### deceleration #####" << std::endl;
    // val->tar.acc = -(val->max.acc);

    while ((val->tar.len) > val->current.len)
    {
        if (val->tar.vel >= val->max.vel)
        {
            val->tar.acc = 0;
            val->tar.vel = val->max.vel;
        }
        vTaskDelay(1 / portTICK_PERIOD_MS);
    }

    // val->tar.vel = val->tar.vel;
    val->tar.acc = 0.0;

    // control->flag = FALSE;

    map->thinking_flag = TRUE;

    // std::cout << "run" << std::endl;
}

void Motion::run_half()
{
    // === オドメトリ補正トリガー ===
    // run_half完了後、現在のセル座標で補正を要求
    control->odom_correction_requested = true;
    control->correction_cell_x = map->pos.x;
    control->correction_cell_y = map->pos.y;
    control->correction_dir = map->pos.dir;

    sens->wall.control = FALSE; // 壁制御OFF
    val->current.flag = FRONT;  // 直進

    val->I.vel_error = 0.0;
    val->I.ang_error = 0.0;
    val->I.wall_error = 0.0;

    val->tar.ang_acc = 0.0;
    val->tar.ang_vel = 0.0;

    val->tar.len = SECTION_HALF;
    val->current.len = 0.0;
    val->tar.acc = val->max.acc;
    // val->tar.vel = 0.0;

    // 角度制御の準備（壁制御がFALSEなので角度制御を有効化）
    val->start_angle = val->current.rad;
    val->angle_control_mode = TRUE;

    control->flag = TRUE; // 制御ON

    // 制御式に欠陥を見つけた。加速度が一定以下だと、while文を抜け出せず、走行距離が延びてしまう。
    while (((val->tar.len - 0.01) - val->current.len) > (((val->tar.vel) * (val->tar.vel) - (val->end.vel) * (val->end.vel)) / (2.0 *
                                                                                                                                val->tar.acc)))
    {
        if (val->tar.len - 0.01 <= val->current.len)
        {
            break;
        }
        if (val->tar.vel >= val->max.vel)
        {
            val->tar.acc = 0;
            val->tar.vel = val->max.vel;
        }

        vTaskDelay(1 / portTICK_PERIOD_MS);
    }

    // std::cout << "##### deceleration #####" << std::endl;
    // val->tar.acc = -(val->tar.acc);

    while ((val->tar.len - 0.001) > val->current.len)
    {
        if (val->tar.vel >= val->max.vel)
        {
            val->tar.acc = 0;
            val->tar.vel = val->max.vel;
        }
        vTaskDelay(1 / portTICK_PERIOD_MS);
    }

    // val->tar.vel = val->tar.vel;
    val->tar.acc = 0.0;
    // control->flag = FALSE;

    // std::cout << "run" << std::endl;
}

void Motion::turn_left()
{
    vTaskDelay(200);

    control->flag = TRUE;            // 制御ON
    sens->wall.control = FALSE;      // 壁制御OFF
    val->angle_control_mode = FALSE; // 角度制御も無効
    val->current.flag = LEFT;        // 左旋回

    val->I.vel_error = 0.0;
    val->I.ang_error = 0.0;
    val->I.wall_error = 0.0;

    val->tar.vel = 0.0;
    val->tar.acc = 0.0;

    val->tar.ang_acc = val->max.ang_acc;
    val->max.ang_vel = val->max.ang_vel;

    val->tar.rad = TURN_QUARTER;

    // std::cout << "turn_left" << std::endl;
    int turn_count = 0;

    local_rad = val->current.rad; // 現在の角度を保存

    while (val->tar.rad > (val->current.rad - local_rad))
    {
        turn_count++;
        // printf("turn_count : %d\n", turn_count);
        vTaskDelay(1 / portTICK_PERIOD_MS);
    }

    // std::cout << "##### deceleration #####" << std::endl;

    val->tar.ang_vel = 0.0;
    val->tar.ang_acc = 0.0;

    // タイムアウト付きで角速度の収束を待つ
    uint32_t timeout_count = 0;
    const uint32_t MAX_TIMEOUT = 10; // 100ms
    while ((val->current.ang_vel >= 0.01 || val->current.ang_vel <= -0.01) &&
           timeout_count < MAX_TIMEOUT)
    {
        timeout_count++;
        vTaskDelay(1 / portTICK_PERIOD_MS);
    }

    control->flag = FALSE;

    // std::cout << "turn" << std::endl;
}

void Motion::turn_right()
{
    vTaskDelay(200);

    control->flag = TRUE;            // 制御ON
    sens->wall.control = FALSE;      // 壁制御OFF
    val->angle_control_mode = FALSE; // 角度制御も無効
    val->current.flag = RIGHT;       // 右旋回

    val->I.vel_error = 0.0;
    val->I.ang_error = 0.0;
    val->I.wall_error = 0.0;

    val->tar.vel = 0.0;
    val->tar.acc = 0.0;

    val->tar.ang_acc = -(val->max.ang_acc);
    // val->max.ang_vel = -(val->max.ang_vel);

    val->tar.rad = -(TURN_QUARTER);

    // std::cout << "turn_left" << std::endl;
    // int turn_count = 0;

    local_rad = val->current.rad; // 現在の角度を保存

    while (val->tar.rad < (val->current.rad - local_rad))
    {
        // turn_count++;
        // printf("val->tar.ang_vel : %f\n", val->tar.ang_vel);
        //  printf("turn_count : %d\n", turn_count);
        vTaskDelay(1 / portTICK_PERIOD_MS);
    }

    // std::cout << "##### deceleration #####" << std::endl;

    val->tar.ang_vel = 0.0;
    val->tar.ang_acc = 0.0;

    // タイムアウト付きで角速度の収束を待つ
    uint32_t timeout_count = 0;
    const uint32_t MAX_TIMEOUT = 10; // 100ms
    while ((val->current.ang_vel >= 0.01 || val->current.ang_vel <= -0.01) &&
           timeout_count < MAX_TIMEOUT)
    {
        timeout_count++;
        vTaskDelay(1 / portTICK_PERIOD_MS);
    }

    control->flag = FALSE;

    // std::cout << "turn" << std::endl;
}

void Motion::turn_half()
{

    // === オドメトリ補正トリガー ===
    // turn_half完了後、現在のセル座標で補正を要求
    control->odom_correction_requested = true;
    control->correction_cell_x = map->pos.x;
    control->correction_cell_y = map->pos.y;
    control->correction_dir = map->pos.dir;

    val->tar.ang_vel = 0.0;
    val->tar.vel = 0.0;
    val->I.vel_error = 0.0;
    val->I.ang_error = 0.0;
    val->I.wall_error = 0.0;

    // vTaskDelay(200);

    control->flag = TRUE;            // 制御ON
    sens->wall.control = FALSE;      // 壁制御OFF
    val->angle_control_mode = FALSE; // 角度制御も無効
    val->current.flag = LEFT;        // 左旋回

    val->tar.acc = 0.0;

    val->tar.ang_acc = val->max.ang_acc;
    // val->max.ang_vel = val->max.ang_vel;

    val->tar.rad = TURN_HALF;

    // std::cout << "turn_left" << std::endl;

    local_rad = val->current.rad; // 現在の角度を保存

    while (val->tar.rad - (val->current.rad - local_rad) > (val->tar.ang_vel * val->tar.ang_vel) / (2.0 * val->tar.ang_acc))
    {
        vTaskDelay(1 / portTICK_PERIOD_MS);
    }

    val->tar.ang_acc = -recalculate_angular_acceleration(val->current.rad - local_rad, val->tar.rad, val->tar.ang_vel);

    while (val->tar.rad > (val->current.rad - local_rad))
    {
        if (val->tar.ang_vel < val->min.ang_vel)
        {
            val->tar.ang_acc = 0;
            val->tar.ang_vel = val->min.ang_vel;
        }

        vTaskDelay(1 / portTICK_PERIOD_MS);
    }

    // std::cout << "##### deceleration #####" << std::endl;

    val->tar.ang_acc = 0.0;
    val->tar.ang_vel = 0.0;

    val->tar.ang_vel = 0.0;
    val->tar.vel = 0.0;
    val->I.vel_error = 0.0;
    val->I.ang_error = 0.0;
    val->I.wall_error = 0.0;

    // タイムアウト付きで角速度の収束を待つ
    uint32_t timeout_count = 0;
    const uint32_t MAX_TIMEOUT = 20; // 100ms
    while ((val->current.ang_vel >= 0.01 || val->current.ang_vel <= -0.01) &&
           timeout_count < MAX_TIMEOUT)
    {
        val->tar.ang_vel = 0.0;
        timeout_count++;
        vTaskDelay(1 / portTICK_PERIOD_MS);
    }

    val->tar.ang_vel = 0.0;
    val->tar.vel = 0.0;
    val->I.vel_error = 0.0;
    val->I.ang_error = 0.0;
    val->I.wall_error = 0.0;

    vTaskDelay(200);

    //control->flag = FALSE;

    // std::cout << "turn" << std::endl;
}

void Motion::stop()
{

    sens->wall.control = FALSE; // 壁制御OFF

    val->I.vel_error = 0.0;
    val->I.ang_error = 0.0;
    val->I.wall_error = 0.0;

    val->tar.ang_acc = 0.0;
    val->tar.ang_vel = 0.0;

    val->tar.len = SECTION_HALF;
    val->current.len = 0.0;
    val->tar.acc = val->max.acc;
    // val->tar.vel = 0.0;

    // 角度制御の準備（壁制御がFALSEなので角度制御を有効化）
    val->start_angle = val->current.rad;
    val->angle_control_mode = TRUE;

    control->flag = TRUE; // 制御ON

    while (((val->tar.len) - val->current.len) > (((val->tar.vel) * (val->tar.vel)) / (2.0 *
                                                                                       val->tar.acc)))
    {
        if (val->tar.vel >= val->max.vel)
        {
            val->tar.acc = 0;
            val->tar.vel = val->max.vel;
        }
        vTaskDelay(1 / portTICK_PERIOD_MS);
    }

    // std::cout << "##### deceleration #####" << std::endl;
    val->tar.acc = -recalculate_acceleration(val->current.len, val->tar.len, val->current.vel);

    while ((val->tar.len) > val->current.len)
    {
        if (val->tar.vel <= val->min.vel)
        {
            val->tar.acc = 0;
            val->tar.vel = val->min.vel;
        }
        vTaskDelay(1 / portTICK_PERIOD_MS);
    }

    val->tar.acc = 0.0;
    val->tar.vel = 0.0;

    while (fabs(val->current.vel) > 0.001)  // 速度が負にならない限りループし続けてしまう
    {
        val->tar.vel = 0.0;
        vTaskDelay(1 / portTICK_PERIOD_MS);
    }

    control->flag = FALSE; // 制御OFF

    val->current.len = 0.0;

    // std::cout << "stop" << std::endl;
}

void ::Motion::stop2()
{

    sens->wall.control = FALSE; // 壁制御OFF

    val->I.vel_error = 0.0;
    val->I.ang_error = 0.0;
    val->I.wall_error = 0.0;

    val->tar.ang_acc = 0.0;
    val->tar.ang_vel = 0.0;

    val->tar.len = SECTION_HALF;
    val->current.len = 0.0;
    val->tar.acc = val->max.acc;
    // val->tar.vel = 0.0;

    bool hosei_flag = NOT_YET;
    uint8_t hosei_dist = 0.050;

    // 角度制御の準備（壁制御がFALSEなので角度制御を有効化）
    val->start_angle = val->current.rad;
    val->angle_control_mode = TRUE;

    control->flag = TRUE; // 制御ON

    while (((val->tar.len - 0.01) - val->current.len) > (((val->tar.vel) * (val->tar.vel)) / (2.0 *
                                                                                              val->tar.acc)))
    {
        /*if (sens->wall.exist.fl == TRUE && sens->wall.exist.fr == TRUE)
        {
            if (sens->wall.val.fl > FRONT_WALL_LIMIT_FL && sens->wall.val.fr > FRONT_WALL_LIMIT_FR)
            {
                bz->play_melody(pc98, 2);
                // control->flag = FALSE;
                break;
            }
            if (sens->wall.val.fl < FRONT_WALL_LIMIT_FL && sens->wall.val.fr < FRONT_WALL_LIMIT_FR && hosei_flag == NOT_YET)
            {
                // bz->play_melody(pc98, 2);
                // while(sens->wall.val.fl < FRONT_WALL_LIMIT_FL && sens->wall.val.fr < FRONT_WALL_LIMIT_FR){vTaskDelay(1/portTICK_PERIOD_MS);}
                val->tar.len = hosei_dist;
                // bz->play_melody(pc98, 2);
                hosei_flag = DONE;
            }
        }*/

        vTaskDelay(1 / portTICK_PERIOD_MS);
    }

    // std::cout << "##### deceleration #####" << std::endl;
    val->tar.acc = -(val->max.acc);

    while ((val->tar.len - 0.001) > val->current.len)
    {
        /*if (val->tar.vel <= val->min.vel)
        {
            val->tar.acc = 0;
            val->tar.vel = val->min.vel;
            if (sens->wall.exist.fl == TRUE && sens->wall.exist.fr == TRUE)
            {
                if (sens->wall.val.fl > FRONT_WALL_LIMIT_FL && sens->wall.val.fr > FRONT_WALL_LIMIT_FR)
                {
                    bz->play_melody(pc98, 2);
                    // control->flag = FALSE;
                    break;
                }
                if (sens->wall.val.fl < FRONT_WALL_LIMIT_FL && sens->wall.val.fr < FRONT_WALL_LIMIT_FR && hosei_flag == NOT_YET)
                {
                    bz->play_melody(pc98, 2);
                    //while(sens->wall.val.fl < FRONT_WALL_LIMIT_FL && sens->wall.val.fr < FRONT_WALL_LIMIT_FR){vTaskDelay(1/portTICK_PERIOD_MS);}
                    val->tar.len = hosei_dist;
                    bz->play_melody(pc98, 2);
                    hosei_flag = DONE;
                }
            }
        }*/
        vTaskDelay(1 / portTICK_PERIOD_MS);
    }

    val->tar.acc = 0.0;
    val->tar.vel = 0.0;

    while (val->current.vel >= 0.0)
    {
        /*if (sens->wall.exist.fl == TRUE && sens->wall.exist.fr == TRUE)
        {
            if (sens->wall.val.fl > FRONT_WALL_LIMIT_FL && sens->wall.val.fr > FRONT_WALL_LIMIT_FR)
            {
                bz->play_melody(pc98, 2);
                // control->flag = FALSE;
                break;
            }
        }*/
        vTaskDelay(1 / portTICK_PERIOD_MS);
    }

    control->flag = FALSE; // 制御OFF

    // std::cout << "stop" << std::endl;
}

void Motion::back()
{
    vTaskDelay(100);

    control->flag = TRUE;            // 制御ON
    sens->wall.control = FALSE;      // 壁制御OFF
    val->angle_control_mode = FALSE; // 角度制御も無効

    val->I.vel_error = 0.0;
    val->I.ang_error = 0.0;
    val->I.wall_error = 0.0;

    val->tar.vel = 0.0;
    val->tar.acc = 0.0;
    val->tar.ang_vel = 0.0;
    val->tar.ang_acc = 0.0;

    val->tar.len = -OFFSET_DISTANCE - 0.006;
    val->current.len = 0.0;
    val->tar.acc = -(0.4);
    // val->tar.vel = 0.0;

    // control->test_flag = TRUE;

    uint16_t count = 0;

    while (val->current.len > val->tar.len)
    {
        if (val->current.len < val->tar.len + 0.011)
        {
            control->flag = FALSE; // 制御ON
            control->test_flag = TRUE;
        }

        if (val->current.vel >= -0.01 && count > 400)
        {
            break;
        }
        count++;
        vTaskDelay(1 / portTICK_PERIOD_MS);
    }
    // static Buzzer::buzzer_score_t pc98[] = {
    //{2000, 100}, {1000, 100}};

    // control->test_flag = TRUE;

    /*while (val->current.len > val->tar.len)
    {
         control->test_Duty_l = -0.25;
         control->test_Duty_r = -0.28;
         if (count > 1000) // 300だとダメ（8）
         {
             //bz->play_melody(pc98, 2);
             break;
         }
         count++;
         vTaskDelay(1/portTICK_PERIOD_MS);
    }*/

    // bz->play_melody(pc98, 2);

    // control->flag = TRUE;
    // control->test_flag = FALSE;

    val->tar.acc = 0.0;
    val->tar.vel = 0.0;

    while (val->current.vel <= 0.0)
    {
        if (count > 400)
        {
            break;
        }
        count++;
        vTaskDelay(1 / portTICK_PERIOD_MS);
    }

    vTaskDelay(100);

    control->test_flag = FALSE;

    control->flag = FALSE; // 制御OFF

    std::cout << "back" << std::endl;
}

void Motion::slalom_left()
{
    // === オドメトリ補正トリガー ===
    // slalom_left完了後、現在のセル座標で補正を要求
    control->odom_correction_requested = true;
    control->correction_cell_x = map->pos.x;
    control->correction_cell_y = map->pos.y;
    control->correction_dir = map->pos.dir;

    map->thinking_flag = FALSE;
    control->flag = TRUE;           // 制御ON
    sens->wall.control = FALSE;     // 壁制御OFF
    val->angle_control_mode = TRUE; // 角度制御も無効
    val->current.flag = SLA_LEFT;   // 左旋回

    val->I.vel_error = 0.0;
    val->I.ang_error = 0.0;
    val->I.wall_error = 0.0;

    // val->tar.vel = 0.0;
    val->tar.acc = 0.0;
    val->tar.ang_vel = 0.0;
    val->tar.ang_acc = 0.0;

    val->current.len = 0.0;

    // val->tar.ang_acc = val->max.ang_acc;
    //  val->max.ang_vel = val->max.ang_vel;

    val->tar.rad = TURN_QUARTER;

    // std::cout << "turn_left" << std::endl;

    local_rad = val->current.rad; // 現在の角度を保存

    val->tar.len = PRE_DISTANCE;
    val->tar.vel = val->max.vel;

    // 前距離
    while ((val->tar.len) > val->current.len)
    {
        if (sens->wall.val.fl > 2500 && sens->wall.val.fr > 3000)
        {
            // 前壁検出
            while (sens->wall.val.fl < 5233 || sens->wall.val.fr < 5538)
            {
                vTaskDelay(1 / portTICK_PERIOD_MS);
            }
            
            break;
        }
        vTaskDelay(1 / portTICK_PERIOD_MS);
    }

    val->angle_control_mode = FALSE;

    val->tar.ang_acc = val->sla.ang_acc;
    // 旋回
    while (val->tar.rad - (val->current.rad - local_rad) > (val->tar.ang_vel * val->tar.ang_vel) / (2.0 * val->tar.ang_acc))
    {
        vTaskDelay(1 / portTICK_PERIOD_MS);
    }

    val->tar.ang_acc = -(val->sla.ang_acc);

    // 減速フェーズ：目標角度到達まで減速しつつ、最小角速度を維持
    const float MIN_ANG_VEL_LEFT = val->sla.ang_vel * 0.1; // 左回転の最小角速度（正の値）

    while (val->tar.rad > (val->current.rad - local_rad))
    {
        // 角速度が最小値より小さくなった場合、最小角速度を設定してゆっくり回転継続
        if (val->tar.ang_vel < MIN_ANG_VEL_LEFT)
        {
            val->tar.ang_acc = 0;
            val->tar.ang_vel = MIN_ANG_VEL_LEFT; // 完全にゼロにせず、最小速度を維持
        }

        vTaskDelay(1 / portTICK_PERIOD_MS);
    }
    val->tar.ang_acc = 0.0;
    val->tar.ang_vel = 0.0;
    val->current.len = 0.0;
    val->tar.len = FOL_DISTANCE;

    // bz->play_melody(pc98, 2);

    val->angle_control_mode = TRUE;

    // 後距離
    while ((val->tar.len) > val->current.len)
    {
        vTaskDelay(1 / portTICK_PERIOD_MS);
    }

    val->angle_control_mode = FALSE;

    map->thinking_flag = TRUE;

    // std::cout << "turn" << std::endl;
}

void Motion::slalom_right()
{
    // === オドメトリ補正トリガー ===
    // slalom_right完了後、現在のセル座標で補正を要求
    control->odom_correction_requested = true;
    control->correction_cell_x = map->pos.x;
    control->correction_cell_y = map->pos.y;
    control->correction_dir = map->pos.dir;

    map->thinking_flag = FALSE;
    control->flag = TRUE;            // 制御ON
    sens->wall.control = FALSE;      // 壁制御OFF
    val->angle_control_mode = FALSE; // 角度制御も無効
    val->current.flag = SLA_RIGHT;   // 左旋回

    val->I.vel_error = 0.0;
    val->I.ang_error = 0.0;
    val->I.wall_error = 0.0;

    // val->tar.vel = 0.0;
    val->tar.acc = 0.0;
    val->tar.ang_vel = 0.0;
    val->tar.ang_acc = 0.0;

    val->current.len = 0.0;

    // val->tar.ang_acc = val->max.ang_acc;
    //  val->max.ang_vel = val->max.ang_vel;

    val->tar.rad = -(TURN_QUARTER);

    // std::cout << "turn_left" << std::endl;

    local_rad = val->current.rad; // 現在の角度を保存

    val->tar.len = PRE_DISTANCE;
    val->tar.vel = val->max.vel;

    uint32_t wait_count = 0;

    // 前距離
    while ((val->tar.len) > val->current.len)
    {
        if (sens->wall.val.fl > 2500 && sens->wall.val.fr > 3000)
        {
            // 前壁検出
            while (sens->wall.val.fl < 5233 || sens->wall.val.fr < 5538)
            {
                vTaskDelay(1 / portTICK_PERIOD_MS);
            }
            
            break;
        }
        vTaskDelay(1 / portTICK_PERIOD_MS);
    }  // 前距離走行前に正面壁がある場合
       //   1. 壁から遠いなら、ある程度前に進むまで待機 <- ここに実装
       //   2. 壁から近いなら、壁あてして旋回 <- 探索の条件分岐で実装

    val->current.len = 0.0;

    val->tar.ang_acc = -(val->sla.ang_acc);
    // 旋回
    while (-(val->tar.rad - (val->current.rad - local_rad)) > (val->tar.ang_vel * val->tar.ang_vel) / (2.0 * -(val->tar.ang_acc)))
    {
        vTaskDelay(1 / portTICK_PERIOD_MS);
    }

    val->tar.ang_acc = val->sla.ang_acc;

    // 減速フェーズ：目標角度到達まで減速しつつ、最小角速度を維持
    const float MIN_ANG_VEL_RIGHT = val->sla.ang_vel * 0.1; // 右回転の最小角速度（正の値として定義）

    while ((val->tar.rad) < (val->current.rad - local_rad))
    {
        // 角速度が最小値の負より大きくなった場合、最小角速度を設定してゆっくり回転継続
        if (val->tar.ang_vel > -(MIN_ANG_VEL_RIGHT))
        {
            val->tar.ang_acc = 0;
            val->tar.ang_vel = -(MIN_ANG_VEL_RIGHT); // 完全にゼロにせず、最小速度を維持
        }

        vTaskDelay(1 / portTICK_PERIOD_MS);
    }

    val->tar.ang_acc = 0.0;
    val->tar.ang_vel = 0.0;
    val->current.len = 0.0;
    val->tar.len = FOL_DISTANCE;

    // bz->play_melody(pc98, 2);

    // 後距離
    while ((val->tar.len) > val->current.len)
    {
        vTaskDelay(1 / portTICK_PERIOD_MS);
    }

    // control->flag = FALSE;

    map->thinking_flag = TRUE;

    // std::cout << "turn" << std::endl;
}

/**
 * @brief 時間指定スラローム
 *
 * 加速区間・定速区間・減速区間を時間（ms）で指定してスラロームを実行します。
 * 引数 dir_flag は回転方向フラグ（SLA_LEFT / SLA_RIGHT）を指定します。
 * accel_ms, const_ms, decel_ms は各区間の時間（ミリ秒）です。
 */
void Motion::slalom_time(uint8_t dir_flag, uint32_t accel_ms, uint32_t const_ms, uint32_t decel_ms)
{
    // === オドメトリ補正トリガー ===
    control->odom_correction_requested = true;
    control->correction_cell_x = map->pos.x;
    control->correction_cell_y = map->pos.y;
    control->correction_dir = map->pos.dir;

    map->thinking_flag = FALSE;
    control->flag = TRUE;           // 制御ON
    sens->wall.control = FALSE;     // 壁制御OFF
    val->angle_control_mode = FALSE;

    // スラローム用のフラグ
    if (dir_flag == SLA_LEFT)
        val->current.flag = SLA_LEFT;
    else
        val->current.flag = SLA_RIGHT;

    // 初期化
    val->I.vel_error = 0.0;
    val->I.ang_error = 0.0;
    val->I.wall_error = 0.0;

    val->tar.vel = 0.0;
    val->tar.acc = 0.0;
    val->tar.ang_vel = 0.0;
    val->tar.ang_acc = 0.0;

    val->current.len = 0.0;

    // 目標角度は四分の一ターン（スラローム想定）
    if (dir_flag == SLA_LEFT)
        val->tar.rad = TURN_QUARTER;
    else
        val->tar.rad = -TURN_QUARTER;

    local_rad = val->current.rad; // 現在角度を保存

    val->tar.len = PRE_DISTANCE;
    val->tar.vel = val->max.vel;

    // 前距離
    while ((val->tar.len) > val->current.len)
    {
        if (sens->wall.val.fl > 2500 && sens->wall.val.fr > 3000)
        {
            // 前壁検出
            while (sens->wall.val.fl < 5233 || sens->wall.val.fr < 5538)
            {
                vTaskDelay(1 / portTICK_PERIOD_MS);
            }
            
            break;
        }
        vTaskDelay(1 / portTICK_PERIOD_MS);
    }  // 前距離走行前に正面壁がある場合
       //   1. 壁から遠いなら、ある程度前に進むまで待機 <- ここに実装
       //   2. 壁から近いなら、壁あてして旋回 <- 探索の条件分岐で実装

    val->current.len = 0.0;

    // 1) 加速フェーズ: 指定時間だけ角加速を与える
    if (accel_ms > 0)
    {
        if (dir_flag == SLA_LEFT)
            val->tar.ang_acc = val->sla.ang_acc;
        else
            val->tar.ang_acc = -(val->sla.ang_acc);

        for (uint32_t t = 0; t < accel_ms; ++t)
        {
            // 途中で目標角度に到達したら抜ける
            if (dir_flag == SLA_LEFT)
            {
                if ((val->current.rad - local_rad) >= val->tar.rad)
                    break;
            }
            else
            {
                if ((val->current.rad - local_rad) <= val->tar.rad)
                    break;
            }
            vTaskDelay(1 / portTICK_PERIOD_MS);
        }
    }

    // 2) 定速フェーズ: 加速度を0にして角速度を維持する
    val->tar.ang_acc = 0.0;
    if (const_ms > 0)
    {
        for (uint32_t t = 0; t < const_ms; ++t)
        {
            if (dir_flag == SLA_LEFT)
            {
                if ((val->current.rad - local_rad) >= val->tar.rad)
                    break;
            }
            else
            {
                if ((val->current.rad - local_rad) <= val->tar.rad)
                    break;
            }
            vTaskDelay(1 / portTICK_PERIOD_MS);
        }
    }

    // 3) 減速フェーズ: 指定時間だけ逆向きの角加速度を与えて減速
    if (decel_ms > 0)
    {
        if (dir_flag == SLA_LEFT)
            val->tar.ang_acc = -(val->sla.ang_acc);
        else
            val->tar.ang_acc = val->sla.ang_acc;

        for (uint32_t t = 0; t < decel_ms; ++t)
        {
            // 目標角度に到達したら抜ける
            if (dir_flag == SLA_LEFT)
            {
                /*if ((val->current.rad - local_rad) >= val->tar.rad)
                {
                    val->tar.ang_acc = 0.0;
                    val->tar.ang_vel = 0.0;
                    //break;
                }*/
                if (val->tar.ang_vel < 0.02)
                {
                    val->tar.ang_acc = 0;
                    val->tar.ang_vel = 0; // 完全にゼロにせず、最小速度を維持
                }
            }
            else
            {
                /*if ((val->current.rad - local_rad) <= val->tar.rad)
                {
                    val->tar.ang_acc = 0.0;
                    val->tar.ang_vel = 0.0;
                    //break;
                }*/
               // 角速度が最小値の負より大きくなった場合、最小角速度を設定してゆっくり回転継続
                if (val->tar.ang_vel > -(0.02))
                {
                    val->tar.ang_acc = 0;
                    val->tar.ang_vel = 0; // 完全にゼロにせず、最小速度を維持
                }

            }
            vTaskDelay(1 / portTICK_PERIOD_MS);
        }
    }

    // 最終的に角速度・角加速度をゼロにする
    val->tar.ang_acc = 0.0;
    val->tar.ang_vel = 0.0;

    // 後距離（既存スラロームと同様に後進距離を取る）
    val->current.len = 0.0;
    val->tar.len = FOL_DISTANCE;

    // 後距離走行（角度制御を有効化して安定させる）
    val->angle_control_mode = TRUE;
    while ((val->tar.len) > val->current.len)
    {
        vTaskDelay(1 / portTICK_PERIOD_MS);
    }

    val->angle_control_mode = FALSE;

    map->thinking_flag = TRUE;
}

/**
 * @brief 躍度制御スラローム（9フェーズ）
 *
 * dir_flag: 回転方向フラグ (SLA_LEFT / SLA_RIGHT)
 * jerk: 躍度 [rad/s^3]（正の値）。符号は dir_flag に依存して適用される。
 * phase_ms: 各区間の時間配列（ミリ秒）。期待する要素数は 9（下記の順序）。
 * phase order:
 * 0: 直進
 * 1: 躍加速
 * 2: 定速
 * 3: 躍減速
 * 4: 定速
 * 5: 躍減速
 * 6: 定速
 * 7: 躍加速
 * 8: 直進
 */
void Motion::slalom_jerk(uint8_t dir_flag, float jerk, const uint32_t *phase_ms, size_t phase_count)
{
    const size_t EXPECTED = 9;
    if (phase_count < EXPECTED || phase_ms == nullptr)
    {
        printf("slalom_jerk: invalid phase_ms (need %d elements)\n", (int)EXPECTED);
        return;
    }

    // オドメトリ補正トリガー
    control->odom_correction_requested = true;
    control->correction_cell_x = map->pos.x;
    control->correction_cell_y = map->pos.y;
    control->correction_dir = map->pos.dir;

    map->thinking_flag = FALSE;
    control->flag = TRUE;
    sens->wall.control = FALSE;

    // 回転フラグ
    if (dir_flag == SLA_LEFT)
        val->current.flag = SLA_LEFT;
    else
        val->current.flag = SLA_RIGHT;

    // 初期化
    val->I.vel_error = 0.0;
    val->I.ang_error = 0.0;
    val->I.wall_error = 0.0;

    val->tar.vel = val->max.vel;
    val->tar.acc = 0.0;
    val->tar.ang_vel = 0.0;
    val->tar.ang_acc = 0.0;

    val->current.len = 0.0;

    // 目標角度（四分の一ターン）
    if (dir_flag == SLA_LEFT)
        val->tar.rad = TURN_QUARTER;
    else
        val->tar.rad = -TURN_QUARTER;

    local_rad = val->current.rad;

    // 符号を決定（躍度をどちら向きに適用するか）
    float sign_accel = (dir_flag == SLA_LEFT) ? 1.0f : -1.0f;

    // === 前距離（距離ベース） ===
    val->tar.len = PRE_DISTANCE;
    val->tar.vel = val->max.vel;
    val->angle_control_mode = TRUE;
    while ((val->tar.len) > val->current.len)
    {
        if (sens->wall.val.fl > 2500 && sens->wall.val.fr > 3000)
        {
            // 前壁検出
            while (sens->wall.val.fl < 5033 || sens->wall.val.fr < 5327) // l:5233 r:5538
            {
                vTaskDelay(1 / portTICK_PERIOD_MS);
            }
            
            break;
        }
        vTaskDelay(1 / portTICK_PERIOD_MS);
    }
    val->angle_control_mode = FALSE;

    // 躍度積分制御の有効化と設定
    val->jerk_integration_enabled = TRUE;
    val->phase_timestamp_ms = 0;  // タイムスタンプをリセット

    // 9フェーズを順に実行（フェーズ1～7）
    for (size_t phase = 1; phase < (EXPECTED - 1); ++phase)
    {
        uint32_t phase_duration_ms = phase_ms[phase];
        
        // フェーズ種別判定
        enum PhaseType { JERK_ACCEL = 1, CONST = 2, JERK_DECEL = 3 };
        PhaseType type = CONST;
        switch (phase)
        {
        case 1:
        case 7:
            type = JERK_ACCEL;
            break;
        case 3:
        case 5:
            type = JERK_DECEL;
            break;
        default: // case 2, 4, 6
            type = CONST;
            break;
        }

        // フェーズ開始時の処理（躍度積分は Interrupt::calc_target で行う）
        if (type == CONST)
        {
            val->current_jerk = 0.0;
            
            // phase 4 に入るとき、前フェーズからの角加速度を明示的にリセット
            if (phase == 4)
            {
                // 躍度積分を一時的に無効化してリセット（Interruptの干渉を防ぐ）
                val->jerk_integration_enabled = FALSE;

                val->tar.ang_acc = 0.0;

                val->jerk_integration_enabled = TRUE; // これがないとタイマー計測されない
            }

            val->angle_control_mode = FALSE;
        }
        else if (type == JERK_ACCEL)
        {
            // 加速フェーズ：current_jerk をそのまま使用（Interrupt で積分）
            val->current_jerk = sign_accel * fabs(jerk);
            val->angle_control_mode = FALSE;
            val->jerk_integration_enabled = TRUE;
        }
        else if (type == JERK_DECEL)
        {
            // 減速フェーズ：躍度を反転
            val->current_jerk = -sign_accel * fabs(jerk);
            val->angle_control_mode = FALSE;
            val->jerk_integration_enabled = TRUE;
        }

        // フェーズ開始時刻を記録
        uint32_t phase_start_time = val->phase_timestamp_ms;
        
        // フェーズ時間分ループ（時間ベース）
        while ((val->phase_timestamp_ms - phase_start_time) < phase_duration_ms)
        {
            if (phase == 4)
            {
                val->tar.ang_acc = 0.0;
            }
            
            vTaskDelay(1 / portTICK_PERIOD_MS);
        }
    }
    
    // === 躍度積分制御を無効化 ===
    
    val->current_jerk = 0.0f;
    val->tar.ang_acc = 0.0;
    val->tar.ang_vel = 0.0;
    val->jerk_integration_enabled = FALSE;

    // === 後距離（距離ベース） ===
    val->current.len = 0.0;
    val->tar.len = FOL_DISTANCE;
    val->angle_control_mode = TRUE;
    while ((val->tar.len) > val->current.len)
    {
        val->tar.ang_acc = 0.0;
        vTaskDelay(1 / portTICK_PERIOD_MS);
    }
    val->angle_control_mode = FALSE;

    map->thinking_flag = TRUE;
}

void Motion::check_enkaigei()
{
    sens->wall.control = FALSE;
    val->angle_control_mode = FALSE; // 角度制御も無効
    control->flag = TRUE;

    val->I.vel_error = 0.0;
    val->I.ang_error = 0.0;
    val->I.wall_error = 0.0;

    val->tar.vel = 0.0;
    val->tar.acc = 0.0;
    val->tar.ang_vel = 0.0;
    val->tar.ang_acc = 0.0;

    while (1)
    {
        printf("Duty_L:%f    Duty_R:%f\n", control->Duty_l, control->Duty_r);
        vTaskDelay(1 / portTICK_PERIOD_MS);
    }

    std::cout << "check_enkaigei" << std::endl;
}

void Motion::turn_left_2()
{
    val->tar.ang_vel = 0.0; // ここに置くとちゃんとゼロになる
    val->tar.vel = 0.0;
    val->I.vel_error = 0.0;
    val->I.ang_error = 0.0;
    val->I.wall_error = 0.0;

    control->flag = TRUE;            // 制御ON
    sens->wall.control = FALSE;      // 壁制御OFF
    val->angle_control_mode = FALSE; // 角度制御も無効
    val->current.flag = LEFT;        // 左旋回

    val->tar.acc = 0.0;

    val->tar.ang_acc = val->max.ang_acc;
    // val->max.ang_vel = val->max.ang_vel;

    val->tar.rad = TURN_QUARTER;

    // std::cout << "turn_left" << std::endl;

    local_rad = val->current.rad; // 現在の角度を保存

    while (val->tar.rad - (val->current.rad - local_rad) > (val->tar.ang_vel * val->tar.ang_vel) / (2.0 * val->tar.ang_acc))
    {
        vTaskDelay(1 / portTICK_PERIOD_MS);
    }

    val->tar.ang_acc = -recalculate_angular_acceleration(val->current.rad - local_rad, val->tar.rad, val->tar.ang_vel);

    while (val->tar.rad > (val->current.rad - local_rad))
    {
        if (val->tar.ang_vel < val->min.ang_vel)
        {
            val->tar.ang_acc = 0;
            val->tar.ang_vel = val->min.ang_vel;
        }

        vTaskDelay(1 / portTICK_PERIOD_MS);
    }

    // std::cout << "##### deceleration #####" << std::endl;

    val->tar.ang_vel = 0.0;
    val->tar.ang_acc = 0.0;

    // タイムアウト付きで角速度の収束を待つ
    uint32_t timeout_count = 0;
    const uint32_t MAX_TIMEOUT = 10; // 100ms
    while ((val->current.ang_vel >= 0.01 || val->current.ang_vel <= -0.01) &&
           timeout_count < MAX_TIMEOUT)
    {
        val->tar.ang_vel = 0.0;
        timeout_count++;
        vTaskDelay(1 / portTICK_PERIOD_MS);
    }

    // control->flag = FALSE;

    val->tar.ang_vel = 0.0;
    val->tar.vel = 0.0;
    val->I.vel_error = 0.0;
    val->I.ang_error = 0.0;
    val->I.wall_error = 0.0;

    vTaskDelay(200);

    // === オドメトリ補正トリガー ===
    // turn_left_2完了後、現在のセル座標で補正を要求
    control->odom_correction_requested = true;
    control->correction_cell_x = map->pos.x;
    control->correction_cell_y = map->pos.y;
    control->correction_dir = map->pos.dir;

    // std::cout << "turn" << std::endl;
}

void Motion::turn_right_2()
{
    val->tar.ang_vel = 0.0;
    val->tar.vel = 0.0;
    val->I.vel_error = 0.0;
    val->I.ang_error = 0.0;
    val->I.wall_error = 0.0;

    // vTaskDelay(200);

    control->flag = TRUE;            // 制御ON
    sens->wall.control = FALSE;      // 壁制御OFF
    val->angle_control_mode = FALSE; // 角度制御も無効
    val->current.flag = RIGHT;       // 右旋回

    val->tar.acc = 0.0;

    val->tar.ang_acc = -(val->max.ang_acc);
    // val->max.ang_vel = -(val->max.ang_vel);

    val->tar.rad = -(TURN_QUARTER);

    // std::cout << "turn_left" << std::endl;
    // int turn_count = 0;

    local_rad = val->current.rad; // 現在の角度を保存

    while (-(val->tar.rad - (val->current.rad - local_rad)) > (val->tar.ang_vel * val->tar.ang_vel) / (2.0 * -(val->tar.ang_acc)))
    {
        vTaskDelay(1 / portTICK_PERIOD_MS);
    }

    val->tar.ang_acc = recalculate_angular_acceleration(val->current.rad - local_rad, val->tar.rad, val->tar.ang_vel);

    while ((val->tar.rad) < (val->current.rad - local_rad))
    {
        if (val->tar.ang_vel > -(val->min.ang_vel))
        {
            val->tar.ang_acc = 0;
            val->tar.ang_vel = -(val->min.ang_vel);
        }

        vTaskDelay(1 / portTICK_PERIOD_MS);
    }

    val->tar.ang_vel = 0.0;
    val->tar.ang_acc = 0.0;

    // タイムアウト付きで角速度の収束を待つ
    uint32_t timeout_count = 0;
    const uint32_t MAX_TIMEOUT = 10; // 100ms
    while ((val->current.ang_vel >= 0.01 || val->current.ang_vel <= -0.01) &&
           timeout_count < MAX_TIMEOUT)
    {
        val->tar.ang_vel = 0.0;
        timeout_count++;
        vTaskDelay(1 / portTICK_PERIOD_MS);
    }

    // std::cout << "##### deceleration #####" << std::endl;

    val->tar.ang_vel = 0.0;
    val->tar.vel = 0.0;
    val->I.vel_error = 0.0;
    val->I.ang_error = 0.0;
    val->I.wall_error = 0.0;

    vTaskDelay(200);

    // control->flag = FALSE;

    // === オドメトリ補正トリガー ===
    // turn_right_2完了後、現在のセル座標で補正を要求
    control->odom_correction_requested = true;
    control->correction_cell_x = map->pos.x;
    control->correction_cell_y = map->pos.y;
    control->correction_dir = map->pos.dir;

    // std::cout << "turn" << std::endl;
}

void Motion::wall_check()
{
    sens->wall.control = TRUE;
    control->flag = FALSE;

    val->I.vel_error = 0.0;
    val->I.ang_error = 0.0;
    val->I.wall_error = 0.0;

    val->tar.vel = 0.0;
    val->tar.acc = 0.0;
    val->tar.ang_vel = 0.0;
    val->tar.ang_acc = 0.0;

    while (1)
    {
        // 設定したsens->wall.th_wallの値によって光るLEDが変わる
        printf("sens.wall.val.fl:%d    sens.wall.val.l:%d    sens.wall.val.r:%d    sens.wall.val.fr:%d\n", sens->wall.val.fl, sens->wall.val.l, sens->wall.val.r, sens->wall.val.fr);
        led->set(sens->wall.exist.fl + (sens->wall.exist.l << 1) + (sens->wall.exist.r << 2) + (sens->wall.exist.fr << 3));
        // printf("val->current.wall_error:%f sens->wall.error.r:%d sens->wall.error.l:%d\n", val->current.wall_error, sens->wall.error.r, sens->wall.error.l);
        //  壁制御に使用しているセンサ値の表示
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }

    // std::cout << "check_enkaigei" << std::endl;
}

void Motion::adjust_pid(const char *gain, float *pid, float step, uint8_t mode_num)
{

    while (1)
    {
        led->set(mode_num + 1);

        if (sens->wall.val.fl + sens->wall.val.l + sens->wall.val.r + sens->wall.val.fr > 3000)
        {
            led->set(0b1111);
            printf("adjust %s\n", gain);
            vTaskDelay(1000 / portTICK_PERIOD_MS);
            return;
        }

        if (val->current.vel > 0.01)
        {

            *pid += step;

            vTaskDelay(pdMS_TO_TICKS(50));
        }
        if (val->current.vel < -0.01)
        {

            *pid -= step;

            vTaskDelay(pdMS_TO_TICKS(50));
        }

        printf("%s : %f\n", gain, *pid);

        vTaskDelay(50 / portTICK_PERIOD_MS);
    }
}

void Motion::set_pid_gain()
{
    uint8_t mode = 0;

    const char *speed_Kp = "speed_Kp";
    const char *speed_Ki = "speed_Ki";
    const char *speed_Kd = "speed_Kd";
    const char *ang_vel_Kp = "ang_vel_Kp";
    const char *ang_vel_Ki = "ang_vel_Ki";
    const char *ang_vel_Kd = "ang_vel_Kd";
    const char *wall_Kp = "wall_Kp";
    const char *wall_Ki = "wall_Ki";
    const char *wall_Kd = "wall_Kd";

    FilePidGain pid_gain = read_file_pid();

    adjust_pid(speed_Kp, &pid_gain.speed_Kp, 0.1, mode);
    adjust_pid(speed_Ki, &pid_gain.speed_Ki, 10, mode + 1);
    adjust_pid(speed_Kd, &pid_gain.speed_Kd, 0.1, mode + 2);
    adjust_pid(ang_vel_Kp, &pid_gain.ang_vel_Kp, 0.01, mode + 3);
    adjust_pid(ang_vel_Ki, &pid_gain.ang_vel_Ki, 1, mode + 4);
    adjust_pid(ang_vel_Kd, &pid_gain.ang_vel_Kd, 0.1, mode + 5);
    adjust_pid(wall_Kp, &pid_gain.wall_Kp, 0.001, mode + 6);
    adjust_pid(wall_Ki, &pid_gain.wall_Ki, 0.01, mode + 7);
    adjust_pid(wall_Kd, &pid_gain.wall_Kd, 0.01, mode + 8);

    write_file_pid(&pid_gain);

    printf("set_pid_gain\n");
}

void Motion::adjust_wall_threshold(const char *threshold, uint16_t *th_value, uint8_t step, uint8_t mode_num)
{

    while (1)
    {
        led->set(mode_num + 1);

        if (sens->wall.val.fl + sens->wall.val.l + sens->wall.val.r + sens->wall.val.fr > 3000)
        {
            led->set(0b1111);
            printf("adjust %s\n", threshold);
            vTaskDelay(1000 / portTICK_PERIOD_MS);
            return;
        }

        if (val->current.vel > 0.01)
        {

            *th_value += step;

            vTaskDelay(pdMS_TO_TICKS(50));
        }
        if (val->current.vel < -0.01)
        {

            *th_value -= step;

            vTaskDelay(pdMS_TO_TICKS(50));
        }

        printf("%s : %d\n", threshold, *th_value);

        vTaskDelay(50 / portTICK_PERIOD_MS);
    }
}

void Motion::set_wall_threshold()
{
    uint8_t mode = 0;

    const char *th_wall_fl = "th_wall_fl";
    const char *th_wall_l = "th_wall_l";
    const char *th_wall_r = "th_wall_r";
    const char *th_wall_fr = "th_wall_fr";
    const char *th_control_l = "th_control_l";
    const char *th_control_r = "th_control_r";
    const char *ref_l = "ref_l";
    const char *ref_r = "ref_r";

    FileWallThreshold th_value = read_file_wall_th();

    adjust_wall_threshold(th_wall_fl, &th_value.th_wall_fl, 1, mode);
    adjust_wall_threshold(th_wall_l, &th_value.th_wall_l, 1, mode + 1);
    adjust_wall_threshold(th_wall_r, &th_value.th_wall_r, 1, mode + 2);
    adjust_wall_threshold(th_wall_fr, &th_value.th_wall_fr, 1, mode + 3);
    adjust_wall_threshold(th_control_l, &th_value.th_control_l, 1, mode + 4);
    adjust_wall_threshold(th_control_r, &th_value.th_control_r, 1, mode + 5);
    adjust_wall_threshold(ref_l, &th_value.ref_l, 10, mode + 6);
    adjust_wall_threshold(ref_r, &th_value.ref_r, 10, mode + 7);

    write_file_wall_th(&th_value);

    printf("set_pid_gain\n");
}

void Motion::offset()
{
    sens->wall.control = FALSE; // 壁制御OFF

    val->I.vel_error = 0.0;
    val->I.ang_error = 0.0;
    val->I.wall_error = 0.0;

    val->tar.ang_acc = 0.0;
    val->tar.ang_vel = 0.0;

    val->tar.len = OFFSET_DISTANCE - 0.003;
    val->current.len = 0.0;
    val->tar.acc = val->max.acc;
    // val->tar.vel = 0.0;

    // 角度制御の準備（壁制御がFALSEなので角度制御を有効化）
    val->start_angle = val->current.rad;
    val->angle_control_mode = TRUE;

    control->flag = TRUE; // 制御ON

    while (((val->tar.len) - val->current.len) > (((val->tar.vel) * (val->tar.vel)) / (2.0 *
                                                                                       val->tar.acc)))
    {
        if (val->tar.vel >= val->max.vel)
        {
            val->tar.acc = 0;
            val->tar.vel = val->max.vel;
        }
        vTaskDelay(1 / portTICK_PERIOD_MS);
    }

    // std::cout << "##### deceleration #####" << std::endl;
    val->tar.acc = -recalculate_acceleration(val->current.len, val->tar.len, val->current.vel);

    while ((val->tar.len) > val->current.len)
    {
        if (val->tar.vel <= val->min.vel)
        {
            val->tar.acc = 0;
            val->tar.vel = val->min.vel;
        }
        vTaskDelay(1 / portTICK_PERIOD_MS);
    }

    val->tar.acc = 0.0;
    val->tar.vel = 0.0;

    while (fabs(val->current.vel) > 0.001)  // 速度が負にならない限りループし続けてしまう
    {
        val->tar.vel = 0.0;
        vTaskDelay(1 / portTICK_PERIOD_MS);
    }

    control->flag = FALSE; // 制御OFF

    val->current.len = 0.0;

    // std::cout << "offset" << std::endl;
}

void Motion::calibrate_wall_th()
{
    FileCenterSensValue center_val;
    bool hosei_flag = false;

    // 壁当てで中央に移動
    /*offset();
    turn_right_2();
    back();
    offset();*/

    /*turn_right_2();
    back();
    offset();
    turn_right_2();
    back();
    offset2();
    run_half();
    slalom_left();
    slalom_left();
    stop();*/

    turn_right_2();
    if ((sens->wall.val.fl > 30000) && (sens->wall.val.fr > 30000))
    {
        turn_half();
        back();
        offset();
        hosei_flag = true;
    }
    else
    {
        turn_half();
    }

    if ((sens->wall.val.fl > 30000) && (sens->wall.val.fr > 30000) && hosei_flag == false)
    {
        turn_half();
        back();
        offset();
        hosei_flag = true;
        turn_left_2();
    }
    else
    {
        turn_right_2();
    }
    if ((sens->wall.val.fl > 30000) && (sens->wall.val.fr > 30000))
    {
        turn_half();
        back();
        offset2();

        hosei_flag = false;
    }
    else
    {
        turn_half();
    }

    //run_half();

    // 右壁見る（前壁センサ）
    /*center_val.right_fl = sens->wall.val.fl;
    center_val.right_fr = sens->wall.val.fr;

    turn_right_2();

    // 後ろ壁見る（前壁センサ）
    center_val.rear_fl = sens->wall.val.fl;
    center_val.rear_fr = sens->wall.val.fr;

    turn_right_2();

    // 左壁見る（前壁センサ）
    center_val.left_fl = sens->wall.val.fl;
    center_val.left_fr = sens->wall.val.fr;

    turn_right_2();

    // 正面を向く（左右センサ）
    center_val.front_l = sens->wall.val.l;
    center_val.front_r = sens->wall.val.r;

    write_file_center_sens_val(&center_val);*/
}

void Motion::offset2()
{
    control->flag = TRUE;           // 制御ON
    sens->wall.control = FALSE;     // 壁制御OFF
    val->angle_control_mode = TRUE; // 角度制御も無効

    val->I.vel_error = 0.0;
    val->I.ang_error = 0.0;
    val->I.wall_error = 0.0;

    val->tar.vel = 0.0;
    val->tar.acc = 0.0;
    val->tar.ang_vel = 0.0;
    val->tar.ang_acc = 0.0;

    val->tar.len = OFFSET_DISTANCE;
    val->current.len = 0.0;
    val->tar.acc = val->max.acc;
    // val->tar.vel = 0.0;

    while (((val->tar.len - 0.01) - val->current.len) > (((val->tar.vel) * (val->tar.vel)) / (2.0 *
                                                                                              val->tar.acc)))
    {
        vTaskDelay(1 / portTICK_PERIOD_MS);
    }

    // std::cout << "##### deceleration #####" << std::endl;
    // val->tar.acc = -(val->max.acc);

    while ((val->tar.len - 0.001) > val->current.len) // offsetにこのwhile文の処理いらないかも
    {
        if (val->tar.vel <= val->min.vel)
        {
            val->tar.acc = 0;
            val->tar.vel = val->min.vel;
        }
        vTaskDelay(1 / portTICK_PERIOD_MS);
    }

    // val->tar.acc = 0.0;
    // val->tar.vel = 0.0;

    // std::cout << "offset" << std::endl;
}

void Motion::fast_straight(uint8_t straight_count)
{
    map->thinking_flag = FALSE;
    control->flag = TRUE;      // 制御ON
    sens->wall.control = TRUE; // 壁制御OFF

    val->I.vel_error = 0.0;
    val->I.ang_error = 0.0;
    val->I.wall_error = 0.0;

    val->tar.ang_acc = 0.0;
    val->tar.ang_vel = 0.0;

    val->tar.len = SECTION * straight_count;
    val->current.len = 0.0;
    val->tar.acc = val->max.acc;

    val->max.vel = calc_velocity((val->tar.len - 0.09), val->fast_ref.vel, val->max.acc); // 一区画以上の場合に加速区間を設定
    val->end.vel = val->fast_ref.vel;

    bool l_wall_check = sens->wall.exist.l;
    bool r_wall_check = sens->wall.exist.r;
    bool hosei_flag = FALSE;
    uint8_t section_cnt = 0;

    while (((val->tar.len - 0.01) - val->current.len) > (((val->tar.vel) * (val->tar.vel) - (val->end.vel) * (val->end.vel)) / (2.0 *
                                                                                                                                val->tar.acc)))
    {
        // 壁あり->壁なし
        if (sens->wall.exist.l == FALSE && l_wall_check == TRUE && hosei_flag == FALSE)
        {
            bz->play_melody(pc98, 2);
            val->current.len = 0.055 + SECTION * (section_cnt);
            hosei_flag = TRUE;
        }

        if (sens->wall.exist.r == FALSE && r_wall_check == TRUE && hosei_flag == FALSE)
        {
            bz->play_melody(pc98, 2);
            val->current.len = 0.055 + SECTION * (section_cnt);
            hosei_flag = TRUE;
        }

        // 壁なし->壁あり
        if (sens->wall.exist.l == TRUE && l_wall_check == FALSE && hosei_flag == FALSE)
        {
            bz->play_melody(pc98_2, 2);
            val->current.len = 0.038 + SECTION * (section_cnt); // 補正後の距離を伸ばしたい場合は、値を小さく
            hosei_flag = TRUE;
        }

        if (sens->wall.exist.r == TRUE && r_wall_check == FALSE && hosei_flag == FALSE)
        {
            bz->play_melody(pc98_2, 2);
            val->current.len = 0.040 + SECTION * (section_cnt);
            hosei_flag = TRUE;
        }

        section_cnt = static_cast<uint8_t>(std::floor(val->current.len / SECTION));

        vTaskDelay(1 / portTICK_PERIOD_MS);
    }

    // std::cout << "##### deceleration #####" << std::endl;
    val->tar.acc = -(val->max.acc);

    while ((val->tar.len) > val->current.len)
    {
        if (val->tar.vel <= val->fast_ref.vel)
        {
            val->tar.acc = 0;
            val->tar.vel = val->fast_ref.vel;
        }

        // 減速時壁切れ補正(ただし、連続する直線区間の距離によってかなり変動する)

        // 壁あり->壁なし
        if (sens->wall.exist.l == FALSE && l_wall_check == TRUE && hosei_flag == FALSE)
        {
            bz->play_melody(pc98, 2);
            val->current.len = 0.055 + SECTION * (section_cnt);
            hosei_flag = TRUE;
        }

        if (sens->wall.exist.r == FALSE && r_wall_check == TRUE && hosei_flag == FALSE)
        {
            bz->play_melody(pc98, 2);
            val->current.len = 0.055 + SECTION * (section_cnt);
            hosei_flag = TRUE;
        }

        // 壁なし->壁あり
        if (sens->wall.exist.l == TRUE && l_wall_check == FALSE && hosei_flag == FALSE)
        {
            bz->play_melody(pc98_2, 2);
            val->current.len = 0.038 + SECTION * (section_cnt); // 補正後の距離を伸ばしたい場合は、値を小さく
            hosei_flag = TRUE;
        }

        if (sens->wall.exist.r == TRUE && r_wall_check == FALSE && hosei_flag == FALSE)
        {
            bz->play_melody(pc98_2, 2);
            val->current.len = 0.040 + SECTION * (section_cnt);
            hosei_flag = TRUE;
        }

        section_cnt = static_cast<uint8_t>(std::floor(val->current.len / SECTION));

        vTaskDelay(1 / portTICK_PERIOD_MS);
    }

    val->tar.acc = 0.0;
    val->tar.vel = val->fast_ref.vel;
    val->max.vel = val->fast_ref.vel;

    map->thinking_flag = TRUE;

    // === オドメトリ補正トリガー ===
    // fast_straight完了後、現在のセル座標で補正を要求
    control->odom_correction_requested = true;
    control->correction_cell_x = map->pos.x;
    control->correction_cell_y = map->pos.y;
    control->correction_dir = map->pos.dir;
}

void Motion::fast_stop(uint8_t straight_count)
{
    map->thinking_flag = FALSE;
    control->flag = TRUE;      // 制御ON
    sens->wall.control = TRUE; // 壁制御OFF

    val->I.vel_error = 0.0;
    val->I.ang_error = 0.0;
    val->I.wall_error = 0.0;

    val->tar.ang_acc = 0.0;
    val->tar.ang_vel = 0.0;

    val->tar.len = SECTION * straight_count;
    val->current.len = 0.0;
    val->tar.acc = val->max.acc;

    val->max.vel = calc_velocity(val->tar.len, val->fast_ref.vel, val->max.acc);
    val->end.vel = 0;

    while (((val->tar.len - 0.01) - val->current.len) > (((val->tar.vel) * (val->tar.vel)) / (2.0 *
                                                                                              val->tar.acc)))
    {
        vTaskDelay(1 / portTICK_PERIOD_MS);
    }

    // std::cout << "##### deceleration #####" << std::endl;
    val->tar.acc = -recalculate_acceleration(val->current.len, val->tar.len, val->current.vel);

    while ((val->tar.len - 0.001) > val->current.len)
    {
        if (val->tar.vel <= val->min.vel)
        {
            val->tar.acc = 0;
            val->tar.vel = val->min.vel;
        }
        vTaskDelay(1 / portTICK_PERIOD_MS);
    }

    val->tar.acc = 0.0;
    val->tar.vel = 0.0;
    val->max.vel = val->fast_ref.vel;

    while (val->current.vel >= 0.0)
    {
        vTaskDelay(1 / portTICK_PERIOD_MS);
    }

    control->flag = FALSE; // 制御OFF

    map->thinking_flag = TRUE;

    // === オドメトリ補正トリガー ===
    // fast_stop完了後、現在のセル座標で補正を要求
    control->odom_correction_requested = true;
    control->correction_cell_x = map->pos.x;
    control->correction_cell_y = map->pos.y;
    control->correction_dir = map->pos.dir;

    // std::cout << "stop" << std::endl;
}

void Motion::check_motor_duty(float Duty_l, float Duty_r, uint32_t time)
{
    control->flag = FALSE; // 制御ON
    control->test_flag = TRUE;

    control->Duty_l = Duty_l;
    control->Duty_r = Duty_r;

    uint32_t count = 0;

    while (count < time)
    {
        mot->set_motor_speed(Duty_l, Duty_r);
        // printf("Duty_L:%f    Duty_R:%f\n", control->Duty_l, control->Duty_r);
        printf(">velocity:%f\n", val->current.vel);
        count++;
        vTaskDelay(1 / portTICK_PERIOD_MS);
    }

    control->flag = FALSE; // 制御OFF
    control->test_flag = FALSE;
}

float Motion::calc_velocity(float dist, float vel, float acc)
{
    return (vel + sqrt((vel * vel) + 2 * acc * dist)) / 2;
}

void Motion::detect_dead_zone(float step_size, float max_duty, uint32_t update_rate, uint32_t settle_time)
{
    control->flag = FALSE; // 制御OFF（手動モータ制御）
    control->test_flag = TRUE;

    float current_duty = 0.0;

    vTaskDelay(settle_time / portTICK_PERIOD_MS);

    while (current_duty <= max_duty)
    {
        control->Duty_l = -current_duty;
        control->Duty_r = current_duty;
        // モーターに指令値を出力
        mot->set_motor_speed(-current_duty, current_duty);

        current_duty += step_size;
        vTaskDelay(update_rate / portTICK_PERIOD_MS); // 1秒待機
    }

    mot->set_motor_speed(0.0, 0.0); // 最終的にモーター停止
    control->flag = FALSE;        // 制御OFF維持
    control->test_flag = FALSE;
}

void Motion::detect_saturation_region(float start_duty, float step_size, float max_duty, uint32_t update_rate, uint32_t settle_time)
{
    control->flag = FALSE; // 制御OFF（手動モータ制御）
    control->test_flag = TRUE;

    float current_duty = start_duty;

    vTaskDelay(settle_time / portTICK_PERIOD_MS);

    while (current_duty <= max_duty)
    {
        control->Duty_l = current_duty;
        control->Duty_r = current_duty;
        // モーターに指令値を出力
        mot->set_motor_speed(current_duty, current_duty);

        current_duty += step_size;
        vTaskDelay(update_rate / portTICK_PERIOD_MS); // 1秒待機
    }

    mot->set_motor_speed(0.0, 0.0); // 最終的にモーター停止
    control->flag = FALSE;        // 制御OFF維持
    control->test_flag = FALSE;
}

void Motion::apply_system_identification_signal(const float *signal_left, const float *signal_right, int num_samples, int sampling_period_ms)
{
    control->flag = FALSE;
    control->test_flag = TRUE;

    // 回転方向推奨値 0.15 (15%)
    // 並進方向推奨値 0.50 (50%)
    static const float DUTY_L = 0.20;
    static const float DUTY_R = 0.20;

    val->tar.vel = 0.0;
    val->tar.acc = 0.0;
    val->tar.ang_vel = 0.0;
    val->tar.ang_acc = 0.0;

    printf("=== Starting M-sequence Signal Application ===\n");
    printf("Number of samples: %d\n", num_samples);
    printf("Sampling period: %d ms\n", sampling_period_ms);

    for (int i = 0; i < num_samples; i++)
    {
        float duty_left = signal_left[i];
        float duty_right = signal_right[i];

        // duty値を安全な範囲に制限 (-0.5 to 0.5)
        if (duty_left > DUTY_L)
            duty_left = DUTY_L;
        if (duty_left < -DUTY_L)
            duty_left = -DUTY_L;
        if (duty_right > DUTY_R)
            duty_right = DUTY_R;
        if (duty_right < -DUTY_R)
            duty_right = -DUTY_R;

        control->Duty_l = duty_left;
        control->Duty_r = duty_right;

        mot->set_motor_speed(duty_left, duty_right);

        if (i % 10 == 0)
        {
            // printf("Progress: %d/%d samples\n", i + 1, num_samples);
        }

        vTaskDelay(sampling_period_ms / portTICK_PERIOD_MS);
    }

    mot->set_motor_speed(0.0, 0.0);
    control->Duty_l = 0.0;
    control->Duty_r = 0.0;

    printf("M-sequence signal application completed.\n");

    control->flag = FALSE;
    control->test_flag = FALSE;
}

void Motion::run_translation_identification(const float *signal_left, const float *signal_right, int num_samples, int sampling_period_ms)
{
    printf("--- Starting Translation Model Identification ---\n");
    printf("Robot will perform parallel wheel motion for system identification.\n");

    apply_system_identification_signal(signal_left, signal_right, num_samples, sampling_period_ms);

    printf("Translation model identification completed.\n");
}

void Motion::run_rotation_identification(const float *signal_left, const float *signal_right, int num_samples, int sampling_period_ms)
{
    printf("--- Starting Rotation Model Identification ---\n");
    printf("Robot will perform differential wheel motion for system identification.\n");

    apply_system_identification_signal(signal_left, signal_right, num_samples, sampling_period_ms);

    printf("Rotation model identification completed.\n");
}

/**
 * @brief 並進運動の加速度を再計算する関数
 *
 * 定速区間終了時に、残り距離と現在速度から停止に必要な加速度を計算します。
 * 等減速運動の公式 v^2 = 2*a*s を使用して、a = v^2 / (2*s) を計算します。
 *
 * @param current_position 現在位置 [m]
 * @param target_position 目標位置 [m]
 * @param current_velocity 現在速度 [m/s]
 * @param control_period 制御周期 [s] (デフォルト: 0.001 = 1kHz)
 * @return float 必要な減速度の絶対値 [m/s^2] (常に正の値)
 */
float Motion::recalculate_acceleration(float current_position, float target_position, float current_velocity, float control_period)
{
    // 残り距離を計算
    float remaining_distance = target_position - current_position;

    // 残り距離が非常に小さい場合は、大きな減速度を返す
    if (fabs(remaining_distance) < 0.0001f)
    {
        return 10.0f; // 最大減速度を想定
    }

    // 等減速運動の公式: v^2 = 2*a*s より a = v^2 / (2*s)
    // ここで、残り距離が負の場合（目標を超過した場合）も考慮
    float required_acceleration = (current_velocity * current_velocity) / (2.0f * fabs(remaining_distance));

    // 符号を考慮：減速する方向に加速度を設定
    if (remaining_distance < 0)
    {
        // 目標を超過している場合は、逆方向の加速度が必要
        // 通常は発生しないが、安全のため大きな値を返す
        required_acceleration = 10.0f;
    }

    return required_acceleration;
}

/**
 * @brief 回転運動の角加速度を再計算する関数
 *
 * 定速区間終了時に、残り角度と現在角速度から停止に必要な角加速度を計算します。
 * 等減速運動の公式 ω^2 = 2*α*θ を使用して、α = ω^2 / (2*θ) を計算します。
 *
 * @param current_angle 現在角度 [rad]
 * @param target_angle 目標角度 [rad]
 * @param current_angular_velocity 現在角速度 [rad/s]
 * @param control_period 制御周期 [s] (デフォルト: 0.001 = 1kHz)
 * @return float 必要な角減速度の絶対値 [rad/s^2] (常に正の値)
 */
float Motion::recalculate_angular_acceleration(float current_angle, float target_angle, float current_angular_velocity, float control_period)
{
    // 残り角度を計算
    float remaining_angle = target_angle - current_angle;

    // 残り角度が非常に小さい場合は、大きな角減速度を返す
    if (fabs(remaining_angle) < 0.0001f)
    {
        return 100.0f; // 最大角減速度を想定
    }

    // 等減速運動の公式: ω^2 = 2*α*θ より α = ω^2 / (2*θ)
    // 角速度の絶対値を使用して計算
    float required_angular_acceleration = (current_angular_velocity * current_angular_velocity) / (2.0f * fabs(remaining_angle));

    // 符号を考慮：減速する方向に角加速度を設定
    if (remaining_angle < 0 && current_angular_velocity > 0)
    {
        // 目標を超過しそうな場合は、大きな値を返す
        required_angular_acceleration = 100.0f;
    }
    else if (remaining_angle > 0 && current_angular_velocity < 0)
    {
        // 逆方向に回転している場合も大きな値を返す
        required_angular_acceleration = 100.0f;
    }

    return required_angular_acceleration;
}

/**
 * @brief 壁センサ値を指定時間計測する関数
 *
 * この関数は、既存のログシステムを利用して壁センサ値と走行距離を記録します。
 * Interrupt::logging()タスクが自動的にセンサ値をパーティションに書き込みます。
 *
 * @param duration_ms 計測時間 [ms] (デフォルト: 5000ms = 5秒)
 */
void Motion::measure_wall_sensor_distance(uint32_t duration_ms)
{
    printf("=== Wall Sensor Distance Measurement Start ===\n");
    printf("Duration: %lu ms\n", duration_ms);
    printf("Logging will be automatically saved to partition.\n");
    printf("Use emergency save function to export data.\n\n");

    // 制御フラグを無効化（ロボットは静止状態）
    control->flag = FALSE;
    sens->wall.control = FALSE;

    // ログフラグを有効化（既存のlogging()タスクが自動的に記録開始）
    control->log_flag = TRUE;

    // 開始時刻を記録
    int64_t start_time = esp_timer_get_time();
    int64_t elapsed_time = 0;

    // 指定時間が経過するまで待機
    while (elapsed_time < (duration_ms * 1000)) // マイクロ秒に変換
    {
        vTaskDelay(10 / portTICK_PERIOD_MS); // 10ms周期で確認
        elapsed_time = esp_timer_get_time() - start_time;

        // 進捗表示（1秒ごと）
        if ((elapsed_time / 1000000) != ((elapsed_time - 10000) / 1000000))
        {
            printf("Elapsed: %.1f s / %.1f s\n",
                   elapsed_time / 1000000.0,
                   duration_ms / 1000.0);
        }
    }

    // ログフラグを無効化（記録停止）
    control->log_flag = FALSE;

    printf("\n=== Wall Sensor Distance Measurement Complete ===\n");
    printf("Total time: %.3f seconds\n", elapsed_time / 1000000.0);
    printf("Please use emergency save to export the logged data.\n");
}

/**
 * @brief 壁センサキャリブレーション用関数
 *
 * ロボットを低速で前進させながら壁センサ値を計測します。
 * 既存のrun()関数とログシステムを組み合わせて使用します。
 */
void Motion::calibrate_wall_sensor_distance()
{
    printf("=== Wall Sensor Calibration Start ===\n");
    printf("Robot will move forward at low speed.\n");
    printf("Distance: 0.5m (approx. 5-6 cells)\n\n");

    // 低速モード設定
    float original_max_vel = val->max.vel;
    float original_max_acc = val->max.acc;
    float original_end_vel = val->end.vel;

    val->max.vel = 0.1; // 低速: 0.2 m/s
    val->max.acc = 1.0; // 緩加速: 0.3 m/s^2
    val->end.vel = 0.1; // 定速走行

    // ログ記録開始
    control->log_flag = TRUE;

    printf("Starting forward motion with logging...\n");

    // 走行距離リセット
    val->sum.len = 0.0;

    // ここからrunの機能

    control->flag = TRUE;       // 制御ON
    sens->wall.control = FALSE; // 壁制御OFF

    val->I.vel_error = 0.0;
    val->I.ang_error = 0.0;
    val->I.wall_error = 0.0;

    val->tar.ang_acc = 0.0;
    val->tar.ang_vel = 0.0;

    val->tar.len = 0.023;
    val->current.len = 0.0;
    val->tar.acc = val->max.acc;

    while (((val->tar.len) - val->current.len) > (((val->tar.vel) * (val->tar.vel)) / (2.0 *
                                                                                       val->tar.acc)))
    {
        if (val->tar.vel >= val->max.vel)
        {
            val->tar.acc = 0;
            val->tar.vel = val->max.vel;
        }
        vTaskDelay(1 / portTICK_PERIOD_MS);
    }

    // std::cout << "##### deceleration #####" << std::endl;
    val->tar.acc = -recalculate_acceleration(val->current.len, val->tar.len, val->current.vel);

    while ((val->tar.len) > val->current.len)
    {
        if (val->tar.vel <= val->min.vel)
        {
            val->tar.acc = 0;
            val->tar.vel = val->min.vel;
        }
        vTaskDelay(1 / portTICK_PERIOD_MS);
    }

    val->tar.acc = 0.0;
    val->tar.vel = 0.0;

    while (val->current.vel >= 0.0)
    {
        vTaskDelay(1 / portTICK_PERIOD_MS);
    }

    control->flag = FALSE; // 制御OFF

    val->current.len = 0.0;

    vTaskDelay(100 / portTICK_PERIOD_MS); // 区画間で少し待機

    turn_half();

    // ログ記録停止
    control->log_flag = FALSE;

    // 元の設定に戻す
    val->max.vel = original_max_vel;
    val->max.acc = original_max_acc;
    val->end.vel = original_end_vel;

    printf("\n=== Wall Sensor Calibration Complete ===\n");
    printf("Total distance traveled: %.3f m\n", val->sum.len);
    printf("Please use emergency save to export the logged data.\n");
    printf("Analyze the data in MATLAB using analyze_wall_sensor_distance.m\n");
}