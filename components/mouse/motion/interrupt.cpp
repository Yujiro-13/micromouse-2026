#include "interrupt.hpp"

#if CONFIG_RMOUSE_WIFI_ENABLE
#include "telemetry_bind.hpp" // ループ計測のテレメトリ公開(OFF 時は本 include ごと無効)
#endif

// #define ENC_MAX 4096
#define TIRE_DIAMETER 0.01495
#define MMPP TIRE_DIAMETER *M_PI / ENC_MAX
#define BASE_BATT 4.2
#define FF_VEL_GAIN 2.2
#define FF_ANG_VEL_GAIN 0.55

Interrupt::Interrupt()
{
}

Interrupt::~Interrupt() { std::cout << "~Interrupt" << std::endl; }

void Interrupt::ptr_by_sensor(SensorData *_sens) { sens = _sens; }

void Interrupt::ptr_by_motion(MotionValues *_val) { val = _val; }

void Interrupt::ptr_by_control(Control *_control) { control = _control; }

void Interrupt::ptr_by_map(MazeMap *_map) { map = _map; }

void Interrupt::set_device_driver(std::shared_ptr<Drivers> driver)
{
    encR = driver->encR;
    encL = driver->encL;
    mot = driver->mot;
    imu = driver->imu;
    // np = driver->np;
    // led = driver->led;
    // bz = driver->bz;
    // adc = driver->adc;

}

void Interrupt::get_semphr_handle(SemaphoreHandle_t *_on_logging) { on_logging = _on_logging; }

void Interrupt::calc_target()
{ //  目標値を計算する

    // === フェーズタイムスタンプのインクリメント（slalom_jerk用） ===
    if (val->jerk_integration_enabled == TRUE)
    {
        val->phase_timestamp_ms++;
    }

    // === 躍度積分制御（slalom_jerk実行時） ===
    if (val->jerk_integration_enabled == TRUE)
    {
        // 躍度から角加速度を積分（1ms単位）
        val->tar.ang_acc += (val->current_jerk) / 1000.0;
        
        // 角加速度をリミット
        float ang_acc_limit = std::min(fabs(val->sla_jerk.ang_acc), fabs(val->max.ang_acc));
        if (val->tar.ang_acc > ang_acc_limit)
            val->tar.ang_acc = ang_acc_limit;
        if (val->tar.ang_acc < -ang_acc_limit)
            val->tar.ang_acc = -ang_acc_limit;
    }

    val->tar.vel += (val->tar.acc) / 1000.0;

    if (val->tar.vel > val->max.vel)
    {
        val->tar.vel = val->max.vel;
    }

    val->tar.ang_vel += (val->tar.ang_acc) / 1000.0;

    if (val->current.flag == LEFT)
    {

        if (val->tar.ang_vel > val->max.ang_vel)
        {
            val->tar.ang_vel = val->max.ang_vel;
        }
    }
    else if (val->current.flag == RIGHT)
    {
        if (val->tar.ang_vel < -(val->max.ang_vel))
        {
            val->tar.ang_vel = -(val->max.ang_vel);
        }
    }
    else if (val->current.flag == SLA_LEFT)
    {
        // jerk 制御実行時と通常スラローム時で角速度制限を分ける
        float ang_vel_limit = val->jerk_integration_enabled ? val->sla_jerk.ang_vel : val->sla.ang_vel;
        if (val->tar.ang_vel > ang_vel_limit)
        {
            val->tar.ang_vel = ang_vel_limit;
        }
    }
    else if (val->current.flag == SLA_RIGHT)
    {
        // jerk 制御実行時と通常スラローム時で角速度制限を分ける
        float ang_vel_limit = val->jerk_integration_enabled ? val->sla_jerk.ang_vel : val->sla.ang_vel;
        if (val->tar.ang_vel < -(ang_vel_limit))
        {
            val->tar.ang_vel = -(ang_vel_limit);
        }
    }
    else
    {

    }

    return;
}

// 壁センサ値が閾値を超えていれば壁あり(TRUE)を返す純関数。
// 旧 wall_control 前半の fl/fr/l/r 同型 if/else と等価。
static inline Bool detect_wall(int val, int th)
{
    return (val > th) ? TRUE : FALSE;
}

// 制御閾値を超えていれば error=val-ref/enable=TRUE、そうでなければ error=0/enable=FALSE。
// 旧 wall_control 中盤の l/r 同型 if/else と等価。
static inline void update_control_error(int val, int th, int ref, int &error, bool &enable)
{
    if (val > th)
    {
        error = val - ref;
        enable = true;
    }
    else
    {
        error = 0;
        enable = false;
    }
}

