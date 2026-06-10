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

    // WS /ws で 20Hz 配信する 1 フレーム分の JSON(envelope: type/v/t/data)を生成して返す。
    // bind 済みポインタ(sens/val/control/map)を read してスナップショット化する(RAM のみ参照)。
    // 戻り値は malloc 確保。呼び出し側が free() する。失敗時は nullptr。
    char *telemetry_frame_json(void);

    // クライアント→サーバの WS 制御メッセージを処理する({"rate":N} / {"pause":bool}、
    // または {"type":"cmd","data":{...}} の envelope)。data は WS フレームのペイロード。
    void telemetry_handle_cmd(const char *data, int len);

    // 現在の送信レート[Hz](1..50 にクランプ)。telemetry_task の周期決定に使う。
    int telemetry_rate_hz(void);

    // 一時停止中かどうか。停止中は配信をスキップする。
    bool telemetry_paused(void);

#ifdef __cplusplus
}
#endif

#endif // CONFIG_RMOUSE_WIFI_ENABLE
#endif // TELEMETRY_HPP
