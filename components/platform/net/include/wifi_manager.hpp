#ifndef WIFI_MANAGER_HPP
#define WIFI_MANAGER_HPP

#include "sdkconfig.h"

#if CONFIG_RMOUSE_WIFI_ENABLE

#ifdef __cplusplus
extern "C" {
#endif

// WiFi(AP / STA / APSTA, Kconfig で選択)と mDNS を起動する。
//
// 前提: 呼び出し前に nvs_flash_init / esp_netif_init /
//       esp_event_loop_create_default が完了していること
//       (main の init_net_base() が担当)。
//
// 設計方針:
//   - WiFi ドライバ/lwIP は core0 に固定(sdkconfig.defaults)し、
//     制御の core1(1ms ループ)を侵さない。
//   - 復旧優先。STA 切断時は AP を維持したまま背景で再接続を続ける。
//   - 設定不備(空SSID/短いAPパス)はブロックせずログして継続する。
void wifi_manager_start(void);

#ifdef __cplusplus
}
#endif

#endif // CONFIG_RMOUSE_WIFI_ENABLE
#endif // WIFI_MANAGER_HPP
