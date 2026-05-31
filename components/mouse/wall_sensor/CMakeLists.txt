# 壁センサ(ADS7066)サンプリングの実装。充放電タイミング制御・EMAフィルタを内包し
# SensorData へ書き込む。タスク本体は tasks コンポーネントの薄いラッパーから呼ばれる。
idf_component_register(SRCS "wall_sensor.cpp"
                       INCLUDE_DIRS "include"
                       REQUIRES mouse_core sensor driver esp_timer esp_rom freertos)