void Interrupt::wall_control() //  壁制御
{
    // 各壁センサの有無判定（fl:左前 / fr:右前 / l:左 / r:右）
    sens->wall.exist.fl = detect_wall(sens->wall.val.fl, sens->wall.th_wall.fl);
    sens->wall.exist.fr = detect_wall(sens->wall.val.fr, sens->wall.th_wall.fr);
    sens->wall.exist.l = detect_wall(sens->wall.val.l, sens->wall.th_wall.l);
    sens->wall.exist.r = detect_wall(sens->wall.val.r, sens->wall.th_wall.r);

    // 左右の壁制御 error / control_enable を設定
    update_control_error(sens->wall.val.l, sens->wall.th_control.l, sens->wall.ref.l,
                         sens->wall.error.l, sens->wall.control_enable.l);
    update_control_error(sens->wall.val.r, sens->wall.th_control.r, sens->wall.ref.r,
                         sens->wall.error.r, sens->wall.control_enable.r);

    // === 柱検出ロジック（ヒステリシス付き） ===
    // 前壁センサが高い場合は柱検出を無効化（壁接近時の誤検出防止）
    const float FRONT_WALL_THRESHOLD = (sens->wall.th_wall.fl + sens->wall.th_wall.fr) * 2.5;
    bool front_wall_detected = (sens->wall.val.fl + sens->wall.val.fr) > FRONT_WALL_THRESHOLD;
    
    // 左柱検出（ヒステリシス: ON閾値=th_pillar, OFF閾値=th_pillar*0.8）
    if (!front_wall_detected && sens->wall.control_enable.l == FALSE)
    {
        if (!sens->wall.pillar_detected.l && sens->wall.val.l > sens->wall.th_pillar.l)
        {
            sens->wall.pillar_detected.l = TRUE;
        }
        else if (sens->wall.pillar_detected.l && sens->wall.val.l < sens->wall.th_pillar.l * 0.8)
        {
            sens->wall.pillar_detected.l = FALSE;
        }
        
        // 柱検出時のエラー計算
        if (sens->wall.pillar_detected.l)
        {
            sens->wall.pillar_error.l = sens->wall.val.l - sens->wall.ref_pillar.l;
        }
        else
        {
            sens->wall.pillar_error.l = 0;
        }
    }
    else
    {
        sens->wall.pillar_detected.l = FALSE;
        sens->wall.pillar_error.l = 0;
    }
    
    // 右柱検出（ヒステリシス: ON閾値=th_pillar, OFF閾値=th_pillar*0.8）
    if (!front_wall_detected && sens->wall.control_enable.r == FALSE)
    {
        if (!sens->wall.pillar_detected.r && sens->wall.val.r > sens->wall.th_pillar.r)
        {
            sens->wall.pillar_detected.r = TRUE;
        }
        else if (sens->wall.pillar_detected.r && sens->wall.val.r < sens->wall.th_pillar.r * 0.8)
        {
            sens->wall.pillar_detected.r = FALSE;
        }
        
        // 柱検出時のエラー計算
        if (sens->wall.pillar_detected.r)
        {
            sens->wall.pillar_error.r = sens->wall.val.r - sens->wall.ref_pillar.r;
        }
        else
        {
            sens->wall.pillar_error.r = 0;
        }
    }
    else
    {
        sens->wall.pillar_detected.r = FALSE;
        sens->wall.pillar_error.r = 0;
    }

    // === 制御計算（優先度: 壁制御 > 柱制御 > 角度制御） ===
    if (sens->wall.control == TRUE && sens->wall.val.fl + sens->wall.val.fr <= (sens->wall.th_wall.fl + sens->wall.th_wall.fr) * 5.0)
    {

        if (sens->wall.control_enable.l == TRUE && sens->wall.control_enable.r == TRUE)
        {
            val->current.wall_error = sens->wall.error.r - sens->wall.error.l;
        }
        else if (sens->wall.control_enable.l == FALSE && sens->wall.control_enable.r == TRUE)
        {
            val->current.wall_error = sens->wall.error.r /2;
        }
        else if (sens->wall.control_enable.l == TRUE && sens->wall.control_enable.r == FALSE)
        {
            val->current.wall_error = -(sens->wall.error.l) /2;
        }
        else
        {
            val->current.wall_error = 0;
        }

        val->I.wall_error += val->current.wall_error / 1000.0;
        val->p.wall_error = (val->p.wall_error - val->current.wall_error) * 1000.0;

        val->tar.wall_val = val->current.wall_error * (control->wall.Kp) + val->I.wall_error * (control->wall.Ki) - val->p.wall_error * (control->wall.Kd);
        
        // 壁制御が有効な場合のみ目標角速度を上書き
        if (sens->wall.control_enable.l == TRUE || sens->wall.control_enable.r == TRUE)
        {
            val->tar.ang_vel = val->tar.wall_val;
        }
        
        val->p.wall_error = val->current.wall_error;
    }
    else if (sens->wall.control == TRUE && !sens->wall.control_enable.l && !sens->wall.control_enable.r &&
             (sens->wall.pillar_detected.l || sens->wall.pillar_detected.r))
    {
        // === 優先度2: 柱制御（壁制御が無効で柱検出時） ===
        // 柱制御はP制御またはPD制御のみ（積分項なし）
        float pillar_control_val = 0.0;
        
        if (sens->wall.pillar_detected.l && sens->wall.pillar_detected.r)
        {
            // 両柱検出時: 左右の差分で制御
            float pillar_error = sens->wall.pillar_error.r - sens->wall.pillar_error.l;
            pillar_control_val = pillar_error * control->pillar.Kp;
            
            // D制御（必要に応じて）
            if (control->pillar.Kd > 0.0)
            {
                float pillar_d_term = (pillar_error - val->p.pillar_error) * 1000.0; // 微分項
                pillar_control_val += pillar_d_term * control->pillar.Kd;
                val->p.pillar_error = pillar_error;
            }
        }
        else if (sens->wall.pillar_detected.r)
        {
            // 右柱のみ検出時
            float pillar_error = sens->wall.pillar_error.r / 2.0;
            pillar_control_val = pillar_error * control->pillar.Kp;
            
            if (control->pillar.Kd > 0.0)
            {
                float pillar_d_term = (pillar_error - val->p.pillar_error) * 1000.0;
                pillar_control_val += pillar_d_term * control->pillar.Kd;
                val->p.pillar_error = pillar_error;
            }
        }
        else if (sens->wall.pillar_detected.l)
        {
            // 左柱のみ検出時
            float pillar_error = -(sens->wall.pillar_error.l / 2.0);
            pillar_control_val = pillar_error * control->pillar.Kp;
            
            if (control->pillar.Kd > 0.0)
            {
                float pillar_d_term = (pillar_error - val->p.pillar_error) * 1000.0;
                pillar_control_val += pillar_d_term * control->pillar.Kd;
                val->p.pillar_error = pillar_error;
            }
        }
        
        // 柱制御量をリミット（壁制御の50%程度に制限）
        const float PILLAR_CONTROL_LIMIT = 0.1; // 適宜調整
        if (pillar_control_val > PILLAR_CONTROL_LIMIT)
            pillar_control_val = PILLAR_CONTROL_LIMIT;
        if (pillar_control_val < -PILLAR_CONTROL_LIMIT)
            pillar_control_val = -PILLAR_CONTROL_LIMIT;
        
        val->tar.ang_vel = pillar_control_val;
    }
    else if (sens->wall.control == FALSE && val->angle_control_mode == TRUE)
    {
        // 壁制御OFF時の角度制御（ジャイロPID制御）
        // 目標角度との誤差を計算
        float angle_error = val->start_angle - val->current.rad;
        
        // 角度誤差を-π～πの範囲に正規化
        while (angle_error > M_PI) angle_error -= 2.0 * M_PI;
        while (angle_error < -M_PI) angle_error += 2.0 * M_PI;
        
        // PID制御で目標角速度を計算（角度維持）
        // 既存の角度制御用PIDゲイン(control->d)を使用
        val->tar.ang_vel = angle_error * control->d.Kp;
    }
    // xSemaphoreGive(on_logging);

    return;
}

