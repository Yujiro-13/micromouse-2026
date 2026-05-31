#ifndef TASK_HPP
#define TASK_HPP

// FreeRTOS タスクのエントリ関数(薄いラッパー)。
// 渡すパラメータは各タスクが処理するクラスのインスタンスへのポインタ:
//   myTaskInterrupt / myTaskLog : Interrupt*
//   myTaskAdc                   : WallSensorSampler*
void myTaskInterrupt(void *pvparam);
void myTaskAdc(void *pvparam);
void myTaskLog(void *pvparam);

#endif
