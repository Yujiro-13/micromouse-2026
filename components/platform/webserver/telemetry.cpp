#include "telemetry.hpp"

#if CONFIG_RMOUSE_WIFI_ENABLE

#include <cstdio>
#include "cJSON.h"
#include "esp_app_desc.h"
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "esp_system.h"
#include "esp_mac.h"
#include "esp_netif.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "sdkconfig.h"

namespace
{
constexpr const char *TAG = "telem";

// 制御層から公開された読み取り専用ポインタ。Phase 4(WS 配信)でスナップショット生成に使う。
const SensorData *s_sens = nullptr;
const MotionValues *s_val = nullptr;
const Control *s_control = nullptr;
const MazeMap *s_map = nullptr;

// WS 配信レート/一時停止の状態(クライアント cmd で更新)。単語アクセスのみでロックフリー。
int s_rate_hz = CONFIG_RMOUSE_TELEMETRY_HZ;
bool s_paused = false;

const char *chip_model_str(esp_chip_model_t model)
{
    switch (model)
    {
    case CHIP_ESP32:
        return "ESP32";
    case CHIP_ESP32S2:
        return "ESP32-S2";
    case CHIP_ESP32S3:
        return "ESP32-S3";
    case CHIP_ESP32C3:
        return "ESP32-C3";
    case CHIP_ESP32C2:
        return "ESP32-C2";
    case CHIP_ESP32C6:
        return "ESP32-C6";
    case CHIP_ESP32H2:
        return "ESP32-H2";
    default:
        return "unknown";
    }
}

// 指定 ifkey の netif から IPv4 を文字列化して root へ追加(存在しなければ何もしない)。
void add_netif_ip(cJSON *root, const char *ifkey, const char *json_key)
{
    esp_netif_t *netif = esp_netif_get_handle_from_ifkey(ifkey);
    if (netif == nullptr)
    {
        return;
    }
    esp_netif_ip_info_t ip = {};
    if (esp_netif_get_ip_info(netif, &ip) != ESP_OK)
    {
        return;
    }
    char buf[16];
    snprintf(buf, sizeof(buf), IPSTR, IP2STR(&ip.ip));
    cJSON_AddStringToObject(root, json_key, buf);
}

} // namespace

void telemetry_bind(const SensorData *sens, const MotionValues *val,
                    const Control *control, const MazeMap *map)
{
    s_sens = sens;
    s_val = val;
    s_control = control;
    s_map = map;
    ESP_LOGI(TAG, "telemetry bound (sens=%p val=%p control=%p map=%p)",
             static_cast<const void *>(sens), static_cast<const void *>(val),
             static_cast<const void *>(control), static_cast<const void *>(map));
}

