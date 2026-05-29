| Supported Targets | ESP32-S3 |
| ----------------- | -------- |

# reRoMouse

ESP32-S3 を用いたマイクロマウス（迷路自律走行ロボット）のファームウェア。ESP-IDF (v5.x / CMake) と C++ で実装し、足立法による迷路探索と PID 制御による走行を行う。

## ディレクトリ構成

```
micromouse-2026/
├── CMakeLists.txt          # プロジェクトルート CMake
├── sdkconfig.defaults      # 共有ビルド設定（Git管理）
├── partitions.csv          # カスタムパーティションテーブル
├── main/                   # アプリケーション層（エントリポイント・制御・UI）
│   ├── main.cpp            #   app_main：ペリフェラル初期化とタスク起動
│   ├── Motion.cpp / Adachi.cpp / Interrupt.cpp  # 走行制御・探索・割り込み
│   └── include/            #   アプリ層ヘッダ
├── components/             # ハードウェアドライバ（コンポーネント分離）
│   ├── ADS7066/            #   16bit ADC（壁センサ読み取り）
│   ├── MPU6500/            #   6軸IMU
│   ├── MA730/              #   磁気エンコーダ（L/R）
│   ├── Motor/              #   DCモータ（PWM＋方向）
│   ├── Buzzer/ NeoPixel/ PCA9632/  # ブザー・RGB LED・I2C LED
│   └── sensor/             #   センサ共通インターフェース
└── tools/                  # 開発支援ツール（ビルド対象外）
    ├── generate_signal_arrays.py   # システム同定用信号配列の生成
    └── matlab/                     # ログ解析・システム同定用 MATLAB スクリプト
```

## ビルド・書き込み

```bash
# ターゲット設定（初回のみ）
idf.py set-target esp32s3

# ビルド
idf.py build

# 書き込み + シリアルモニタ
idf.py -p [PORT] flash monitor

# 設定変更
idf.py menuconfig

# ビルドキャッシュのクリア
idf.py fullclean
```

## 主な機能

- **迷路探索**: 足立法による最短経路探索と動的な地図構築（探索 / 全面探索モード）
- **走行制御**: PID による速度・位置制御、スラロームターン、壁制御
- **センサ**: IR 壁センサ（前/前左/前右/左/右）、IMU、エンコーダによる自己位置推定
- **永続化**: 走行パラメータ・地図・ログを Flash (FAT/SPIFFS) に保存
- **UI**: 探索 / 最短走行 / テスト / ログのモード切り替え

## 開発環境

VSCode + ESP-IDF 拡張を推奨。`.vscode/` にビルド・書き込み・モニタのタスクを定義済み。
</content>
