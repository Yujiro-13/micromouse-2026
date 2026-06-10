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

#endif // CONFIG_RMOUSE_WIFI_ENABLE