char *telemetry_info_json(void)
{
    cJSON *root = cJSON_CreateObject();
    if (root == nullptr)
    {
        return nullptr;
    }

    // --- ファームウェア ---
    const esp_app_desc_t *app = esp_app_get_description();
    if (app != nullptr)
    {
        char build[32];
        snprintf(build, sizeof(build), "%s %s", app->date, app->time);
        cJSON_AddStringToObject(root, "proj", app->project_name);
        cJSON_AddStringToObject(root, "fw", app->version);
        cJSON_AddStringToObject(root, "build", build);
        cJSON_AddStringToObject(root, "idf", app->idf_ver);
    }

    // --- チップ ---
    esp_chip_info_t chip = {};
    esp_chip_info(&chip);
    cJSON *cobj = cJSON_AddObjectToObject(root, "chip");
    if (cobj != nullptr)
    {
        cJSON_AddStringToObject(cobj, "model", chip_model_str(chip.model));
        cJSON_AddNumberToObject(cobj, "cores", chip.cores);
        cJSON_AddNumberToObject(cobj, "rev", chip.revision);
        uint32_t flash_size = 0;
        if (esp_flash_get_size(nullptr, &flash_size) == ESP_OK)
        {
            cJSON_AddNumberToObject(cobj, "flash_mb",
                                    static_cast<double>(flash_size) / (1024.0 * 1024.0));
        }
    }

    // --- MAC(STA 基準の base MAC) ---
    uint8_t mac[6] = {};
    if (esp_read_mac(mac, ESP_MAC_WIFI_STA) == ESP_OK)
    {
        char macstr[18];
        snprintf(macstr, sizeof(macstr), "%02x:%02x:%02x:%02x:%02x:%02x",
                 mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
        cJSON_AddStringToObject(root, "mac", macstr);
    }

    // --- ネットワーク ---
    cJSON_AddStringToObject(root, "host", CONFIG_RMOUSE_WIFI_HOSTNAME);
    cJSON_AddNumberToObject(root, "port", CONFIG_RMOUSE_TELEMETRY_PORT);
    add_netif_ip(root, "WIFI_AP_DEF", "ap_ip");
    add_netif_ip(root, "WIFI_STA_DEF", "sta_ip");

    // --- ヒープ ---
    cJSON_AddNumberToObject(root, "heap", esp_get_free_heap_size());
    cJSON_AddNumberToObject(root, "heap_min", esp_get_minimum_free_heap_size());

    // --- テレメトリ設定 / バインド状態 ---
    cJSON_AddNumberToObject(root, "tel_hz", CONFIG_RMOUSE_TELEMETRY_HZ);
    cJSON_AddBoolToObject(root, "bound", s_sens != nullptr);

    char *out = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    return out; // 呼び出し側が free()
}

char *telemetry_frame_json(void)
{
    cJSON *root = cJSON_CreateObject();
    if (root == nullptr)
    {
        return nullptr;
    }

    // --- envelope(§5.10): 型付き封筒 + スキーマ版 v + タイムスタンプ ---
    cJSON_AddStringToObject(root, "type", "telemetry");
    cJSON_AddNumberToObject(root, "v", 1);
    cJSON_AddNumberToObject(root, "t", static_cast<double>(esp_timer_get_time() / 1000)); // ms
    cJSON *data = cJSON_AddObjectToObject(root, "data");

    // --- hw(本フェーズはヒープのみ。タスク loop/CPU% は Phase 6) ---
    cJSON *hw = cJSON_AddObjectToObject(data, "hw");
    if (hw != nullptr)
    {
        cJSON_AddNumberToObject(hw, "heap", esp_get_free_heap_size());
        cJSON_AddNumberToObject(hw, "heap_min", esp_get_minimum_free_heap_size());
    }
    cJSON_AddBoolToObject(data, "bound", s_sens != nullptr);

    // --- sens ---
    if (s_sens != nullptr)
    {
        const SensorData *s = s_sens;
        cJSON *sens = cJSON_AddObjectToObject(data, "sens");
        cJSON *wall = cJSON_AddObjectToObject(sens, "wall");
        cJSON_AddNumberToObject(wall, "fl", s->wall.val.fl);
        cJSON_AddNumberToObject(wall, "l", s->wall.val.l);
        cJSON_AddNumberToObject(wall, "r", s->wall.val.r);
        cJSON_AddNumberToObject(wall, "fr", s->wall.val.fr);
        cJSON *exist = cJSON_AddObjectToObject(sens, "exist");
        cJSON_AddBoolToObject(exist, "fl", s->wall.exist.fl);
        cJSON_AddBoolToObject(exist, "l", s->wall.exist.l);
        cJSON_AddBoolToObject(exist, "r", s->wall.exist.r);
        cJSON_AddBoolToObject(exist, "fr", s->wall.exist.fr);
        cJSON_AddNumberToObject(sens, "batt", s->battery_voltage);
        cJSON *gyro = cJSON_AddObjectToObject(sens, "gyro");
        cJSON_AddNumberToObject(gyro, "deg", s->gyro.degree);
        cJSON_AddNumberToObject(gyro, "rad", s->gyro.radian);
        cJSON_AddNumberToObject(gyro, "ref", s->gyro.ref);
        cJSON *enc = cJSON_AddObjectToObject(sens, "enc");
        cJSON_AddNumberToObject(enc, "l", s->enc.data.l);
        cJSON_AddNumberToObject(enc, "r", s->enc.data.r);
    }

    // --- pose(自己位置: control->odom) ---
    if (s_control != nullptr)
    {
        const Odometry *o = &s_control->odom;
        cJSON *pose = cJSON_AddObjectToObject(data, "pose");
        cJSON_AddNumberToObject(pose, "x", o->x_pos);
        cJSON_AddNumberToObject(pose, "y", o->y_pos);
        cJSON_AddNumberToObject(pose, "th", o->theta);
        cJSON_AddNumberToObject(pose, "xc", o->x_pos_corrected);
        cJSON_AddNumberToObject(pose, "yc", o->y_pos_corrected);
        cJSON_AddNumberToObject(pose, "thc", o->theta_corrected);
        cJSON_AddNumberToObject(pose, "perr", o->position_error);
        cJSON_AddNumberToObject(pose, "therr", o->theta_error);
        cJSON_AddNumberToObject(pose, "vx", o->vel_x);
        cJSON_AddNumberToObject(pose, "vy", o->vel_y);
    }

    // --- motion(走行値: val + Duty は control) ---
    if (s_val != nullptr)
    {
        const MotionValues *v = s_val;
        cJSON *motion = cJSON_AddObjectToObject(data, "motion");
        cJSON_AddNumberToObject(motion, "vel", v->current.vel);
        cJSON_AddNumberToObject(motion, "vtar", v->tar.vel);
        cJSON_AddNumberToObject(motion, "w", v->current.ang_vel);
        cJSON_AddNumberToObject(motion, "wtar", v->tar.ang_vel);
        cJSON_AddNumberToObject(motion, "len", v->current.len);
        if (s_control != nullptr)
        {
            cJSON_AddNumberToObject(motion, "dl", s_control->Duty_l);
            cJSON_AddNumberToObject(motion, "dr", s_control->Duty_r);
        }
    }

    // --- stat(その他: モード/フラグ/セル) ---
    if (s_map != nullptr)
    {
        const MazeMap *m = s_map;
        cJSON *stat = cJSON_AddObjectToObject(data, "stat");
        cJSON_AddBoolToObject(stat, "think", m->thinking_flag != FALSE);
        cJSON_AddNumberToObject(stat, "stime", static_cast<double>(m->search_time));
        cJSON *cell = cJSON_AddObjectToObject(stat, "cell");
        cJSON_AddNumberToObject(cell, "x", m->pos.x);
        cJSON_AddNumberToObject(cell, "y", m->pos.y);
        cJSON_AddNumberToObject(cell, "dir", m->pos.dir);
        if (s_control != nullptr)
        {
            cJSON_AddBoolToObject(stat, "log", s_control->log_flag != FALSE);
        }
    }

    char *out = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    return out; // 呼び出し側が free()
}

void telemetry_handle_cmd(const char *data, int len)
{
    if (data == nullptr || len <= 0)
    {
        return;
    }
    cJSON *root = cJSON_ParseWithLength(data, static_cast<size_t>(len));
    if (root == nullptr)
    {
        return;
    }

    // envelope {"type":"cmd","data":{...}} なら data を、なければトップレベルを見る。
    cJSON *obj = root;
    cJSON *d = cJSON_GetObjectItem(root, "data");
    if (cJSON_IsObject(d))
    {
        obj = d;
    }

    cJSON *rate = cJSON_GetObjectItem(obj, "rate");
    if (cJSON_IsNumber(rate))
    {
        int v = static_cast<int>(rate->valuedouble);
        if (v < 1) v = 1;
        if (v > 50) v = 50;
        s_rate_hz = v;
        ESP_LOGI(TAG, "WS cmd: rate=%d Hz", v);
    }

    cJSON *pause = cJSON_GetObjectItem(obj, "pause");
    if (cJSON_IsBool(pause))
    {
        s_paused = cJSON_IsTrue(pause);
        ESP_LOGI(TAG, "WS cmd: pause=%s", s_paused ? "true" : "false");
    }

    cJSON_Delete(root);
}

int telemetry_rate_hz(void)
{
    int r = s_rate_hz;
    if (r < 1) r = 1;
    if (r > 50) r = 50;
    return r;
}

bool telemetry_paused(void)
{
    return s_paused;
}

#endif // CONFIG_RMOUSE_WIFI_ENABLE
