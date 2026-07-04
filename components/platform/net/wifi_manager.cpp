#include "wifi_manager.hpp"

#if CONFIG_RMOUSE_WIFI_ENABLE

#include <string.h>
#include "sdkconfig.h"
#include "nvs_flash.h"
#include "esp_wifi.h"
#include "esp_wifi_default.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_mac.h"
#include "esp_log.h"
#include "mdns.h"

namespace
{
constexpr const char *TAG = "wifi";

// STA を実際に接続させるか（空 SSID のときは false のまま）。
bool s_sta_enabled = false;
// STA 再接続のリトライ回数（IP 取得でリセット）。ログ抑制用。
int s_sta_retry = 0;

void on_wifi_event(void *arg, esp_event_base_t base, int32_t id, void *data)
{
    (void)arg;
    if (base == WIFI_EVENT)
    {
        switch (id)
        {
        case WIFI_EVENT_STA_START:
            if (s_sta_enabled)
            {
                esp_wifi_connect();
            }
            break;

        case WIFI_EVENT_STA_DISCONNECTED:
            if (s_sta_enabled)
            {
                if (s_sta_retry < CONFIG_RMOUSE_WIFI_MAX_RETRY)
                {
                    s_sta_retry++;
                    ESP_LOGW(TAG, "STA disconnected; reconnect (%d/%d)",
                             s_sta_retry, CONFIG_RMOUSE_WIFI_MAX_RETRY);
                }
                else if (s_sta_retry == CONFIG_RMOUSE_WIFI_MAX_RETRY)
                {
                    s_sta_retry++; // 以降は警告を出さず背景で再接続継続
                    ESP_LOGW(TAG, "STA retry limit reached; keep retrying in background (AP stays up)");
                }
                esp_wifi_connect();
            }
            break;

        case WIFI_EVENT_AP_STACONNECTED:
        {
            auto *e = static_cast<wifi_event_ap_staconnected_t *>(data);
            ESP_LOGI(TAG, "AP client joined: " MACSTR, MAC2STR(e->mac));
            break;
        }

        case WIFI_EVENT_AP_STADISCONNECTED:
        {
            auto *e = static_cast<wifi_event_ap_stadisconnected_t *>(data);
            ESP_LOGI(TAG, "AP client left: " MACSTR, MAC2STR(e->mac));
            break;
        }

        default:
            break;
        }
    }
    else if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP)
    {
        auto *e = static_cast<ip_event_got_ip_t *>(data);
        s_sta_retry = 0;
        ESP_LOGI(TAG, "STA got IP " IPSTR "  ->  http://" IPSTR ":%d",
                 IP2STR(&e->ip_info.ip), IP2STR(&e->ip_info.ip),
                 CONFIG_RMOUSE_TELEMETRY_PORT);
    }
}

void start_mdns()
{
    esp_err_t err = mdns_init();
    if (err != ESP_OK)
    {
        ESP_LOGW(TAG, "mdns_init failed: %s", esp_err_to_name(err));
        return;
    }
    mdns_hostname_set(CONFIG_RMOUSE_WIFI_HOSTNAME);
    mdns_instance_name_set("reRoMouse telemetry");
    // HTTP サービスを広告。実体のサーバは webserver(webserver_start) が同ポートで起動する。
    mdns_service_add(NULL, "_http", "_tcp", CONFIG_RMOUSE_TELEMETRY_PORT, NULL, 0);
    ESP_LOGI(TAG, "mDNS up: http://%s.local:%d",
             CONFIG_RMOUSE_WIFI_HOSTNAME, CONFIG_RMOUSE_TELEMETRY_PORT);
}

// Kconfig 文字列(コンパイル時定数)を固定長バッファへ安全にコピーし、コピー長を返す。
// strnlen(src, sizeof(dst)) は src がサイズ既知のリテラルだと bound>サイズで
// -Wstringop-overread を誘発するため使わない。strlen+クランプなら、コピー長は常に
// ソース長以下となりソース範囲外を読まないため警告にならない。
size_t copy_kstr(uint8_t *dst, size_t dst_size, const char *src)
{
    size_t n = strlen(src);
    if (n > dst_size)
    {
        n = dst_size;
    }
    memcpy(dst, src, n);
    return n;
}