void Interrupt::feedback_control()
{ // フィードバック制御
    if (control->flag == TRUE && control->test_flag == FALSE)
    {
        control->V_l = 0;
        control->V_r = 0;
        control->Duty_l = 0;
        control->Duty_r = 0;


        // 静止状態の判定
        if (fabs(val->current.vel) < control->stationary_threshold_vel && 
            fabs(val->current.ang_vel) < control->stationary_threshold_ang_vel)
        {
            control->is_stationary = TRUE;
        }
        else
        {
            control->is_stationary = FALSE;
        }

        // FF制御（速度）
        control->Duty_l = FF_VEL_GAIN * (BASE_BATT/sens->battery_voltage) * ff_control_velocity(val->tar.vel);
        control->Duty_r = FF_VEL_GAIN * (BASE_BATT/sens->battery_voltage) * ff_control_velocity(val->tar.vel);
        
        // 静止状態から加速する場合のみ、静摩擦補償を追加
        /*if (control->is_stationary)
        {
            // 前進の場合（直進）
            if (val->tar.vel > control->stationary_threshold_vel && 
                fabs(val->tar.ang_vel) < control->stationary_threshold_ang_vel)
            {
                control->Duty_l += control->static_friction_compensation_straight;
                control->Duty_r += control->static_friction_compensation_straight;
            }
            // 超信地旋回の場合（速度ほぼゼロで角速度あり）
            else if (fabs(val->tar.vel) < control->stationary_threshold_vel && 
                     fabs(val->tar.ang_vel) > control->stationary_threshold_ang_vel)
            {
                // 左旋回の場合
                if (val->tar.ang_vel > 0)
                {
                    control->Duty_l -= control->static_friction_compensation_turn;
                    control->Duty_r += control->static_friction_compensation_turn;
                }
                // 右旋回の場合
                else
                {
                    control->Duty_l -= control->static_friction_compensation_turn;
                    control->Duty_r += control->static_friction_compensation_turn;
                }
            }
        }*/

        // 速度制御
        val->current.vel_error = val->tar.vel - val->current.vel;
        val->I.vel_error += val->current.vel_error / 1000.0;
        //val->p.vel_error = (val->p.vel - val->current.vel) * 1000.0;
        val->p.vel_error = val->current.vel_error;
        
        // 不完全微分
        control->v.D_operation_amount = control->v.N *((control->v.Kd * val->p.vel_error) - control->v.diff);
        control->v.diff += (control->v.D_operation_amount) * (0.001);


        control->V_l = val->current.vel_error * (control->v.Kp) + val->I.vel_error * (control->v.Ki) - control->v.D_operation_amount;
        control->V_r = val->current.vel_error * (control->v.Kp) + val->I.vel_error * (control->v.Ki) - control->v.D_operation_amount;
        

        // FF制御（角速度）
        control->Duty_l -= FF_ANG_VEL_GAIN * (BASE_BATT/sens->battery_voltage) * ff_control_angular_velocity(val->tar.ang_vel);
        control->Duty_r += FF_ANG_VEL_GAIN * (BASE_BATT/sens->battery_voltage) * ff_control_angular_velocity(val->tar.ang_vel);

        // 角速度制御
        val->current.ang_error = val->tar.ang_vel - val->current.ang_vel;
        val->I.ang_error += val->current.ang_error / 1000.0;
        //val->p.ang_error = (val->p.ang_vel - val->current.ang_vel) * 1000.0;
        val->p.ang_error = val->current.ang_error;

        // 不完全微分
        control->o.D_operation_amount = control->o.N *((control->o.Kd * val->p.ang_error) - control->o.diff);
        control->o.diff += (control->o.D_operation_amount) * (0.001);

        control->V_l -= val->current.ang_error * (control->o.Kp) + val->I.ang_error * (control->o.Ki) - control->o.D_operation_amount;
        control->V_r += val->current.ang_error * (control->o.Kp) + val->I.ang_error * (control->o.Ki) - control->o.D_operation_amount;

        
        // FB制御の出力をDutyに加算
        control->Duty_l += control->V_l / sens->battery_voltage; // zero division error に注意
        control->Duty_r += control->V_r / sens->battery_voltage;

        mot->set_motor_speed(control->Duty_r, control->Duty_l);
    }
    else if (control->flag == FALSE && control->test_flag == FALSE)
    {
        control->V_l = 0;
        control->V_r = 0;
        control->Duty_l = 0;
        control->Duty_r = 0;

        mot->set_motor_speed(0.0, 0.0);
    }
    else if (control->flag == FALSE && control->test_flag == TRUE)
    {
        // mot->set_motor_speed(0.0, 0.0); // テスト、実験時は無効にしておかないといけない
    }
    
    val->p.vel = val->current.vel;
    val->p.ang_vel = val->current.ang_vel;

    return;
}

