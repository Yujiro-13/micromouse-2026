# パラメータ・地図・ログの永続化（FAT + ウェアレベリング）。
idf_component_register(SRCS "files.cpp"
                       INCLUDE_DIRS "include"
                       REQUIRES mouse_core fatfs wear_levelling)
