#include "http_server.hpp"

#if CONFIG_RMOUSE_WIFI_ENABLE

#include <cstdlib>
#include <cstring>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include "sdkconfig.h"
#include "telemetry.hpp"

namespace
{
constexpr const char *TAG = "httpd";

// httpd ハンドル。多重起動防止のため保持する。
httpd_handle_t s_server = nullptr;

#if CONFIG_HTTPD_WS_SUPPORT
// テレメトリ配信タスク(core0)。多重起動防止のため保持する。
TaskHandle_t s_telem_task = nullptr;
// 同時クライアント上限。httpd の max_open_sockets(既定 7)を十分カバーする固定長。
constexpr size_t kMaxClients = 8;
#endif

// EMBED した gzip 済みアセット(CMake: target_add_binary_data)。
// シンボル名は埋め込み元ファイル名由来('.'→'_')。
extern const uint8_t index_html_gz_start[] asm("_binary_index_html_gz_start");
extern const uint8_t index_html_gz_end[] asm("_binary_index_html_gz_end");
extern const uint8_t app_js_gz_start[] asm("_binary_app_js_gz_start");
extern const uint8_t app_js_gz_end[] asm("_binary_app_js_gz_end");

// gzip 済みアセットを Content-Encoding: gzip 付きで配信する共通処理。
esp_err_t send_gzip(httpd_req_t *req, const char *content_type,
                    const uint8_t *start, const uint8_t *end)
{
    const size_t gz_len = static_cast<size_t>(end - start);
    httpd_resp_set_type(req, content_type);
    httpd_resp_set_hdr(req, "Content-Encoding", "gzip");
    return httpd_resp_send(req, reinterpret_cast<const char *>(start), gz_len);
}

// GET / : ダッシュボード本体(静的, gzip)。
esp_err_t root_get_handler(httpd_req_t *req)
{
    return send_gzip(req, "text/html", index_html_gz_start, index_html_gz_end);
}

// GET /app.js : ダッシュボードのスクリプト(静的, gzip)。
esp_err_t app_js_get_handler(httpd_req_t *req)
{
    return send_gzip(req, "application/javascript", app_js_gz_start, app_js_gz_end);
}

// GET /api/info : デバイス静的情報(チップ/MAC/IP/FW/ヒープ等)を JSON で返す(1 回取得用)。
esp_err_t info_get_handler(httpd_req_t *req)
{
    char *json = telemetry_info_json();
    if (json == nullptr)
    {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "info json build failed");
        return ESP_FAIL;
    }
    httpd_resp_set_type(req, "application/json");
    esp_err_t r = httpd_resp_sendstr(req, json);
    free(json);
    return r;
}

#if CONFIG_HTTPD_WS_SUPPORT
// WS /ws : サーバ push(20Hz) のテレメトリ・ストリーム。
// ハンドシェイク後の最初の呼び出し(HTTP_GET)では接続確立のみ。以降はクライアントからの
// 制御メッセージ(rate/pause)を受信して telemetry へ委譲する。配信は telemetry_task が行う。
esp_err_t ws_handler(httpd_req_t *req)
{
    if (req->method == HTTP_GET)
    {
        ESP_LOGI(TAG, "WS client connected (fd=%d)", httpd_req_to_sockfd(req));
        return ESP_OK;
    }

    // フレーム長を取得(payload=null, max_len=0)。
    httpd_ws_frame_t frame = {};
    frame.type = HTTPD_WS_TYPE_TEXT;
    esp_err_t ret = httpd_ws_recv_frame(req, &frame, 0);
    if (ret != ESP_OK)
    {
        return ret;
    }
    if (frame.len == 0 || frame.len > 256)
    {
        return ESP_OK; // 空/過大フレームは無視
    }

    uint8_t buf[257] = {};
    frame.payload = buf;
    ret = httpd_ws_recv_frame(req, &frame, sizeof(buf) - 1);
    if (ret != ESP_OK)
    {
        return ret;
    }
    if (frame.type == HTTPD_WS_TYPE_TEXT)
    {
        telemetry_handle_cmd(reinterpret_cast<const char *>(buf), static_cast<int>(frame.len));
    }
    return ESP_OK;
}

