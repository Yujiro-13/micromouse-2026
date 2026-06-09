#include "http_server.hpp"

#if CONFIG_RMOUSE_WIFI_ENABLE

#include "esp_http_server.h"
#include "esp_log.h"
#include "sdkconfig.h"

namespace
{
constexpr const char *TAG = "httpd";

// httpd ハンドル。多重起動防止のため保持する。
httpd_handle_t s_server = nullptr;

// EMBED した gzip 済み index.html(CMake: target_add_binary_data)。
// シンボル名は埋め込み元ファイル名 index.html.gz 由来('.'→'_')。
extern const uint8_t index_html_gz_start[] asm("_binary_index_html_gz_start");
extern const uint8_t index_html_gz_end[] asm("_binary_index_html_gz_end");

// GET / : 最小ダッシュボード(静的, gzip)を返す。
esp_err_t root_get_handler(httpd_req_t *req)
{
    const size_t gz_len = static_cast<size_t>(index_html_gz_end - index_html_gz_start);
    httpd_resp_set_type(req, "text/html");
    httpd_resp_set_hdr(req, "Content-Encoding", "gzip");
    return httpd_resp_send(req, reinterpret_cast<const char *>(index_html_gz_start), gz_len);
}

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

    ESP_LOGI(TAG, "HTTP server up on port %d (core0); GET / -> index.html (gzip)",
             CONFIG_RMOUSE_TELEMETRY_PORT);
}

#endif // CONFIG_RMOUSE_WIFI_ENABLE
