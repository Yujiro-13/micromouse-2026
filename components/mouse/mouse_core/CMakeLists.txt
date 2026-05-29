# 共有データ構造・基底インターフェース・ドライバ集約ヘッダ（ヘッダのみ）。
# 各アプリ層コンポーネント（storage/motion/adachi/ui）と main が依存する基盤。
idf_component_register(INCLUDE_DIRS "include"
                       REQUIRES NeoPixel MPU6500 PCA9632 Buzzer Motor MA730 ADS7066 esp_timer)
