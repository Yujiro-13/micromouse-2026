# マイクロマウス全体のオーケストレーション(合成ルート)。
# 各アプリ層(motion/adachi/ui/tasks)を束ね、パラメータ設定とモード選択ループを担う run_micromouse を提供。
idf_component_register(SRCS "micromouse.cpp"
                       INCLUDE_DIRS "include"
                       REQUIRES motion adachi ui tasks mouse_core sensor freertos)