void Interrupt::calc_distance()
{ //  走行距離を計算する

    // エンコーダの値を取得
    sens->enc.data.l = encL->read_angle();
    sens->enc.data.r = encR->read_angle();

    sens->enc.locate.l = sens->enc.data.l;
    sens->enc.locate.r = sens->enc.data.r;

    // 差分を計算
    sens->enc.diff_pulse.l = sens->enc.locate.l - sens->enc.p_locate.l;
    sens->enc.diff_pulse.r = sens->enc.locate.r - sens->enc.p_locate.r;

    // 制御１周期分前の値を保持
    sens->enc.p_locate.l = sens->enc.locate.l;
    sens->enc.p_locate.r = sens->enc.locate.r;

    if (sens->enc.diff_pulse.l > ENC_HALF)
        sens->enc.diff_pulse.l -= ENC_MAX - 1;
    if (sens->enc.diff_pulse.l < -ENC_HALF)
        sens->enc.diff_pulse.l += ENC_MAX - 1;
    if (sens->enc.diff_pulse.r > ENC_HALF)
        sens->enc.diff_pulse.r -= ENC_MAX - 1;
    if (sens->enc.diff_pulse.r < -ENC_HALF)
        sens->enc.diff_pulse.r += ENC_MAX - 1;

    float len_L = sens->enc.diff_pulse.l * MMPP;
    float len_R = sens->enc.diff_pulse.r * MMPP;

    // val->current.len += (len_L + len_R) / 2.0;


    val->l.vel = len_L / 0.001; // 1ms
    val->r.vel = len_R / 0.001;

    // 高精度速度推定システム（エンコーダ+IMU融合）
    estimate_velocity_fusion();


    val->current.len += val->current.vel * 0.001;
    val->sum.len += val->current.vel * 0.001;
    val->I.vel += val->current.vel; // 積分値更新

    return;
}

void Interrupt::estimate_velocity_fusion() // 多分-accel_yが正しい方向
{ // エンコーダ+IMU融合による高精度速度推定システム
    
    // エンコーダベース速度（既存計算）
    float encoder_vel = (val->l.vel + val->r.vel) / 2.0;
    
    // IMU加速度データ取得と処理（バイアス補正を適用）
    float imu_accel = 0.0;
    if (!imu->in_survaeybias) 
    { // サーベイバイアス中は加速度を計算しない
        // 生の加速度値を取得し、バイアスを減算
        float accel_y_raw = imu->accel_y() - sens->accel.y_ref; // G単位、バイアス補正済み
        
        // 向心加速度補正を適用
        float accel_y_compensated = compensate_centripetal_acceleration(accel_y_raw);
        
        imu_accel = -(accel_y_compensated * 9.80665) * 0.001; // m/s^2 から m/ms に変換
        val->current.acc = imu_accel;
        
        // === 進行方向加速度の記録（ログ用） ===
        accel_y_raw = -(accel_y_compensated * 9.80665); // m/s^2 単位で保存（補正済み）
        
        // 移動平均の計算（30ms分）
        accel_y_buffer[accel_buffer_index] = accel_y_raw;
        accel_buffer_index = (accel_buffer_index + 1) % ACCEL_MA_SIZE;
        
        // 移動平均を計算
        float sum = 0.0;
        for (int i = 0; i < ACCEL_MA_SIZE; i++) {
            sum += accel_y_buffer[i];
        }
        accel_y_filtered = sum / ACCEL_MA_SIZE;
    }
    

    //val->current.vel = encoder_vel;
    float enc_vel_filtered = encoder_vel * 0.1 + val->p.vel * 0.9;

    // センサフュージョン：相補フィルタによる速度推定
    // alpha値により重み付けを調整（エンコーダとIMUのバランス）
    float predicted_vel = enc_vel_filtered + accel_y_filtered * 0.001; // IMUによる予測速度
    val->current.vel = val->current.alpha * predicted_vel + (1.0 - val->current.alpha) * enc_vel_filtered;

    //val->current.vel = enc_vel_filtered; // エンコーダベース速度

    return;
}