void configure_ap()
{
    wifi_config_t ap = {};
    ap.ap.ssid_len = static_cast<uint8_t>(
        copy_kstr(ap.ap.ssid, sizeof(ap.ap.ssid), CONFIG_RMOUSE_WIFI_AP_SSID));
    ap.ap.channel = CONFIG_RMOUSE_WIFI_AP_CHANNEL;
    ap.ap.max_connection = CONFIG_RMOUSE_WIFI_AP_MAX_CONN;
    ap.ap.pmf_cfg.required = false;

    const size_t plen = strlen(CONFIG_RMOUSE_WIFI_AP_PASSWORD);
    if (plen == 0)
    {
        ap.ap.authmode = WIFI_AUTH_OPEN;
    }
    else if (plen < 8)
    {
        // WPA2 は 8 文字以上必須。短い場合はブロックせず OPEN にフォールバック。
        ESP_LOGW(TAG, "AP password < 8 chars; falling back to OPEN");
        ap.ap.authmode = WIFI_AUTH_OPEN;
    }
    else
    {
        copy_kstr(ap.ap.password, sizeof(ap.ap.password), CONFIG_RMOUSE_WIFI_AP_PASSWORD);
        ap.ap.authmode = WIFI_AUTH_WPA2_PSK;
    }
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &ap));
}

// STA を設定。接続対象があれば true。
bool configure_sta()
{
    if (strlen(CONFIG_RMOUSE_WIFI_STA_SSID) == 0)
    {
        ESP_LOGW(TAG, "STA SSID empty; not connecting (AP remains available)");
        return false;
    }
    wifi_config_t sta = {};
    copy_kstr(sta.sta.ssid, sizeof(sta.sta.ssid), CONFIG_RMOUSE_WIFI_STA_SSID);
    copy_kstr(sta.sta.password, sizeof(sta.sta.password), CONFIG_RMOUSE_WIFI_STA_PASSWORD);
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &sta));
    return true;
}

} // namespace

void wifi_manager_start(void)
{
    // --- ネット基盤の初期化（NVS / netif / default event loop） ---
    // NVS は WiFi PHY キャリブレーション等の保存に必須。esp_wifi_init より前に行う。
    esp_err_t nret = nvs_flash_init();
    if (nret == ESP_ERR_NVS_NO_FREE_PAGES || nret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        nret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(nret);
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // --- モード決定（Kconfig） ---
#if defined(CONFIG_RMOUSE_WIFI_MODE_AP)
    const wifi_mode_t mode = WIFI_MODE_AP;
#elif defined(CONFIG_RMOUSE_WIFI_MODE_STA)
    const wifi_mode_t mode = WIFI_MODE_STA;
#else
    const wifi_mode_t mode = WIFI_MODE_APSTA;
#endif
    const bool want_ap = (mode == WIFI_MODE_AP || mode == WIFI_MODE_APSTA);
    const bool want_sta = (mode == WIFI_MODE_STA || mode == WIFI_MODE_APSTA);

    // --- netif 生成（init_net_base() で netif/event 初期化済み前提） ---
    if (want_ap)
    {
        esp_netif_create_default_wifi_ap();
    }
    esp_netif_t *sta_netif = nullptr;
    if (want_sta)
    {
        sta_netif = esp_netif_create_default_wifi_sta();
        // ホスト名は netif 生成直後に設定（DHCP 開始時に使われる）。
        esp_err_t herr = esp_netif_set_hostname(sta_netif, CONFIG_RMOUSE_WIFI_HOSTNAME);
        if (herr != ESP_OK)
        {
            ESP_LOGW(TAG, "set hostname failed: %s", esp_err_to_name(herr));
        }
    }

    // --- WiFi 初期化 ---
    wifi_init_config_t init_cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&init_cfg));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM)); // 資格情報は毎回 Kconfig から

    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        WIFI_EVENT, ESP_EVENT_ANY_ID, &on_wifi_event, nullptr, nullptr));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        IP_EVENT, IP_EVENT_STA_GOT_IP, &on_wifi_event, nullptr, nullptr));

    ESP_ERROR_CHECK(esp_wifi_set_mode(mode));

    if (want_ap)
    {
        configure_ap();
    }
    if (want_sta)
    {
        s_sta_enabled = configure_sta();
    }

    ESP_ERROR_CHECK(esp_wifi_start());

    if (s_sta_enabled)
    {
        // テレメトリ応答性を優先（消費電力増に注意。電池駆動の走行時は要検討）。
        esp_wifi_set_ps(WIFI_PS_NONE);
    }

    start_mdns();

    if (want_ap)
    {
        ESP_LOGI(TAG, "AP up: SSID '%s' ch %d  ->  http://192.168.4.1:%d",
                 CONFIG_RMOUSE_WIFI_AP_SSID, CONFIG_RMOUSE_WIFI_AP_CHANNEL,
                 CONFIG_RMOUSE_TELEMETRY_PORT);
    }
    if (s_sta_enabled)
    {
        ESP_LOGI(TAG, "STA connecting to '%s' (hostname '%s')",
                 CONFIG_RMOUSE_WIFI_STA_SSID, CONFIG_RMOUSE_WIFI_HOSTNAME);
    }
    ESP_LOGI(TAG, "wifi_manager started (mode=%d)", static_cast<int>(mode));
}

#endif // CONFIG_RMOUSE_WIFI_ENABLE
