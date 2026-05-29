#pragma once

// システム同定用フルサイズ信号データのアクセサ。
//
// 旧 main/include/fullsize_signals.hpp (Shift-JIS, 18443 行に float 配列を直書き)
// を置き換える。実データは components/signals/data/*.bin (float32 LE) に分離し、
// CMakeLists の EMBED_FILES でファームウェアの .rodata へ埋め込む。
// 値は旧ソースから抽出しており同一 (信号は +-1.0 のみで float32 で厳密表現可能)。
//
// 利用側 (test.cpp) は従来どおり FullSizeSignals::translation_signal_left_45900 等を
// const float* として関数へ渡すだけなので、参照箇所の変更は不要。

namespace FullSizeSignals {

// 埋め込みバイナリ先頭への const float* (定義は fullsize_signals.cpp)
extern const float* const translation_signal_left_45900;
extern const float* const translation_signal_right_45900;
extern const float* const rotation_signal_left_45900;
extern const float* const rotation_signal_right_45900;

// サンプル数等の定数 (旧ヘッダと同一)
static constexpr int TRANSLATION_SAMPLES = 45900;
static constexpr int ROTATION_SAMPLES = 45900;
static constexpr int SAMPLING_PERIOD_MS = 1;  // 1ms
static constexpr float TRANSLATION_EXPERIMENT_DURATION_SEC = 45.900f;  // seconds
static constexpr float ROTATION_EXPERIMENT_DURATION_SEC = 45.900f;  // seconds

}  // namespace FullSizeSignals