float Interrupt::compensate_centripetal_acceleration(float accel_y_raw)
{
    // 向心加速度補正処理
    // 入力: 生の加速度（バイアス補正済み、G単位）
    // 出力: 向心加速度補正後の加速度（G単位）
    
    // === 1. センサオフセット位置ベクトル r ===
    // IMUが回転中心からずれている場合の位置ベクトル
    // x = 15.036mm, y = 21.044mm, z = 0mm (実測値)
    float r_x = sens->accel.offset.x; // [m]（前後方向）
    float r_y = sens->accel.offset.y; // [m]（左右方向）
    [[maybe_unused]] float r_z = sens->accel.offset.z; // [m]（上下方向・2D運動のため現状未使用）
    
    // === 2. 角速度と角加速度 ===
    float omega_z = val->current.ang_vel;     // Z軸周りの角速度 [rad/s]
    float alpha_z = ang_accel_filtered;       // Z軸周りの角加速度 [rad/s²]
    
    // === 3. 重力ベクトル（ボディ座標系） ===
    // 2D平面運動（ロール・ピッチが小さい）を仮定
    // 姿勢角が小さければY軸方向への重力成分はほぼゼロ
    float g_body_y = 0.0; // [G] 平面運動のため
    
    // === 4. 向心加速度成分の計算 ===
    // ω × (ω × r) のY成分
    // 2D平面（Z軸周り回転）では: 
    // centripetal_y = -omega_z^2 * r_y
    float centripetal_y = -omega_z * omega_z * r_y / 9.80665; // [G]単位に変換
    
    // === 5. 角加速度による接線加速度成分 ===
    // α × r のY成分
    // 2D平面（Z軸周り回転）では:
    // tangential_y = -alpha_z * r_x
    float tangential_y = -alpha_z * r_x / 9.80665; // [G]単位に変換
    
    // === 6. 補正後の加速度 ===
    // a_corrected = a_measured + g_body - (α × r) - (ω × (ω × r))
    // 並進加速度 = 測定値 - 回転による見かけの加速度
    float accel_y_compensated = accel_y_raw + g_body_y - tangential_y - centripetal_y;
    
    return accel_y_compensated;
}

void Interrupt::calc_angle()
{ //  角度を計算する
    // float _yaw = 0.0;
    if (!imu->in_survaeybias)
    { // サーベイバイアス中は角速度を計算しない
        sens->gyro.yaw = imu->gyro_z() - sens->gyro.ref;
    }

    val->current.ang_vel = sens->gyro.yaw * (M_PI / 180.0);

    val->current.rad += val->current.ang_vel / 1000.0;
    
    // === 角加速度の計算（数値微分） ===
    if (!imu->in_survaeybias) {
        // 角加速度を計算 [rad/s²]
        ang_accel = (val->current.ang_vel - prev_ang_vel) / 0.001; // dt = 1ms
        
        // ローパスフィルタ適用（ノイズ除去）
        // カットオフ周波数調整用: alpha = 0.3 → 約50Hz想定
        const float LPF_ALPHA = 0.3;
        ang_accel_filtered = LPF_ALPHA * ang_accel + (1.0 - LPF_ALPHA) * ang_accel_filtered;
        
        // 前回値を保存
        prev_ang_vel = val->current.ang_vel;
    }

    val->I.ang_vel += val->current.ang_vel; // 角速度積分値更新

    return;
}

