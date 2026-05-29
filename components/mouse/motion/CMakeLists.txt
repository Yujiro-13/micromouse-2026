# 走行制御（PID・壁制御・速度推定）と割り込み駆動のリアルタイム制御。
idf_component_register(SRCS "motion.cpp" "interrupt.cpp"
                       INCLUDE_DIRS "include"
                       REQUIRES mouse_core storage esp_timer esp_partition spi_flash freertos)
