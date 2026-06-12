#ifndef TELEMETRY_BIND_HPP
#define TELEMETRY_BIND_HPP

#include "sdkconfig.h"

#if CONFIG_RMOUSE_WIFI_ENABLE

// 制御層(run_micromouse)のローカル構造体(val/control/map)とグローバル sens を、
// 通信層(platform/webserver)へ「読み取り専用ポインタ」で公開するための宣言。
//
// 配置理由: 呼び出し側 micromouse.cpp(mouse 層)から常に見える neutral な場所へ宣言だけ置き、
//           実体は telemetry.cpp(webserver, WiFi 有効時のみビルド)に置く。これにより
//           mouse → webserver の CMake 依存(REQUIRES)を作らずに済む。最終リンクは IDF が
//           全コンポーネントを 1 グループにまとめるため、シンボルは解決される。
// OFF 時:   本ヘッダ全体が空になり、micromouse.cpp 側の呼び出しも #if で除去されるため、
//           OFF ビルドにはこの宣言も呼び出しも一切現れない(従来同一)。
#include <cstdint> // structs.hpp は uint*_t を使うが自前で include しないため先に取り込む
#include "structs.hpp"

#ifdef __cplusplus
extern "C"
{
#endif

    // run_micromouse() の初期化直後に 1 回呼ぶ。以降テレメトリ層は read のみ行う。
    void telemetry_bind(const SensorData *sens,
                        const MotionValues *val,
                        const Control *control,
                        const MazeMap *map);

    // 各制御タスクのループ計測 ID。telemetry 側の tasks[] 添字と対応する。
    enum
    {
        TELEM_TASK_INTERRUPT = 0, // 1ms 制御ループ(core1)
        TELEM_TASK_ADC = 1,       // 壁センササンプリング(core1)
        TELEM_TASK_LOG = 2,       // ログ書き込み(core1, イベント駆動)
        TELEM_TASK_COUNT = 3,
    };

    // 各制御タスクが自身のループ末尾で 1 回呼び、直近の実行時間/周期を公開する。
    // 書き込みは uint32 のワード単位ストアのみ(ロックフリー)。telemetry_task が低レートで読む。
    // OFF 時は呼び出しごと #if 除去され、計測コード自体が存在しない(従来同一)。
    void telemetry_report_loop(int task_id, uint32_t loop_us, uint32_t period_us);

#ifdef __cplusplus
}
#endif

#endif // CONFIG_RMOUSE_WIFI_ENABLE
#endif // TELEMETRY_BIND_HPP
