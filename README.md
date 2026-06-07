| Supported Targets | ESP32-S3 |
| ----------------- | -------- |

# reRoMouse

ESP32-S3 を用いたマイクロマウス（迷路自律走行ロボット）のファームウェア。ESP-IDF (v5.x / CMake) と C++ で実装し、足立法による迷路探索と PID 制御による走行を行う。

> マイクロマウスは、自律型ロボットが未知の迷路を探索し最短経路を走行する競技。
> 競技の詳細は [全日本マイクロマウス大会（NTF）](https://www.ntf.or.jp/?page_id=25) を参照。

## ディレクトリ構成

ハードウェア依存度に応じて `components/` を **hardware / platform / mouse** の3層に分類している。`main/` はエントリポイントのみを持ち、アプリ層は `components/mouse/` 側に集約する。

```
micromouse-2026/
├── CMakeLists.txt          # プロジェクトルート CMake
├── sdkconfig.defaults      # 共有ビルド設定（Git管理）
├── partitions.csv          # カスタムパーティションテーブル
├── main/                   # エントリポイント
│   └── main.cpp            #   app_main：ペリフェラル初期化（init_hardware）とタスク起動
├── components/
│   ├── hardware/           # ハードウェアドライバ（ESP32ペリフェラル依存）
│   │   ├── board_config/   #   全ピン・バス定義の集約
│   │   ├── ads7066/        #   16bit ADC（壁センサ読み取り）
│   │   ├── mpu6500/        #   6軸IMU
│   │   ├── ma730/          #   磁気エンコーダ（L/R）
│   │   ├── motor/          #   DCモータ（PWM＋方向）
│   │   ├── buzzer/ neopixel/ pca9632/  # ブザー・RGB LED・I2C LED
│   │   └── sensor/         #   センサ共通インターフェース
│   ├── platform/           # ESP-IDF プラットフォーム層
│   │   ├── storage/        #   Flash (FAT) へのパラメータ・ログ永続化
│   │   └── tasks/          #   FreeRTOS タスクの薄いラッパ
│   └── mouse/              # マウス本体ロジック（プラットフォーム非依存寄り）
│       ├── mouse_core/     #   共通データ構造（structs.hpp）・定数
│       ├── signals/        #   システム同定用信号配列（EMBED_FILES）
│       ├── motion/         #   走行制御（Motion）・割り込み制御（Interrupt）
│       ├── adachi/         #   足立法による迷路探索
│       ├── wall_sensor/    #   壁センサのサンプリング（WallSensorSampler）
│       ├── ui/             #   モードUI（探索 / 最短 / テスト / ログ）
│       └── app/            #   オーケストレーション（run_micromouse）
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

## ピン設定（menuconfig）

ボードの GPIO / SPI / I2C 配線は `components/hardware/board_config/` に集約している。
参照元は menuconfig のマスタースイッチで切り替えられる。

- **OFF（既定）**: `board_config.h` にソース定義した値を使う（現行基板はこのまま）
- **ON**: menuconfig で設定した値（`CONFIG_BOARD_*`）を使う（基板移植・派生機向け）

```bash
idf.py menuconfig
# reRoMouse Board Hardware (GPIO / SPI / I2C)
#   └ [*] Use menuconfig values for board pins   ← ON で配下のピン項目を編集可能
```

将来の WiFi 設定（SSID/パスワード等）やデバッグフラグは `reRoMouse Application` メニューに定義済み。
詳細は [`components/hardware/board_config/README.md`](components/hardware/board_config/README.md) を参照。

## 主な機能

- **迷路探索**: 足立法による最短経路探索と動的な地図構築（探索 / 全面探索モード）
- **走行制御**: PID による速度・位置制御、スラロームターン、壁制御
- **センサ**: IR 壁センサ（前/前左/前右/左/右）、IMU、エンコーダによる自己位置推定
- **永続化**: 走行パラメータ・地図・ログを Flash (FAT) に保存
- **UI**: 探索 / 最短走行 / テスト / ログのモード切り替え

## 開発環境

VSCode + ESP-IDF 拡張を推奨。`.vscode/` にビルド・書き込み・モニタのタスクを定義済み。

## 参考

- [全日本マイクロマウス大会（NTF）](https://www.ntf.or.jp/?page_id=25) — マイクロマウス競技の公式情報
