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

#ifdef __cplusplus
}
#endif

#endif // CONFIG_RMOUSE_WIFI_ENABLE
#endif // TELEMETRY_BIND_HPP
