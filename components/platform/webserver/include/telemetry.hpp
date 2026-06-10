#ifndef TELEMETRY_HPP
#define TELEMETRY_HPP

#include "sdkconfig.h"

#if CONFIG_RMOUSE_WIFI_ENABLE

#include "telemetry_bind.hpp"  // telemetry_bind 宣言(mouse_core)+ 共有構造体型
#include "telemetry_types.hpp" // HwStats など通信専用型

#ifdef __cplusplus
extern "C"
{
#endif

    // /api/info 用の静的デバイス情報 JSON 文字列を生成して返す。
    // 戻り値は malloc 確保(cJSON_PrintUnformatted)。呼び出し側が free() する。
    // 失敗時は nullptr。
    char *telemetry_info_json(void);

#ifdef __cplusplus
}
#endif

#endif // CONFIG_RMOUSE_WIFI_ENABLE
#endif // TELEMETRY_HPP
