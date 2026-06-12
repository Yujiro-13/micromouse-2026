#ifndef TELEMETRY_TYPES_HPP
#define TELEMETRY_TYPES_HPP

#include "sdkconfig.h"

#if CONFIG_RMOUSE_WIFI_ENABLE

#include <stdint.h>

// テレメトリ専用のデータ型。制御用データの正本(structs.hpp)とは責務を分け、
// 通信層が必要とする「計測値・スナップショット」だけをここに定義する。
//
// Phase 6 の実装ではループ計測を telemetry.cpp 内の軽量配列で保持し(s_loop_us/s_period_us、
// 制御タスクが telemetry_report_loop で更新)、stack/core/prio は RTOS から低レートで取得して
// hw.tasks[] へ直接シリアライズする。下記の HwStats/HwTaskStat は当初設計の参照型として残す。

// 1 タスク分のループ計測。各制御タスクが自身の実行時間/周期を書き込み、telemetry が読む。
struct HwTaskStat
{
    const char *name = nullptr; // タスク名(リテラル/静的文字列を想定)
    int core = -1;              // 実行コア(0/1)
    int prio = 0;               // 優先度
    uint32_t loop_us = 0;       // 直近 1 ループの実行時間 [us]
    uint32_t period_us = 0;     // 実周期 [us]
};

// ハードウェア統計のスナップショット(Phase 4 で充填)。
struct HwStats
{
    uint32_t heap_free = 0; // 空きヒープ [byte]
    uint32_t heap_min = 0;  // 起動来の最小空きヒープ [byte]
    HwTaskStat tasks[4];    // interrupt / adc / log / (予備)
    int task_count = 0;
};

#endif // CONFIG_RMOUSE_WIFI_ENABLE
#endif // TELEMETRY_TYPES_HPP
