# board_config — ボードのピン・バス定義

基板固有のハードウェア配線（GPIO / SPI / I2C のピン割り当て、I2C アドレス・クロック）を
**1 か所に集約** するコンポーネント。各ドライバや `main.cpp` はここで定義した
`board::` 名前空間の定数のみを参照し、ピン番号を直書きしない。

```
board_config/
├── include/board_config.h   # board:: 定数（ソース定義値 と CONFIG_* 値の両持ち）
├── Kconfig.projbuild        # menuconfig 項目（ピン値・採用元スイッチ）
└── README.md                # このファイル
```

## ピン設定の参照元（2 系統 + 切り替えスイッチ）

各定数は **2 つの値**を持ち、menuconfig のマスタースイッチでどちらを使うか切り替える。

| スイッチ | 採用される値 |
|---|---|
| **OFF（既定）** | `board_config.h` にソース定義した値（`BOARD_PICK` の第2引数） |
| **ON** | menuconfig で設定した値（`CONFIG_BOARD_*`） |

`board_config.h` 内の実装イメージ:

```cpp
// 第1引数 = menuconfig値, 第2引数 = board_config.h ソース定義値
constexpr gpio_num_t kBuzzer =
    static_cast<gpio_num_t>(BOARD_PICK(CONFIG_BOARD_BUZZER_GPIO, 15));
```

> menuconfig 既定値（`default`）とソース定義値（fallback）は**いずれも現行基板と同一**。
> したがってスイッチを切り替えても、設定を変更しない限り配線挙動は変わらない。

## 使い方

### A. 現行基板のまま使う（既定）

何もしなくてよい。`board_config.h` のソース定義値が使われる。
ピンを恒久的に変えたい場合は `board_config.h` の `BOARD_PICK(..., <ここ>)` を編集する。

### B. menuconfig からピンを設定する（基板移植・派生機向け）

```bash
idf.py menuconfig
```

1. `reRoMouse Board Hardware (GPIO / SPI / I2C)` を開く
2. `[*] Use menuconfig values for board pins` を **ON** にする
   （ON にすると配下の GPIO/SPI/I2C 項目が編集可能になる）
3. 各ピン番号・SPI ホスト・I2C アドレス/クロックを設定して保存

`board_config.h` を編集せずにピンを差し替えられる。チーム内で共有したい既定は
`sdkconfig.defaults` に `CONFIG_BOARD_*` を追記する。

## 設定項目一覧（menuconfig）

`reRoMouse Board Hardware (GPIO / SPI / I2C)` メニュー配下。

| 区分 | CONFIG シンボル | 既定値 |
|---|---|---|
| 採用元スイッチ | `BOARD_PINS_FROM_MENUCONFIG` | n（=ソース定義値） |
| 起動時出力 GPIO | `BOARD_STARTUP_OUTPUT_GPIO0..3` | 10 / 17 / 18 / 21 |
| IMU/ADC SPI ホスト | `BOARD_IMU_ADC_SPI_HOST_*` | SPI2_HOST |
| IMU/ADC SPI | `BOARD_IMU_ADC_SPI_{MISO,MOSI,SCLK}_GPIO` | 2 / 4 / 3 |
| ADS7066 CS / MPU6500 CS | `BOARD_ADC_CS_GPIO` / `BOARD_IMU_CS_GPIO` | 5 / 1 |
| Encoder SPI ホスト | `BOARD_ENCODER_SPI_HOST_*` | SPI3_HOST |
| Encoder SPI | `BOARD_ENCODER_SPI_{MOSI,MISO,SCLK}_GPIO` | 9 / 8 / 7 |
| Encoder CS (L/R) | `BOARD_ENCODER_{LEFT,RIGHT}_CS_GPIO` | 6 / 14 |
| LED I2C | `BOARD_LED_I2C_{PORT,SDA_GPIO,SCL_GPIO}` | 0 / 38 / 39 |
| LED I2C アドレス / クロック | `BOARD_LED_I2C_{ADDR,CLOCK_HZ}` | 0x62 / 1000000 |
| ブザー / NeoPixel | `BOARD_BUZZER_GPIO` / `BOARD_NEOPIXEL_GPIO` | 15 / 13 |
| モータ Phase/Enable (R) | `BOARD_MOTOR_{PHASE,ENABLE}_RIGHT_GPIO` | 41 / 42 |
| モータ Phase/Enable (L) | `BOARD_MOTOR_{PHASE,ENABLE}_LEFT_GPIO` | 45 / 46 |
| 吸引ファン / モータ MODE | `BOARD_MOTOR_FAN_GPIO` / `BOARD_MOTOR_MODE_GPIO` | 11 / 40 |

### 補足

- 起動時出力 GPIO は `-1` を設定すると「未使用スロット」として無視される
  （`main.cpp` の初期化ループでスキップ）。
- SPI ホストは列挙型（`spi_host_device_t`）のため int 化できず、menuconfig では
  `SPI2_HOST` / `SPI3_HOST` の choice として扱う。
- WiFi / デバッグ等のアプリ層設定は `main/Kconfig.projbuild`（メニュー
  `reRoMouse Application`）側で定義している。
