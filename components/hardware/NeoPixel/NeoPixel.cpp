#include "NeoPixel.hpp"

NeoPixel::NeoPixel(gpio_num_t pin, int _leds = 100){
    rmt_tx_channel_config_t _tx_config;
    memset(&_tx_config, 0, sizeof(_tx_config));
    _tx_config.clk_src = RMT_CLK_SRC_DEFAULT;
    _tx_config.gpio_num = pin;
    _tx_config.mem_block_symbols = 48;
    _tx_config.resolution_hz = RMT_RESOLUTION;
    _tx_config.trans_queue_depth = 1;

    this->tx_config.loop_count = 0;

    ESP_ERROR_CHECK(rmt_new_tx_channel(&_tx_config, &neopixel_ch));
    ESP_ERROR_CHECK(encoder_rmt_create());
    ESP_ERROR_CHECK(rmt_enable(neopixel_ch));

    leds = _leds;
    grb = (rgb_t*)calloc(leds, sizeof(rgb_t));
    ESP_ERROR_CHECK(grb ? ESP_OK : ESP_ERR_NO_MEM);
}

NeoPixel::~NeoPixel(){
    encoder_delete(neopixel_enc);
    rmt_del_channel(neopixel_ch);
}

void NeoPixel::set(rgb_t _grb, uint start = 0,uint len = 1){
    if(start >= leds){
        return;
    }
    for(int i = start;i < start + len;i++){
        if(i >= leds){
            break;
        }
        grb[i] = _grb;
    }
}

void NeoPixel::show(){
    ESP_ERROR_CHECK(rmt_transmit(this->neopixel_ch,this->neopixel_enc,grb,sizeof(rgb_t)*leds,&this->tx_config));
    ESP_ERROR_CHECK(rmt_tx_wait_all_done(this->neopixel_ch, portMAX_DELAY));
}

esp_err_t NeoPixel::encoder_rmt_create(){
    esp_err_t ret = ESP_OK;
    uint32_t reset_ticks;
    rmt_symbol_word_t _reset;

    neopixel_encoder_t *led_encoder = NULL;
    led_encoder = (neopixel_encoder_t*)calloc(1, sizeof(neopixel_encoder_t));
    ESP_GOTO_ON_FALSE(led_encoder, ESP_ERR_NO_MEM, err, this->TAG, "calloc failed");
    led_encoder->base.encode = this->encoder_rmt;
    led_encoder->base.del = this->encoder_delete;
    led_encoder->base.reset = this->encoder_reset;

    rmt_bytes_encoder_config_t bytes_encoder_config;
    bytes_encoder_config.bit0.level0 = 1;
    bytes_encoder_config.bit0.duration0 = 0.3 * RMT_RESOLUTION / 1000000, // T0H=0.3us
    bytes_encoder_config.bit0.level1 = 0,
    bytes_encoder_config.bit0.duration1 = 0.9 * RMT_RESOLUTION / 1000000, // T0L=0.9us

    bytes_encoder_config.bit1.level0 = 1,
    bytes_encoder_config.bit1.duration0 = 0.9 * RMT_RESOLUTION / 1000000, // T1H=0.9us
    bytes_encoder_config.bit1.level1 = 0,
    bytes_encoder_config.bit1.duration1 = 0.3 * RMT_RESOLUTION / 1000000, // T1L=0.3us
    bytes_encoder_config.flags.msb_first = 1;

    ESP_GOTO_ON_ERROR(rmt_new_bytes_encoder(&bytes_encoder_config, &led_encoder->bytes_encoder), err, TAG, "rmt_new_bytes_encoder failed");
    ESP_GOTO_ON_ERROR(rmt_new_copy_encoder(&this->copy_enc,&led_encoder->copy_encoder), err, TAG, "rmt_new_copy_encoder failed");

    reset_ticks = RMT_RESOLUTION / 1000000 * 50 / 2;
    _reset.level0 = 0;
    _reset.duration0 = reset_ticks;
    _reset.level1 = 0;
    _reset.duration1 = reset_ticks;
    led_encoder->reset_code = _reset;

    neopixel_enc = &led_encoder->base;
    return ESP_OK;

err:
    if(led_encoder){
        if(led_encoder->bytes_encoder){
            rmt_del_encoder(led_encoder->bytes_encoder);
        }
        if(led_encoder->copy_encoder){
            rmt_del_encoder(led_encoder->copy_encoder);
        }
        free(led_encoder);
    }
    return ret;
}

