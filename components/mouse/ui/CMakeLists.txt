# 動作モードの UI（探索 / 最短走行 / テスト / ログ）。
idf_component_register(SRCS "search.cpp" "fast.cpp" "test.cpp" "log.cpp"
                       INCLUDE_DIRS "include"
                       REQUIRES motion adachi signals storage
                                fatfs esp_partition spi_flash heap esp_timer freertos)
