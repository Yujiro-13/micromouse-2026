# FreeRTOS タスクのエントリ(薄いラッパー)。ESP-IDF 依存のため platform 配下に置く。
# 実装本体は対応クラス(motion の Interrupt / wall_sensor の WallSensorSampler)側にある。
idf_component_register(SRCS "task.cpp"
                       INCLUDE_DIRS "include"
                       REQUIRES motion wall_sensor)
