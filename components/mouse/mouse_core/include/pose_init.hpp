#ifndef POSE_INIT_HPP
#define POSE_INIT_HPP

#include "structs.hpp"

// 自己位置(オドメトリ)・姿勢角・論理セル位置を「スタート地点」へ初期化する共通処理。
//
// 従来は各走行モード(search/fast 等)の main_task 冒頭に同一の初期化が複製されており、
// かつ「走行を開始するまで一度も初期化されない」ため、待機中はオドメトリが原点のまま
// ジャイロ積分でドリフトしていた(自己位置 2D が確認できない主因)。
// 本関数で一元化し、各モード冒頭に加えて起動直後(待機状態)にも呼ぶことで、
// 走行前から自己位置が定義される。
//
// 値の意味:
//   - センサベース推定(x_pos/y_pos): ロボットの物理スタート位置(セル中心より 15mm 手前)。
//   - 補正後(x_pos_corrected/y_pos_corrected): セル(0,0)中心を真値として扱う。
//   - 姿勢: NORTH(θ=0)。
inline void reset_pose_to_start(Control *control, MotionValues *val, MazeMap *map)
{
    // 論理セル位置
    map->pos.x = 0;
    map->pos.y = 0;
    map->pos.dir = NORTH;

    // 姿勢角の積分器・走行距離
    val->current.rad = 0.0;
    val->sum.len = 0.0;

    // センサベース推定位置(ロボット実位置)
    const float ROBOT_START_X = 0.045f; // 45mm - セル(0,0)中心X
    const float ROBOT_START_Y = 0.030f; // 30mm - セル中心より 15mm 手前
    control->odom.x_pos = ROBOT_START_X;
    control->odom.y_pos = ROBOT_START_Y;
    control->odom.theta = 0.0f; // NORTH

    // 補正後オドメトリ(セル(0,0)中心)
    const float CELL_CENTER = 0.045f; // 45mm
    control->odom.x_pos_corrected = CELL_CENTER;
    control->odom.y_pos_corrected = CELL_CENTER;
    control->odom.theta_corrected = 0.0f;
}

#endif // POSE_INIT_HPP
