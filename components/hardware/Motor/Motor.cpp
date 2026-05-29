#include "Motor.hpp"

#define BDC_MCPWM_TIMER_RESOLUTION_HZ 10000000                                      // 10MHz, 1 tick = 0.1us
#define BDC_MCPWM_FREQ_HZ 150000                                                    // 250KHz PWM
#define BDC_MCPWM_DUTY_TICK_MAX (BDC_MCPWM_TIMER_RESOLUTION_HZ / BDC_MCPWM_FREQ_HZ) // maximum value we can set for the duty cycle, in ticks (=PWM分解能 40(5bit))

mcpwm_cmpr_handle_t comparator_r = NULL;
mcpwm_cmpr_handle_t comparator_l = NULL;

Motor::Motor(gpio_num_t _ph_pin_R, gpio_num_t _en_pin_R, gpio_num_t _ph_pin_L, gpio_num_t _en_pin_L, gpio_num_t _fan_pin, gpio_num_t _mode_pin)
{
    ph_pin_R = _ph_pin_R;
    ph_pin_L = _ph_pin_L;
    fan_pin = _fan_pin;
    en_pin_R = _en_pin_R;
    en_pin_L = _en_pin_L;

    gpio_config_t io_conf = {};
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask =
        (1ULL << fan_pin) | (1ULL << ph_pin_L) | (1ULL << ph_pin_R) | (1ULL << en_pin_L) | (1ULL << en_pin_R) | (1ULL << _mode_pin);
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;

    ESP_ERROR_CHECK(gpio_config(&io_conf));
    ESP_ERROR_CHECK(gpio_set_level(fan_pin, 0));
    ESP_ERROR_CHECK(gpio_set_level(ph_pin_L, 0));
    ESP_ERROR_CHECK(gpio_set_level(ph_pin_R, 0));
    ESP_ERROR_CHECK(gpio_set_level(en_pin_L, 0));
    ESP_ERROR_CHECK(gpio_set_level(en_pin_R, 0));
    ESP_ERROR_CHECK(gpio_set_level(_mode_pin, 1));

    mcpwm_timer_handle_t timer = NULL;
    mcpwm_timer_config_t timer_config;
    timer_config.group_id = 0,
    timer_config.clk_src = MCPWM_TIMER_CLK_SRC_DEFAULT;
    timer_config.resolution_hz = BDC_MCPWM_TIMER_RESOLUTION_HZ;
    timer_config.period_ticks = BDC_MCPWM_DUTY_TICK_MAX;
    timer_config.intr_priority = 0;
    timer_config.count_mode = MCPWM_TIMER_COUNT_MODE_UP;
    ESP_ERROR_CHECK(mcpwm_new_timer(&timer_config, &timer));

    mcpwm_oper_handle_t oper = NULL;
    mcpwm_operator_config_t operator_config = {};
    operator_config.group_id = 0; // operator must be in the same group to the timer
    ESP_ERROR_CHECK(mcpwm_new_operator(&operator_config, &oper));
    ESP_ERROR_CHECK(mcpwm_operator_connect_timer(oper, timer));

    // コンパレーターを作る
    mcpwm_comparator_config_t comparator_config = {};
    comparator_config.flags.update_cmp_on_tez = true;
    ESP_ERROR_CHECK(mcpwm_new_comparator(oper, &comparator_config, &comparator_r));
    ESP_ERROR_CHECK(mcpwm_new_comparator(oper, &comparator_config, &comparator_l));

    // ジェネレーターを作る
    mcpwm_gen_handle_t generator_r = NULL;
    mcpwm_gen_handle_t generator_l = NULL;
    mcpwm_generator_config_t generator_config = {};
    generator_config.gen_gpio_num = en_pin_R;
    ESP_ERROR_CHECK(mcpwm_new_generator(oper, &generator_config, &generator_r));
    generator_config.gen_gpio_num = en_pin_L;
    ESP_ERROR_CHECK(mcpwm_new_generator(oper, &generator_config, &generator_l));

    // set the initial compare value
    ESP_ERROR_CHECK(mcpwm_comparator_set_compare_value(comparator_r, 0));
    ESP_ERROR_CHECK(mcpwm_comparator_set_compare_value(comparator_l, 0));

    // go high on counter empty
    ESP_ERROR_CHECK(mcpwm_generator_set_action_on_timer_event(generator_r,
                                                              MCPWM_GEN_TIMER_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP, MCPWM_TIMER_EVENT_EMPTY, MCPWM_GEN_ACTION_HIGH)));
    ESP_ERROR_CHECK(mcpwm_generator_set_action_on_timer_event(generator_l,
                                                              MCPWM_GEN_TIMER_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP, MCPWM_TIMER_EVENT_EMPTY, MCPWM_GEN_ACTION_HIGH)));
    // go low on compare threshold
    ESP_ERROR_CHECK(mcpwm_generator_set_action_on_compare_event(generator_r,
                                                                MCPWM_GEN_COMPARE_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP, comparator_r, MCPWM_GEN_ACTION_LOW)));
    ESP_ERROR_CHECK(mcpwm_generator_set_action_on_compare_event(generator_l,
                                                                MCPWM_GEN_COMPARE_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP, comparator_l, MCPWM_GEN_ACTION_LOW)));

    // タイマーを有効にしてスタート
    ESP_ERROR_CHECK(mcpwm_timer_enable(timer));
    ESP_ERROR_CHECK(mcpwm_timer_start_stop(timer, MCPWM_TIMER_START_NO_STOP));

    // 吸引ファン
    ledc_timer_config_t ledc_timer = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .duty_resolution = LEDC_TIMER_8_BIT,
        .timer_num = LEDC_TIMER_1,
        .freq_hz = 100 * 1000,
        .clk_cfg = LEDC_AUTO_CLK};
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

    // Prepare and then apply the LEDC PWM channel configuration
    ledc_channel_config_t ledc_channel = {
        .gpio_num = fan_pin,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = LEDC_CHANNEL_0,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = LEDC_TIMER_1,
        .duty = 0,
        .hpoint = 0};

    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));

    set_motor_speed(0, 0);
    set_fan_speed(0);

    ESP_LOGI("Motor", "Initialized");
}
Motor::~Motor() {}

