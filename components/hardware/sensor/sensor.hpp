#ifndef SENSOR_HPP
#define SENSOR_HPP

#include <iostream>
#include "sens_structs.hpp"

/*
    < センサドライバの基底クラス >
    センサからデータを取得するドライバを作成する場合は、このクラスを継承する。

    NOTE: 旧 share_sensor_data(SensorData*) は形骸インターフェース（ドライバ側で
    保持するだけで未使用）だったため除去した。SensorData への詰め替えは mouse 層が
    生値 getter を呼んで行う。hardware 層は SensorData を関知しない。
    （sens_structs.hpp の include は mouse 層への暫定的な供給連鎖維持のためのもので、
      正本の mouse_core 移設＝案B で解消予定）
*/

struct Sensor
{
};

#endif // SENSOR_HPP