size_t NeoPixel::encoder_rmt(rmt_encoder_t *encoder, rmt_channel_handle_t channel, const void *primary_data, size_t data_size, rmt_encode_state_t *ret_state){
    neopixel_encoder_t *led_encoder = __containerof(encoder, neopixel_encoder_t, base);
    rmt_encoder_handle_t bytes_encoder = led_encoder->bytes_encoder;
    rmt_encoder_handle_t copy_encoder = led_encoder->copy_encoder;
    rmt_encode_state_t session_state = RMT_ENCODING_RESET;
    int state = RMT_ENCODING_RESET;
    size_t encoded_symbols = 0;

    switch(led_encoder->state){
        case 0:
            encoded_symbols += bytes_encoder->encode(bytes_encoder, channel, primary_data, data_size, &session_state);
            if(session_state & RMT_ENCODING_COMPLETE){
                led_encoder->state = 1;
            }
            if(session_state & RMT_ENCODING_MEM_FULL){
                state |= RMT_ENCODING_MEM_FULL;
                goto out;
            }
            break;
        case 1:
            encoded_symbols += copy_encoder->encode(copy_encoder, channel, &led_encoder->reset_code, sizeof(led_encoder->reset_code), &session_state);
            if(session_state & RMT_ENCODING_COMPLETE){
                led_encoder->state = RMT_ENCODING_RESET;
                state |= RMT_ENCODING_COMPLETE;
            }
            if(session_state & RMT_ENCODING_MEM_FULL){
                state |= static_cast<rmt_encode_state_t>(RMT_ENCODING_MEM_FULL);
                goto out;
            }
            break;
    }
out:
    *ret_state = (rmt_encode_state_t)state;
    return encoded_symbols;
}

esp_err_t NeoPixel::encoder_delete(rmt_encoder_t *encoder){
    neopixel_encoder_t *led_encoder = __containerof(encoder, neopixel_encoder_t, base);
    rmt_del_encoder(led_encoder->bytes_encoder);
    rmt_del_encoder(led_encoder->copy_encoder);
    free(led_encoder);
    return ESP_OK;
}

esp_err_t NeoPixel::encoder_reset(rmt_encoder_t *encoder){
    neopixel_encoder_t *led_encoder = __containerof(encoder, neopixel_encoder_t, base);
    rmt_encoder_reset(led_encoder->bytes_encoder);
    rmt_encoder_reset(led_encoder->copy_encoder);
    led_encoder->state = RMT_ENCODING_RESET;
    return ESP_OK;
}

void NeoPixel::set_hsv(hsv_t hsv, uint start, uint len){
    uint32_t h = hsv.h;
    uint32_t s = hsv.s;
    uint32_t v = hsv.v;

    h %= 360; // h -> [0,360]
    uint32_t rgb_max = v * 2.55f;
    uint32_t rgb_min = rgb_max * (100 - s) / 100.0f;

    uint32_t i = h / 60;
    uint32_t diff = h % 60;

    // RGB adjustment amount by hue
    uint32_t rgb_adj = (rgb_max - rgb_min) * diff / 60;

    rgb_t rgb;

    switch (i) {
    case 0:
        rgb.r = rgb_max;
        rgb.g = rgb_min + rgb_adj;
        rgb.b = rgb_min;
        break;
    case 1:
        rgb.r = rgb_max - rgb_adj;
        rgb.g = rgb_max;
        rgb.b = rgb_min;
        break;
    case 2:
        rgb.r = rgb_min;
        rgb.g = rgb_max;
        rgb.b = rgb_min + rgb_adj;
        break;
    case 3:
        rgb.r = rgb_min;
        rgb.g = rgb_max - rgb_adj;
        rgb.b = rgb_max;
        break;
    case 4:
        rgb.r = rgb_min + rgb_adj;
        rgb.g = rgb_min;
        rgb.b = rgb_max;
        break;
    default:
        rgb.r = rgb_max;
        rgb.g = rgb_min;
        rgb.b = rgb_max - rgb_adj;
        break;
    }

    set(rgb, start, len);
}

void NeoPixel::gaming_mouse(){
    hsv_t hsv;
    hsv.s = 100;  // 彩度: 100%
    hsv.v = 50;  // 明るさ: 100%

    while (1) {
        for (int h = 0; h < 360; h += 5) { // 0°から360°まで変化
            hsv.h = h;
            set_hsv(hsv, 0, 1); // 全LEDに適用
            show(); // 色を反映

            vTaskDelay(pdMS_TO_TICKS(50)); // 100ms待機
        }
    }
}