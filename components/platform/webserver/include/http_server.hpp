#ifndef HTTP_SERVER_HPP
#define HTTP_SERVER_HPP

#include "sdkconfig.h"

#if CONFIG_RMOUSE_WIFI_ENABLE

#ifdef __cplusplus
extern "C" {
#endif

// HTTP サーバ(esp_http_server)を起動し、GET / で最小ダッシュボード
// (web/index.html を gzip 圧縮して EMBED したもの)を配信する。
//
// 前提: 呼び出し前に wifi_manager_start() で netif/WiFi(AP/STA) が起動済みで
//       あること。本サーバは全インタフェースにバインドするため、AP・STA の
//       どちらの経路からも到達できる。
//
// 設計方針:
//   - HTTP タスクは core0(PRO_CPU) に固定し、制御の core1(1ms ループ)を侵さない。
//   - 多重起動はガードする。起動失敗時もブロックせずログして継続する。
void webserver_start(void);

#ifdef __cplusplus
}
#endif

#endif // CONFIG_RMOUSE_WIFI_ENABLE
#endif // HTTP_SERVER_HPP