void Interrupt::update_odometry()
{
    // === センサベース位置推定（デッドレコニング） ===
    const float dt = 0.001; // 1ms
    
    // 現在の姿勢角を取得（calc_angle()で更新済み）
    float theta = val->current.rad;
    control->odom.theta = theta;
    
    // グローバル座標系での速度成分を計算
    // マイクロマウス座標系: NORTH(Y+)=0, EAST(X+)=π/2
    // NORTH: θ=0 → X=0, Y=1 → sin(0)=0, cos(0)=1
    // EAST:  θ=π/2 → X=1, Y=0 → sin(π/2)=1, cos(π/2)=0
    // しかし、実機ではX軸が反転しているため、sinの符号を反転
    float vel_global = val->current.vel;
    control->odom.vel_x = -vel_global * sin(theta);  // X軸反転のため負号追加
    control->odom.vel_y = vel_global * cos(theta);
    
    // センサベース位置を積分更新（オイラー法）
    control->odom.x_pos += control->odom.vel_x * dt;
    control->odom.y_pos += control->odom.vel_y * dt;
    
    // === 補正後オドメトリ: センサベースと同じ動作 ===
    // 基本はセンサ値で積分（滑らかさ優先）
    control->odom.x_pos_corrected += control->odom.vel_x * dt;
    control->odom.y_pos_corrected += control->odom.vel_y * dt;
    control->odom.theta_corrected = theta;
    
    // === セル単位の補正（モーション完了時のトリガーベース） ===
    if (control->odom_correction_requested) {
        const float CELL_SIZE = 0.09; // 90mm
        const float CELL_CENTER_OFFSET = CELL_SIZE / 2.0; // 45mm
        
        // 補正時のセル座標を使用してセル中心座標を計算
        float cell_center_x = control->correction_cell_x * CELL_SIZE + CELL_CENTER_OFFSET;
        float cell_center_y = control->correction_cell_y * CELL_SIZE + CELL_CENTER_OFFSET;
        
        // 補正時の方向から姿勢角を計算
        float cell_theta = 0.0;
        switch (control->correction_dir) {
            case 0: // NORTH
                cell_theta = 0.0;
                break;
            case 1: // EAST
                cell_theta = M_PI / 2.0;
                break;
            case 2: // SOUTH
                cell_theta = M_PI;
                break;
            case 3: // WEST
                cell_theta = 3.0 * M_PI / 2.0;
                break;
        }
        
        // 補正オドメトリをセル中心にリセット
        control->odom.x_pos_corrected = cell_center_x;
        control->odom.y_pos_corrected = cell_center_y;
        control->odom.theta_corrected = cell_theta;
        
        // フラグをクリア
        control->odom_correction_requested = false;
    }
}

void Interrupt::update_cell_reference_position()
{
    // この関数は真値位置の更新用だが、誤差計算がMATLAB側に移行したため不要
    // 関数自体は互換性のため残しておく（空実装）
}

void Interrupt::calculate_position_error()
{
    // この関数は誤差計算用だが、ログリソース削減のため、
    // 実際の誤差計算はMATLAB側で実施する
    // 関数自体は互換性のため残しておく（空実装）
}

void Interrupt::sync_position_from_cell()
{
    // map->pos.x, map->pos.y が更新されたときに呼び出す
    // センサベース推定位置をセル座標（真値）に同期
    
    const float CELL_SIZE = 0.09;
    const float CELL_CENTER_OFFSET = CELL_SIZE / 2.0;
    
    // センサベース位置をセル中心座標にリセット
    control->odom.x_pos = map->pos.x * CELL_SIZE + CELL_CENTER_OFFSET;
    control->odom.y_pos = map->pos.y * CELL_SIZE + CELL_CENTER_OFFSET;
    
    // 姿勢角を同期
    switch (map->pos.dir) {
        case NORTH:
            control->odom.theta = 0.0;
            break;
        case EAST:
            control->odom.theta = M_PI / 2.0;
            break;
        case SOUTH:
            control->odom.theta = M_PI;
            break;
        case WEST:
            control->odom.theta = 3.0 * M_PI / 2.0;
            break;
    }
    
    val->current.rad = control->odom.theta;
    
    // 真値位置も更新
    update_cell_reference_position();
    
    // 誤差をゼロリセット
    control->odom.x_error = 0.0;
    control->odom.y_error = 0.0;
    control->odom.theta_error = 0.0;
    control->odom.position_error = 0.0;
    control->odom.x_offset = 0.0;
    control->odom.y_offset = 0.0;
    
    // 誤差共分散をリセット
    control->odom.cov_xx = 0.01;
    control->odom.cov_yy = 0.01;
    control->odom.cov_tt = 0.001;
}

void Interrupt::reset_odometry_to_cell_center()
{
    // 壁制御などでセル中心に到達したと判断できる場合に呼び出す
    sync_position_from_cell();
}

void Interrupt::apply_cell_correction()
{
    // === セル単位の位置によるセンサオドメトリの補正 ===
    // セル移動が確認されたタイミングで、センサオドメトリ値をセル位置に基づいて上書き
    
    const float CELL_SIZE = 0.09; // 90mm
    const float CELL_CENTER_OFFSET = CELL_SIZE / 2.0; // 45mm
    
    // 現在のセル座標を計算
    float cell_x = map->pos.x * CELL_SIZE + CELL_CENTER_OFFSET;
    float cell_y = map->pos.y * CELL_SIZE + CELL_CENTER_OFFSET;
    
    // 現在の姿勢角を取得
    float cell_theta = 0.0;
    switch (map->pos.dir) {
        case NORTH:
            cell_theta = 0.0;
            break;
        case EAST:
            cell_theta = M_PI / 2.0;
            break;
        case SOUTH:
            cell_theta = M_PI;
            break;
        case WEST:
            cell_theta = 3.0 * M_PI / 2.0;
            break;
        default:
            cell_theta = 0.0;
            break;
    }
    
    // === 補正適用：センサオドメトリをセル位置で上書き ===
    control->odom.x_pos_corrected = cell_x;
    control->odom.y_pos_corrected = cell_y;
    control->odom.theta_corrected = cell_theta;
    
    // 注意: 誤差計算はMATLAB側で実施（リソース削減のため）
}

