// EMBED_FILES で埋め込んだフルサイズ信号バイナリを const float* として公開する。
//
// EMBED_FILES "data/<name>.bin" は、リンク時に `_binary_<name>_bin_start` という
// シンボルを生成する (パスは除去され basename の非英数字が '_' に置換される)。

#include "fullsize_signals.hpp"

#include <cstdint>

extern "C" {
extern const uint8_t translation_left_start[] asm("_binary_translation_signal_left_45900_bin_start");
extern const uint8_t translation_right_start[] asm("_binary_translation_signal_right_45900_bin_start");
extern const uint8_t rotation_left_start[] asm("_binary_rotation_signal_left_45900_bin_start");
extern const uint8_t rotation_right_start[] asm("_binary_rotation_signal_right_45900_bin_start");
}

namespace FullSizeSignals {

const float* const translation_signal_left_45900 =
    reinterpret_cast<const float*>(translation_left_start);
const float* const translation_signal_right_45900 =
    reinterpret_cast<const float*>(translation_right_start);
const float* const rotation_signal_left_45900 =
    reinterpret_cast<const float*>(rotation_left_start);
const float* const rotation_signal_right_45900 =
    reinterpret_cast<const float*>(rotation_right_start);

}  // namespace FullSizeSignals
