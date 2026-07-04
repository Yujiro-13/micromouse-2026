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
│   │   ├── net/            #   WiFi(APSTA)・mDNS・再接続（wifi_manager, WiFi有効時のみ）
│   │   ├── webserver/      #   HTTP + WebSocket テレメトリ配信・web/ ダッシュボード資産
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

デバッグフラグ等も `reRoMouse Application` メニューに定義済み。
詳細は [`components/hardware/board_config/README.md`](components/hardware/board_config/README.md) を参照。

## WiFi テレメトリ・ダッシュボード（任意機能）

ブラウザでセンサ・自己位置・各タスクのループ時間などをリアルタイム表示する機能。
ESP32-S3 が HTTP + WebSocket でダッシュボード（HTML/JS）を配信するため、PC/スマホに**追加ソフト不要**。

> **競技ビルドでは無効（既定 `n`）のまま**にすること。無効時は WiFi 関連のコード・
> コンポーネント・タスクを一切ビルド/起動せず、**従来挙動・サイズと完全に同一**（OFF=無いのと同じ）。
> 調整・デバッグ時のみ有効化する。

### 有効化（menuconfig）

```bash
idf.py menuconfig
# reRoMouse Application → WiFi / Telemetry
#   [*] Enable WiFi telemetry feature (master switch)   ← これを ON
# さらに WebSocket 配信には ESP-IDF 側の設定も必要:
# Component config → HTTP Server → [*] WebSocket server support
```

| 設定 | 既定値 | 備考 |
| --- | --- | --- |
| マスタースイッチ `RMOUSE_WIFI_ENABLE` | `n` | 競技は n のまま |
| WiFi モード | `APSTA`（AP と STA 同時） | AP のみ / STA のみも選択可 |
| AP SSID | `reromouse-ap` | パスワード空=オープン |
| STA SSID / PW | （空） | 自宅ルータ等に接続する場合に設定。**秘匿値は git 管理外の `sdkconfig` へ** |
| Hostname | `reromouse` | mDNS で `reromouse.local` |
| サーバポート | `80` | HTTP/WebSocket 共用 |
| 配信レート | `20` Hz | 画面のスライダで 1〜50Hz に変更可 |

### 使い方

```bash
idf.py build && idf.py -p [PORT] flash monitor
```

1. 起動ログで配信先を確認:
   `AP up ... http://192.168.4.1:80` / `STA got IP <addr>` / `mDNS up: http://reromouse.local`。
2. 同じ AP（SSID `reromouse-ap`）に接続したスマホ/PC のブラウザで **`http://192.168.4.1/`**、
   または同一 LAN から **`http://reromouse.local/`** を開く。
3. 画面上部で接続状態（LIVE/OFF）・電圧・fps・**配信レート（スライダ）**・**一時停止**を操作できる。
   数値の行をクリックすると時系列グラフにピン留め、`pose` セクションは 2D で自己位置を表示。

### 無効化（従来ビルドへ戻す）

```bash
idf.py menuconfig   # RMOUSE_WIFI_ENABLE を n に戻す
idf.py fullclean && idf.py build
```

## 主な機能

- **迷路探索**: 足立法による最短経路探索と動的な地図構築（探索 / 全面探索モード）
- **走行制御**: PID による速度・位置制御、スラロームターン、壁制御
- **センサ**: IR 壁センサ（前/前左/前右/左/右）、IMU、エンコーダによる自己位置推定
- **永続化**: 走行パラメータ・地図・ログを Flash (FAT) に保存
- **UI**: 探索 / 最短走行 / テスト / ログのモード切り替え
- **WiFi テレメトリ**（任意・既定 OFF）: ブラウザでセンサ/自己位置/タスク負荷をリアルタイム表示（HTTP + WebSocket）

## 開発環境

VSCode + ESP-IDF 拡張を推奨。`.vscode/` にビルド・書き込み・モニタのタスクを定義済み。

## 参考

- [全日本マイクロマウス大会（NTF）](https://www.ntf.or.jp/?page_id=25) — マイクロマウス競技の公式情報