// 接続中の全 WS クライアントへ 1 フレームを配信する。
void broadcast_frame(void)
{
    size_t num = kMaxClients;
    int client_fds[kMaxClients];
    if (httpd_get_client_list(s_server, &num, client_fds) != ESP_OK)
    {
        return;
    }

    char *json = nullptr; // 送る相手がいる時だけ生成する(無駄な JSON 化を避ける)
    size_t len = 0;
    for (size_t i = 0; i < num; i++)
    {
        int fd = client_fds[i];
        if (httpd_ws_get_fd_info(s_server, fd) != HTTPD_WS_CLIENT_WEBSOCKET)
        {
            continue;
        }
        if (json == nullptr)
        {
            json = telemetry_frame_json();
            if (json == nullptr)
            {
                return;
            }
            len = strlen(json);
        }
        httpd_ws_frame_t frame = {};
        frame.final = true; // 単一フレーム送信(FIN=1)。未設定だとブラウザが分割扱いにする。
        frame.type = HTTPD_WS_TYPE_TEXT;
        frame.payload = reinterpret_cast<uint8_t *>(json);
        frame.len = len;
        httpd_ws_send_frame_async(s_server, fd, &frame);
    }
    if (json != nullptr)
    {
        free(json);
    }
}

// テレメトリ配信タスク。core0 固定・低優先度で、制御の core1(1ms) を侵さない。
void telemetry_task(void *arg)
{
    (void)arg;
    while (true)
    {
        int hz = telemetry_rate_hz();
        TickType_t delay = pdMS_TO_TICKS(1000 / (hz < 1 ? 1 : hz));
        if (delay == 0)
        {
            delay = 1;
        }
        if (!telemetry_paused() && s_server != nullptr)
        {
            broadcast_frame();
        }
        vTaskDelay(delay);
    }
}
#endif // CONFIG_HTTPD_WS_SUPPORT

} // namespace

void webserver_start(void)
{
    if (s_server != nullptr)
    {
        return; // 多重起動防止
    }

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = CONFIG_RMOUSE_TELEMETRY_PORT;
    // HTTP サーバタスクを core0(PRO_CPU) に固定し、制御の core1(1ms)を侵さない。
    config.core_id = 0;
    // 応答が滞ったクライアントのソケットを自動回収し、接続枯渇を防ぐ。
    config.lru_purge_enable = true;

    esp_err_t err = httpd_start(&s_server, &config);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "httpd_start failed: %s", esp_err_to_name(err));
        s_server = nullptr;
        return;
    }

    httpd_uri_t root = {};
    root.uri = "/";
    root.method = HTTP_GET;
    root.handler = root_get_handler;
    root.user_ctx = nullptr;
    httpd_register_uri_handler(s_server, &root);

    httpd_uri_t app_js = {};
    app_js.uri = "/app.js";
    app_js.method = HTTP_GET;
    app_js.handler = app_js_get_handler;
    app_js.user_ctx = nullptr;
    httpd_register_uri_handler(s_server, &app_js);

    httpd_uri_t info = {};
    info.uri = "/api/info";
    info.method = HTTP_GET;
    info.handler = info_get_handler;
    info.user_ctx = nullptr;
    httpd_register_uri_handler(s_server, &info);

#if CONFIG_HTTPD_WS_SUPPORT
    httpd_uri_t ws = {};
    ws.uri = "/ws";
    ws.method = HTTP_GET;
    ws.handler = ws_handler;
    ws.user_ctx = nullptr;
    ws.is_websocket = true;
    httpd_register_uri_handler(s_server, &ws);

    // 配信タスクを core0(PRO_CPU) に固定・低優先度で起動(制御 core1 を不可侵に保つ)。
    if (s_telem_task == nullptr)
    {
        xTaskCreatePinnedToCore(telemetry_task, "telem", 6144, nullptr,
                                tskIDLE_PRIORITY + 2, &s_telem_task, 0);
    }
    ESP_LOGI(TAG, "HTTP server up on port %d (core0); GET / , /app.js , /api/info, WS /ws @ %d Hz",
             CONFIG_RMOUSE_TELEMETRY_PORT, telemetry_rate_hz());
#else
    ESP_LOGW(TAG, "HTTP server up on port %d (core0); GET / , /api/info "
                  "(WS DISABLED: enable CONFIG_HTTPD_WS_SUPPORT for /ws telemetry)",
             CONFIG_RMOUSE_TELEMETRY_PORT);
#endif
}

#endif // CONFIG_RMOUSE_WIFI_ENABLE
