# ESP-IDF プロジェクト構成規約

本ドキュメントは、ESP-IDF v5.x (CMake ビルドシステム) を用いたプロジェクトにおける  
構成ファイルの仕様と記述規約をまとめたものです。  
新規プロジェクトの立ち上げ時の仕様書として活用してください。

---

## 目次

1. [プロジェクト全体のディレクトリ構成](#1-プロジェクト全体のディレクトリ構成)
2. [sdkconfig.defaults — ビルド設定の共有](#2-sdkconfigdefaults--ビルド設定の共有)
3. [Kconfig.projbuild — ユーザ設定のメニュー化](#3-kconfigprojbuild--ユーザ設定のメニュー化)
4. [idf_component.yml — 外部コンポーネントの管理](#4-idf_componentyml--外部コンポーネントの管理)
5. [手動コンポーネントの追加規則](#5-手動コンポーネントの追加規則)
6. [CMakeLists.txt の仕様と記述方法](#6-cmakeliststxt-の仕様と記述方法)
7. [FreeRTOS タスクの設定規則](#7-freertos-タスクの設定規則)

---

## 1. プロジェクト全体のディレクトリ構成

```
<project-root>/
├── CMakeLists.txt              # プロジェクトルート CMakeLists（定型文）
├── sdkconfig.defaults          # ビルド設定のデフォルト値（Git管理対象）
├── sdkconfig                   # idf.py が生成する実際の設定（Git管理対象外）
├── partitions.csv              # カスタムパーティションテーブル（必要な場合）
├── dependencies.lock           # コンポーネントマネージャのロックファイル（Git管理対象）
│
├── main/
│   ├── CMakeLists.txt          # main コンポーネントのビルド定義
│   ├── main.c                  # エントリポイント（app_main のみ）
│   ├── idf_component.yml       # main が依存する外部コンポーネント宣言
│   └── Kconfig.projbuild       # プロジェクト全体の menuconfig 設定項目
│
├── components/                 # 手動作成のカスタムコンポーネント群
│   ├── board_config/           # ボードGPIO設定（ハードウェア固有）
│   ├── camera/                 # カメラドライバ＋キャプチャタスク
│   ├── wifi_manager/           # Wi-Fi 初期化＋再接続
│   ├── web_server/             # HTTP サーバ＋MJPEG ストリーム
│   └── dl_inference/           # 推論パイプライン（DLモデル統括）
│
└── managed_components/         # コンポーネントマネージャが自動生成（Git管理対象外）
```

**規則:**

- `main/main.c` は各コンポーネントの `init()` 関数を呼ぶだけにとどめ、  
  ロジックを一切持たせない。
- ハードウェア固有の設定（GPIO番号等）はすべて `board_config` コンポーネントに集約する。
- `sdkconfig` および `managed_components/` は `.gitignore` に追加し、Git 管理対象外とする。

---

## 2. sdkconfig.defaults — ビルド設定の共有

### 役割

`idf.py menuconfig` で生成される `sdkconfig` はローカル設定のため Git 管理しない。  
チーム間で共有すべきビルド設定は `sdkconfig.defaults` に記述する。  
`idf.py build` 実行時、`sdkconfig` が存在しない場合にこのファイルの値が適用される。

### ファイル位置

```
<project-root>/sdkconfig.defaults
```

### 記述例

```ini
# ===== PSRAM Basic Settings =====
CONFIG_ESP32_SPIRAM_SUPPORT=y
CONFIG_SPIRAM=y
CONFIG_SPIRAM_MODE_OCTAL=y
CONFIG_SPIRAM_SPEED_80M=y
CONFIG_SPIRAM_BOOT_INIT=y
CONFIG_SPIRAM_IGNORE_NOTFOUND=y

# ===== Memory Optimization =====
CONFIG_SPIRAM_MALLOC_ALWAYSINTERNAL=16384
CONFIG_SPIRAM_USE_MALLOC=y
CONFIG_SPIRAM_USE_CAPS_ALLOC=y

# ===== Flash Settings =====
CONFIG_ESPTOOLPY_FLASHSIZE_8MB=y
CONFIG_ESPTOOLPY_FLASHSIZE="8MB"
CONFIG_PARTITION_TABLE_CUSTOM=y
CONFIG_PARTITION_TABLE_CUSTOM_FILENAME="partitions.csv"

# ===== Performance Tuning =====
CONFIG_SPIRAM_CACHE_WORKAROUND=y
CONFIG_SPIRAM_FETCH_RESOURCE_OVER_4BYTE_LEN=y

# ===== Task Watchdog =====
# Extended to 10s: first DL inference (lazy load + inference + JPEG encode) can exceed 5s default
CONFIG_ESP_TASK_WDT_TIMEOUT_S=10
```

### 記述規則

| 規則 | 内容 |
|---|---|
| セクション分け | `# ===== Section Name =====` で区切る |
| コメント | `#` で始まる行。設定値の意図・理由を英語で記述する |
| 変更禁止設定 | ハードウェアに依存する設定（フラッシュサイズ、PSRAM モード等）は必ず記述する |
| 変更可能設定 | パフォーマンス調整値は `menuconfig` 側でも変更可能な旨をコメントで示す |

### カスタムパーティションテーブルを使用する場合

DL モデルなど大容量データを含む場合は `partitions.csv` を作成し、  
`sdkconfig.defaults` に以下を追記する。

```ini
CONFIG_PARTITION_TABLE_CUSTOM=y
CONFIG_PARTITION_TABLE_CUSTOM_FILENAME="partitions.csv"
```

`partitions.csv` の例（8MB フラッシュ、DL モデル搭載想定）:

```csv
# Name,     Type,  SubType,  Offset,   Size
nvs,        data,  nvs,      0x9000,   0x6000
phy_init,   data,  phy,      0xF000,   0x1000
factory,    app,   factory,  0x10000,  0x380000
```

> **注意:** factory パーティションは `0x10000` (64KB アライン) から開始すること。  
> DL モデルのウェイトがバイナリに埋め込まれる場合、サイズを 3.5MB 以上確保する。

---

## 3. Kconfig.projbuild — ユーザ設定のメニュー化

### 役割

`idf.py menuconfig` に独自の設定項目を追加するための Kconfig ファイル。  
Wi-Fi 認証情報・モデル選択など、ユーザが環境ごとに変更する設定を定義する。  
値は `#if CONFIG_xxx` / `CONFIG_xxx` としてソースから参照できる。

### ファイル位置

| 配置先 | 適用範囲 |
|---|---|
| `main/Kconfig.projbuild` | プロジェクト全体の設定（Wi-Fi 等） |
| `components/<name>/Kconfig.projbuild` | コンポーネント固有の設定（モデル選択等） |

### 記述例 — Wi-Fi 設定（`main/Kconfig.projbuild`）

```kconfig
menu "Wi-Fi Configuration"

    menu "STA (Station) Settings"

        config ESP_WIFI_SSID
            string "STA WiFi SSID"
            default "myssid"

        config ESP_WIFI_PASSWORD
            string "STA WiFi Password"
            default "mypassword"

    endmenu

    menu "AP (Access Point) Settings"

        config ESP_WIFI_AP_SSID
            string "AP WiFi SSID"
            default "esp32-ap"

        config ESP_WIFI_AP_PASSWORD
            string "AP WiFi Password"
            default "esp32pass"

        config ESP_WIFI_AP_CHANNEL
            int "AP WiFi Channel"
            default 1
            range 1 13

        config ESP_WIFI_AP_MAX_CONN
            int "AP Max Connections"
            default 4
            range 1 10

    endmenu

endmenu
```

### 記述例 — コンパイル時モデル選択（`components/dl_inference/Kconfig.projbuild`）

```kconfig
menu "Inference Configuration"

    choice DL_INFERENCE_MODEL
        prompt "Inference model"
        default DL_INFERENCE_MODEL_NONE
        help
            Select which deep learning model to run on each camera frame.
            "None" passes frames through without any inference.

        config DL_INFERENCE_MODEL_NONE
            bool "None (passthrough)"

        config DL_INFERENCE_MODEL_FACE
            bool "Human Face Detection"
            depends on IDF_TARGET_ESP32S3

        config DL_INFERENCE_MODEL_HAND_DETECT
            bool "Hand Detection"
            depends on IDF_TARGET_ESP32S3

        config DL_INFERENCE_MODEL_HAND_GESTURE
            bool "Hand Gesture Recognition"
            depends on IDF_TARGET_ESP32S3

        config DL_INFERENCE_MODEL_COLOR_DETECT
            bool "Color Detection"
            depends on IDF_TARGET_ESP32S3

    endchoice

endmenu
```

### ソースコードからの参照方法

```c
// C ソース (.c)
#include "sdkconfig.h"  // 通常は自動インクルード

wifi_config_t ap_config = {
    .ap = {
        .ssid     = CONFIG_ESP_WIFI_AP_SSID,
        .password = CONFIG_ESP_WIFI_AP_PASSWORD,
        .channel  = CONFIG_ESP_WIFI_AP_CHANNEL,
        .max_connection = CONFIG_ESP_WIFI_AP_MAX_CONN,
    }
};

// C++ ソース (.cpp): コンパイル時分岐
#if CONFIG_DL_INFERENCE_MODEL_FACE
#include "human_face_detect.hpp"
#elif CONFIG_DL_INFERENCE_MODEL_HAND_DETECT
#include "hand_detect.hpp"
#endif
```

### 型と制約の一覧

| Kconfig 型 | 用途 | 制約指定 |
|---|---|---|
| `bool` | ON/OFF フラグ | なし |
| `int` | 数値設定 | `range <min> <max>` |
| `string` | 文字列設定 | なし |
| `choice` / `endchoice` | 排他選択 | `depends on` で依存条件付加可 |

---

## 4. idf_component.yml — 外部コンポーネントの管理

### 役割

ESP Component Registry (components.espressif.com) および GitHub 上のコンポーネントを  
自動でダウンロード・バージョン管理するためのマニフェストファイル。  
`idf.py build` 時にコンポーネントマネージャが `managed_components/` へ自動展開する。

### ファイル位置

- `main/idf_component.yml` — main コンポーネントが依存する外部ライブラリ
- `components/<name>/idf_component.yml` — 各コンポーネントが依存する外部ライブラリ

> **注意:** `idf_component.yml` は依存関係の **宣言** のみを行う。  
> ビルドへの組み込みは別途 `CMakeLists.txt` の `REQUIRES` で行う。

### 記述例 — `main/idf_component.yml`

```yaml
dependencies:
  espressif/esp-dl: "*"
  espressif/esp32-camera: "^2.0.16"
  idf:
    version: ">=4.1.0"
```

### 記述例 — `components/dl_inference/idf_component.yml`

```yaml
dependencies:
  espressif/human_face_detect: "*"
  espressif/hand_detect: "*"
  espressif/hand_gesture_recognition: "*"
  espressif/color_detect: "*"
  idf:
    version: ">=5.1.0"
```

### バージョン指定の書式

| 書式 | 意味 | 例 |
|---|---|---|
| `"*"` | 最新バージョン | `espressif/esp-dl: "*"` |
| `"^2.0.16"` | メジャー互換 (2.x.x の最新) | `espressif/esp32-camera: "^2.0.16"` |
| `"~1.0.0"` | マイナー互換 (1.0.x の最新) | `espressif/esp-dl: "~3.3.0"` |
| `">=5.1.0"` | 下限指定 | `idf: version: ">=5.1.0"` |
| `"==1.3.1"` | バージョン固定 | 再現性が必要な場合 |

### `dependencies.lock` について

`idf.py build` 後に自動生成されるロックファイル。  
実際に解決されたバージョンが記録されており、**Git 管理対象とする**ことで  
チーム間のバージョン再現性を保証する。

---

## 5. 手動コンポーネントの追加規則

### 5-1. コンポーネントの基本構造

`components/<component-name>/` 以下に以下のファイルを作成する。

```
components/<component-name>/
├── CMakeLists.txt          # 必須
├── include/
│   └── <component-name>.h  # 公開ヘッダ（extern "C" ガード付き）
├── <component-name>.c      # 実装（または .cpp）
└── idf_component.yml       # 外部依存がある場合のみ
```

### 5-2. ドライバコンポーネントの追加

ハードウェアを直接操作するコンポーネント（センサー、モータ等）は  
`board_config` コンポーネントに **ピン定義のみ** を置き、  
ドライバロジックは専用コンポーネントに分離する。

**手順:**

1. `components/board_config/include/` に新しいピン定義ヘッダを追加する
2. `components/board_config/include/board_config.h` から `#include` する
3. 新しいドライバコンポーネントを `components/<driver>/` に作成する
4. ドライバコンポーネントの `CMakeLists.txt` に `board_config` を REQUIRES に追加する
5. `main/CMakeLists.txt` の `REQUIRES` にドライバコンポーネント名を追加する

**例 — IMU センサーを追加する場合:**

```
components/
├── board_config/include/imu_pins.h   ← ピン定義を追加
└── imu_driver/                        ← 新規ドライバコンポーネント
    ├── CMakeLists.txt
    ├── include/imu_driver.h
    └── imu_driver.c
```

`imu_driver/CMakeLists.txt`:
```cmake
idf_component_register(
    SRCS "imu_driver.c"
    INCLUDE_DIRS "include"
    REQUIRES driver board_config freertos   # board_config を必ず含める
)
```

### 5-3. メインタスク（アルゴリズム）コンポーネントの追加

制御アルゴリズムや推論パイプラインなど、複数のドライバを統合する  
上位レイヤーのコンポーネントを追加する場合の規則。

**手順:**

1. `components/<algorithm>/` に新規コンポーネントを作成する
2. 依存するコンポーネントを `CMakeLists.txt` の `REQUIRES` に列挙する
3. タスク起動は `init()` 関数内で行い、`app_main()` から呼び出す
4. コンポーネント間のデータ受け渡しは **FreeRTOS Queue** を使用する  
   （直接関数呼び出しで他コンポーネントのデータを取り出す設計は禁止）
5. Queue のハンドルは `get_queue()` 関数でカプセル化し、外部公開する

**例 — パイプライン構成:**

```c
// main.c: init関数の呼び出し順序がパイプライン構成を表す
void app_main(void)
{
    nvs_flash_init();
    board_config_init();            // GPIO 設定（必ず最初）

    camera_manager_init();          // Step1: カメラ起動
    dl_inference_init(              // Step2: カメラキューを受け取り推論
        camera_manager_get_queue()
    );
    wifi_manager_init();            // Step3: Wi-Fi 起動
    web_server_start(               // Step4: 推論結果キューを購読
        dl_inference_get_queue()
    );
}
```

**カプセル化の規則:**

```c
// <component>.h: コンポーネントの公開インターフェース
#pragma once
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

// 初期化（タスク起動を含む）
void <component>_init(QueueHandle_t upstream_queue);

// 下流コンポーネントへのキュー取得
QueueHandle_t <component>_get_queue(void);
```

### 5-4. C++ コンポーネントと C コンポーネントの混在

C++ (.cpp) でコンポーネントを実装し、C (.c) から呼び出す場合は  
`extern "C"` で公開 API をラップする。

```cpp
// dl_inference.cpp
extern "C" void dl_inference_init(QueueHandle_t camera_queue) { ... }
extern "C" QueueHandle_t dl_inference_get_queue(void) { ... }
```

```c
// main.c / web_server.c から通常通り呼び出せる
#include "dl_inference.h"
dl_inference_init(camera_manager_get_queue());
```

---

## 6. CMakeLists.txt の仕様と記述方法

### 6-1. プロジェクトルート `CMakeLists.txt`

変更不要な定型文。プロジェクト名のみ変更する。

```cmake
cmake_minimum_required(VERSION 3.16)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(<project-name>)
```

### 6-2. コンポーネント `CMakeLists.txt`

**基本形:**

```cmake
idf_component_register(
    SRCS "foo.c" "bar.c"          # ソースファイル（スペース区切り）
    INCLUDE_DIRS "include"         # 公開ヘッダのディレクトリ
    REQUIRES <dep1> <dep2>         # このコンポーネントが依存するコンポーネント
    PRIV_REQUIRES <priv_dep>       # 内部実装のみが依存するコンポーネント
)
```

**REQUIRES と PRIV_REQUIRES の使い分け:**

| キーワード | 用途 | ヘッダが公開されるか |
|---|---|---|
| `REQUIRES` | 依存先のヘッダを自コンポーネントの公開ヘッダで `#include` している場合 | される |
| `PRIV_REQUIRES` | 依存先のヘッダを .c/.cpp 内でのみ使用する場合 | されない |

**禁止事項:**

```cmake
# NG: esp_log は ESP-IDF コアに含まれるため REQUIRES に記述してはならない
# 記述すると "Failed to resolve component 'esp_log'" ビルドエラーになる
idf_component_register(
    SRCS "foo.c"
    INCLUDE_DIRS "include"
    REQUIRES esp_log   # ← 禁止
)
```

`esp_log`、`esp_system`、`freertos` (基本型のみ使用する場合) などの  
ESP-IDF コアコンポーネントは暗黙的に利用可能なため `REQUIRES` への記述は不要。  
ただし `freertos` の高度な API (Task, Queue, Semaphore 等) を使用する場合は `REQUIRES freertos` が必要。

**各コンポーネントの REQUIRES 一覧（本リポジトリ実例）:**

| コンポーネント | REQUIRES |
|---|---|
| `board_config` | `driver` |
| `camera` | `esp32-camera board_config freertos` |
| `wifi_manager` | `esp_wifi esp_netif freertos` |
| `web_server` | `esp_http_server esp32-camera freertos dl_inference` |
| `dl_inference` | `esp-dl esp32-camera freertos` + 選択モデル |

### 6-3. `main/CMakeLists.txt`

```cmake
idf_component_register(
    SRCS "main.c"
    INCLUDE_DIRS "."
    PRIV_REQUIRES esp_psram nvs_flash   # main.c 内部でのみ使用
    REQUIRES board_config camera dl_inference wifi_manager web_server
)
```

### 6-4. Kconfig 値を使った条件分岐 REQUIRES

選択された設定に応じてリンクするコンポーネントを切り替える場合:

```cmake
# components/dl_inference/CMakeLists.txt
set(model_reqs "")
if(CONFIG_DL_INFERENCE_MODEL_FACE)
    list(APPEND model_reqs human_face_detect)
elseif(CONFIG_DL_INFERENCE_MODEL_HAND_DETECT)
    list(APPEND model_reqs hand_detect)
elseif(CONFIG_DL_INFERENCE_MODEL_HAND_GESTURE)
    list(APPEND model_reqs hand_gesture_recognition)
elseif(CONFIG_DL_INFERENCE_MODEL_COLOR_DETECT)
    list(APPEND model_reqs color_detect)
endif()

idf_component_register(
    SRCS "dl_inference.cpp"
    INCLUDE_DIRS "include"
    REQUIRES esp-dl esp32-camera freertos ${model_reqs}
)
```

> **効果:** 選択されていないモデルのウェイト（数MB）はバイナリに含まれない。

---

## 7. FreeRTOS タスクの設定規則

### 7-1. タスク生成 API の選択

| API | 用途 | コア指定 |
|---|---|---|
| `xTaskCreatePinnedToCore()` | コアを明示的に指定する場合 | 第7引数 (0 or 1) |
| `xTaskCreate()` | スケジューラに委ねる場合 | なし (tskNO_AFFINITY) |

**本プロジェクトでの使用方針:**

- **ハードウェア割り込みに紐づくタスク**（カメラキャプチャ等）→ `xTaskCreatePinnedToCore(..., 1)` で Core 1 固定
- **Wi-Fi / HTTP スタック**（ESP-IDF 内部）→ Core 0 で動作（変更不可）
- **DL 推論タスク**等の汎用タスク → `xTaskCreate()` でスケジューラに委ねる

```c
// カメラタスク: Core 1 固定（Wi-Fi スタックと競合しないよう分離）
xTaskCreatePinnedToCore(camera_task, "camera_task", 4096, NULL, 5, NULL, 1);

// 推論タスク: tskNO_AFFINITY（コア0/1をスケジューラに委ねる）
xTaskCreate(inference_task, "infer_task", 8192, NULL, 4, NULL);
```

### 7-2. タスクパラメータの規則

| パラメータ | 推奨値・規則 |
|---|---|
| タスク名 | コンポーネント名を反映した短い文字列（例: `"camera_task"`, `"infer_task"`） |
| スタックサイズ | 最小 4096 bytes。JPEG デコード/エンコードを含む場合は 8192 bytes 以上 |
| 引数 (`arg`) | 原則 `NULL`。必要な場合は静的変数で保持しタスク関数から参照する |
| 優先度 | 下表参照 |
| ハンドル | 不要な場合は `NULL` |

**タスク優先度ガイドライン:**

| 優先度 | 用途の目安 |
|---|---|
| 23 | Wi-Fi ドライバ（ESP-IDF 内部、変更不可） |
| 5 | ハードウェアキャプチャ（カメラタスク等、リアルタイム性が高い） |
| 4 | 推論・処理タスク（重いが遅延許容） |
| 1〜3 | バックグラウンド通信・ログ送信等 |

### 7-3. コンポーネント間のデータ受け渡し（Queue パターン）

FreeRTOS Queue を用いた Producer-Consumer パターンを標準とする。

```
[camera_task] ──Queue(size=1)──▶ [infer_task] ──Queue(size=1)──▶ [web_server]
  Core 1                           tskNO_AFFINITY                  Core 0
  priority 5                       priority 4                       httpd
```

**Queue サイズは常に 1 とし、最新フレームのみ保持する:**

```c
#define FRAME_QUEUE_LEN 1
s_frame_queue = xQueueCreate(FRAME_QUEUE_LEN, sizeof(camera_fb_t *));
```

**古いデータを捨てて最新データのみ Queue に入れるパターン（Producer 側）:**

```c
// 古いデータを捨てる
camera_fb_t *stale = NULL;
if (xQueueReceive(s_frame_queue, &stale, 0) == pdTRUE) {
    esp_camera_fb_return(stale);
}
// 新しいデータを積む
if (xQueueSend(s_frame_queue, &fb, 0) != pdTRUE) {
    esp_camera_fb_return(fb);  // キューが取れなかった場合も解放する
}
```

**Consumer 側のタイムアウト待ち:**

```c
// 1000ms 待ってデータが来なければ次のループへ
if (xQueueReceive(s_queue, &item, pdMS_TO_TICKS(1000)) != pdTRUE) {
    vTaskDelay(pdMS_TO_TICKS(1));
    continue;
}
```

### 7-4. Task Watchdog (WDT) への対応

WDT のデフォルトタイムアウトは 5 秒。  
IDLE タスクが 5 秒以上実行されない場合にトリガーされる。

**WDT を発生させないための規則:**

1. **重い処理（推論・JPEG エンコード）の前後に `vTaskDelay(pdMS_TO_TICKS(1))` を挿入する**

```c
// JPEG デコード完了後、推論前に IDLE に実行機会を与える
esp_camera_fb_return(fb);
fb = nullptr;
vTaskDelay(pdMS_TO_TICKS(1));  // IDLE0 に実行機会を与える

// 推論実行
auto &results = s_detector->run(img);

vTaskDelay(pdMS_TO_TICKS(1));  // JPEG エンコード前にも IDLE へ譲る

// JPEG 再エンコード
dl::image::jpeg_img_t jpeg_out = dl::image::sw_encode_jpeg(img, 80);
```

2. **ループ末尾には必ず `vTaskDelay(pdMS_TO_TICKS(1))` を置く**

```c
while (true) {
    // ... 処理 ...
    vTaskDelay(pdMS_TO_TICKS(1));  // 末尾で必ず IDLE に実行機会を与える
}
```

3. **DL モデルの初回推論（レイジーロード）は特に時間がかかる**ため、  
   `sdkconfig.defaults` で WDT タイムアウトを 10 秒に設定する:

```ini
CONFIG_ESP_TASK_WDT_TIMEOUT_S=10
```

4. **カメラ起動直後の不正フレーム**（SOI マーカー欠如）は  
   JPEG デコーダーの無限ループを引き起こすため、デコード前に検証する:

```c
// JPEG SOI マーカー (0xFF 0xD8) を検証してから sw_decode_jpeg に渡す
static bool is_valid_jpeg(const camera_fb_t *fb) {
    return fb->len >= 2 && fb->buf[0] == 0xFF && fb->buf[1] == 0xD8;
}

if (!is_valid_jpeg(fb)) {
    ESP_LOGW(TAG, "Invalid JPEG frame, skipping");
    esp_camera_fb_return(fb);
    fb = nullptr;
    vTaskDelay(pdMS_TO_TICKS(1));
    continue;
}
```

### 7-5. メモリ管理規則

| 用途 | 確保方法 | 解放方法 |
|---|---|---|
| カメラフレームバッファ | `esp_camera_fb_get()` | `esp_camera_fb_return()` |
| DL デコード済み画像 (>16KB) | `sw_decode_jpeg()` → PSRAM に自動確保 | `heap_caps_free(img.data)` |
| DL エンコード済み JPEG | `sw_encode_jpeg()` → PSRAM に自動確保 | `heap_caps_free(jpeg_out.data)` |
| 一般的な大バッファ | `heap_caps_malloc(size, MALLOC_CAP_SPIRAM)` | `heap_caps_free()` |

**フレームバッファの所有権を示す型パターン（本リポジトリの `infer_frame_t`）:**

```c
typedef struct {
    const uint8_t *buf;   // データポインタ
    size_t         len;   // データサイズ
    camera_fb_t   *fb;    // fb != NULL → esp_camera_fb_return で解放
                          // fb == NULL → heap_caps_free で解放
} infer_frame_t;
```

---

## 付録: ビルド・書き込みコマンド早見表

```bash
# ビルド
idf.py build

# 書き込み + モニター起動
idf.py -p COM8 flash monitor

# 設定変更（menuconfig）
idf.py menuconfig

# モニターのみ（書き込みなし）
idf.py -p COM8 monitor

# ビルドキャッシュのクリア（依存関係が壊れた場合）
idf.py fullclean
```

---

*本ドキュメントは ESP-IDF v5.3.5 / xiao-esp32s3-sample リポジトリを基に作成。*