void Interrupt::calculate_corrected_position_error()
{
    // この関数は誤差計算用だが、ログリソース削減のため、
    // 実際の誤差計算はMATLAB側で実施する
    // 関数自体は互換性のため残しておく（空実装）
}

void Interrupt::logging()
{
    esp_err_t err;

    const esp_partition_t *partition = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_ANY, "logs");
    if (partition == NULL)
    {
        ESP_LOGE("logging", "partition not found");
        vTaskDelete(NULL);
    }

    err = esp_partition_erase_range(partition, 0, partition->size);
    if (err != ESP_OK)
    {
        ESP_LOGE("logging", "erase error");
        vTaskDelete(NULL);
    }
    uint32_t mem_offset = 0;
    int16_t adcs[37];  // 35列 → 37列に拡張（加速度データ2つ追加）
    //int16_t arr[4];
    //int16_t arr2[4];

    ESP_LOGI("logging", "start logging");

    while (1)
    {

        // if (control->log_flag == TRUE)
        //{
        xSemaphoreTake(*on_logging, portMAX_DELAY); // セマフォが取得できるまで無制限に待機 （他タスクによって解放されるまでブロックされる）
#if CONFIG_RMOUSE_WIFI_ENABLE
        int64_t log_start = esp_timer_get_time(); // ログ書き込み 1 回分の計測開始(OFF 時は除去)
#endif
        // 壁センサ値はuint16_t（0-65535）だが、int16_t配列に格納するためキャストが必要
        // ただし、値の範囲を保持するため、読み出し側でuint16_tとして解釈する必要がある
        adcs[0] = (int16_t)(sens->wall.val.fl);
        adcs[1] = (int16_t)(sens->wall.val.l);
        adcs[2] = (int16_t)(sens->wall.val.r);
        adcs[3] = (int16_t)(sens->wall.val.fr);
        adcs[4] = (uint16_t)(sens->battery_voltage * 1000);
        adcs[5] = (int16_t)(val->current.vel * 1000);
        adcs[6] = (int16_t)(val->tar.vel * 1000);
        adcs[7] = (int16_t)(val->sum.len * 1000);
        adcs[8] = (int16_t)(val->current.ang_vel * 1000);
        adcs[9] = (int16_t)(val->tar.ang_vel * 1000);
        adcs[10] = (int16_t)(val->current.rad * 1000);
        adcs[11] = (int16_t)(val->tar.acc * 1000);
        // 角加速度のみ100倍スケール（オーバーフロー対策）
        // M_PI*80.0 = 251.327 [rad/s²] → 251.327*1000 = 251327 (int16_t範囲外)
        // 100倍スケールなら 25132 (int16_t範囲内: -32768～32767)
        adcs[12] = (int16_t)(val->tar.ang_acc * 100);
        adcs[13] = (int16_t)(val->current.vel_error * 1000);
        adcs[14] = (int16_t)(val->I.vel_error * 1000);
        adcs[15] = (int16_t)(val->p.vel_error * 1000);
        adcs[16] = (int16_t)(val->current.ang_error * 1000);
        adcs[17] = (int16_t)(val->I.ang_error * 1000);
        adcs[18] = (int16_t)(val->p.ang_error * 1000);
        adcs[19] = (int16_t)(control->Duty_l * 1000);
        adcs[20] = (int16_t)(control->Duty_r * 1000);
        adcs[21] = (int16_t)(sens->enc.data.l);
        adcs[22] = (int16_t)(sens->enc.data.r);
        adcs[23] = (int16_t)(val->current.len * 1000);
        adcs[24] = (int16_t)(val->tar.len * 1000);
        adcs[25] = delta_time;
        adcs[26] = map->thinking_flag;
        
        // === オドメトリデータ (27-34) ===
        adcs[27] = (int16_t)(control->odom.x_pos * 1000);          // 生センサ推定X [mm]
        adcs[28] = (int16_t)(control->odom.y_pos * 1000);          // 生センサ推定Y [mm]
        adcs[29] = (int16_t)(control->odom.theta * 1000);          // 生センサ推定θ [mrad]
        adcs[30] = (int16_t)(control->odom.x_pos_corrected * 1000); // セル補正後X [mm]
        adcs[31] = (int16_t)(control->odom.y_pos_corrected * 1000); // セル補正後Y [mm]
        adcs[32] = (int16_t)(control->odom.theta_corrected * 1000); // セル補正後θ [mrad]
        adcs[33] = (int16_t)(map->pos.x);                           // 真値セルX座標
        adcs[34] = (int16_t)(map->pos.y);                           // 真値セルY座標
        
        // === 加速度データ (35-36) ===
        adcs[35] = (int16_t)(accel_y_raw * 1000);                   // 生の加速度 [mm/s²]
        adcs[36] = (int16_t)(accel_y_filtered * 1000);              // 移動平均後の加速度 [mm/s²]
        
        err = esp_partition_write(partition, mem_offset, adcs, sizeof(adcs));
        if (err != ESP_OK)
        {
            ESP_LOGE("logging", "write error");
            printf("%s\n", esp_err_to_name(err));

            break;
        }
        mem_offset += sizeof(adcs);

#if CONFIG_RMOUSE_WIFI_ENABLE
        // 1 回の書き込み時間と起動間隔をテレメトリへ公開(イベント駆動のため idle 時は更新されない)。
        static int64_t log_prev_start = 0;
        uint32_t log_period = (log_prev_start != 0) ? static_cast<uint32_t>(log_start - log_prev_start) : 0;
        log_prev_start = log_start;
        telemetry_report_loop(TELEM_TASK_LOG,
                              static_cast<uint32_t>(esp_timer_get_time() - log_start), log_period);
#endif

        if (mem_offset >= partition->size)
            break;

        //}
        // vTaskDelay(1 / portTICK_PERIOD_MS);
    }
    
    vTaskDelete(NULL);
    return;
}