void Motor::set_motor_speed(float spdR, float spdL)
{
    // 左右モータの+-に注意
    if (spdR > 0)
    {
        gpio_set_level(ph_pin_R, 0);
    }
    else
    {
        gpio_set_level(ph_pin_R, 1);
        spdR = -spdR;
    }
    if (spdL > 0)
    {
        gpio_set_level(ph_pin_L, 1);
    }
    else
    {
        gpio_set_level(ph_pin_L, 0);
        spdL = -spdL;
    }

    uint32_t dutyR = spdR * BDC_MCPWM_DUTY_TICK_MAX;
    uint32_t dutyL = spdL * BDC_MCPWM_DUTY_TICK_MAX;

    if(dutyR > BDC_MCPWM_DUTY_TICK_MAX) dutyR = BDC_MCPWM_DUTY_TICK_MAX;
    if(dutyL > BDC_MCPWM_DUTY_TICK_MAX) dutyL = BDC_MCPWM_DUTY_TICK_MAX;

    ESP_LOGD("Motor", "dutyR: %ld, dutyL: %ld", dutyR, dutyL);

    esp_err_t MCPWM_ERROR_R = mcpwm_comparator_set_compare_value(comparator_r, dutyR);
    esp_err_t MCPWM_ERROR_L = mcpwm_comparator_set_compare_value(comparator_l, dutyL);

    if (MCPWM_ERROR_R != ESP_OK)
    {
        ESP_LOGE("Motor", "MCPWM_ERROR_R: %d", MCPWM_ERROR_R);
    }
    if (MCPWM_ERROR_L != ESP_OK)
    {
        ESP_LOGE("Motor", "MCPWM_ERROR_L: %d", MCPWM_ERROR_L);
    }
}

void Motor::set_fan_speed(float fan)
{
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, fan * 256);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
}