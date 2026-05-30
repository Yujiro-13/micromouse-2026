# FreeRTOS タスク本体（割り込み制御 / 壁センサADC / ログ）と関連リソースの初期化。
# 関数シグネチャは main 側の呼び出しと同一（実装場所のみ main から本コンポーネントへ移設）。
idf_component_register(SRCS "task.cpp"
                       INCLUDE_DIRS "include"
                       REQUIRES motion mouse_core sensor driver esp_timer esp_rom freertos)