void Interrupt::reset_i_gain()
{
    val->I.vel = 0.0;
    val->I.ang_vel = 0.0;
    val->I.wall_error = 0.0;
    val->p.pillar_error = 0.0;  // 柱制御の前回値もリセット
    return;
}

float Interrupt::calc_target_accel()
{
    return ((val->end.vel) * (val->end.vel) - (val->current.vel) * (val->current.vel)) / (2.0 * val->tar.len);
}

float Interrupt::ff_control_velocity(float reference_input)
{
    // 差分方程式: out[k] = -a1*out[k-1] - a2*out[k-2] + b0*in[k] + b1*in[k-1] + b2*in[k-2]
    // a1 = -1.8, a2 = 0.81, b0 = 0.1848, b1 = -0.0092, b2 = -0.1743

    // 入力信号の更新（最新を ff_vel_in[0] に格納）
    ff_vel_in[2] = ff_vel_in[1];
    ff_vel_in[1] = ff_vel_in[0];
    ff_vel_in[0] = reference_input;

    // 差分方程式の計算
    float output = 1.8 * ff_vel_out[0] - 0.81 * ff_vel_out[1] + 0.1848 * ff_vel_in[0] - 0.0092 * ff_vel_in[1] - 0.1743 * ff_vel_in[2];

    // 出力信号の更新（最新を ff_vel_out[0] に格納）
    ff_vel_out[2] = ff_vel_out[1];
    ff_vel_out[1] = ff_vel_out[0];
    ff_vel_out[0] = output;

    return output;
}

float Interrupt::ff_control_angular_velocity(float reference_input)
{
    // 角速度用差分方程式: out[k] = -a1*out[k-1] - a2*out[k-2] + b0*in[k] + b1*in[k-1] + b2*in[k-2]
    // TODO: 角速度用の係数を同定して設定してください
    // 現在は速度制御と同じ係数を使用（仮）
    // a1 = -1.8, a2 = 0.81, b0 = 0.1848, b1 = -0.0092, b2 = -0.1743

    // 入力信号の更新（最新を ff_ang_in[0] に格納）
    ff_ang_in[2] = ff_ang_in[1];
    ff_ang_in[1] = ff_ang_in[0];
    ff_ang_in[0] = reference_input;

    // 差分方程式の計算
    float output = 1.8 * ff_ang_out[0] - 0.81 * ff_ang_out[1] + 0.0346 * ff_ang_in[0] - 0.0615 * ff_ang_in[1] + 0.0270 * ff_ang_in[2];

    // 出力信号の更新（最新を ff_ang_out[0] に格納）
    ff_ang_out[2] = ff_ang_out[1];
    ff_ang_out[1] = ff_ang_out[0];
    ff_ang_out[0] = output;

    return output;
}

void Interrupt::interrupt()
{ //  xtaskcreate
    val->current.alpha = 0.9;

    while (1)
    {
        start_time = esp_timer_get_time();

        calc_target();
        wall_control();
        feedback_control();
        calc_distance();
        calc_angle();
        
        // オドメトリ更新
        update_odometry();

        if (control->log_flag == TRUE)
        {
            xSemaphoreGive(*on_logging);
        }

        if (map->search_count_flag == TRUE)
        {
            map->search_time++;
        }

        //control->time_count++;

        end_time = esp_timer_get_time();
        delta_time = end_time - start_time;

#if CONFIG_RMOUSE_WIFI_ENABLE
        // ループ実行時間(delta_time)と実周期(前回開始からの差)をテレメトリへ公開。
        // OFF 時は本ブロックごと除去され、従来挙動・サイズに影響しない。
        static int64_t prev_start = 0;
        uint32_t period_us = (prev_start != 0) ? static_cast<uint32_t>(start_time - prev_start) : 0;
        prev_start = start_time;
        telemetry_report_loop(TELEM_TASK_INTERRUPT, static_cast<uint32_t>(delta_time), period_us);
#endif

        vTaskDelay(1 / portTICK_PERIOD_MS);


    }
}
