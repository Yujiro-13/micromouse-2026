# 足立法による迷路探索アルゴリズム（Motion を継承）。
idf_component_register(SRCS "Adachi.cpp"
                       INCLUDE_DIRS "include"
                       REQUIRES motion freertos esp_timer)
