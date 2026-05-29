#ifndef SENSOR_HPP
#define SENSOR_HPP

#include <iostream>
#include "sens_structs.hpp"

/*
    < センサドライバのインターフェースクラス >
    センサからデータを取得するドライバを作成する場合は、このクラスを継承
*/

struct Sensor
{
    virtual void Shar_SensData(SensorData *_sens) = 0;
};

#endif // SENSOR_HPP