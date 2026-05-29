# マイクロマウスログ解析システム

ESP32マイクロマウスの`log_print`関数から出力されるログデータをリアルタイムで受信・グラフ表示するMATLABスクリプト集です。

## ファイル構成

- `start_mouse_logger.m` - メインエントリーポイント（推奨起動方法）
- `mouse_log_viewer.m` - リアルタイムログビューア
- `mouse_log_analyzer.m` - 詳細分析ツール
- `README_MATLAB_Logger.md` - このファイル

## 機能概要

### 1. リアルタイムログビューア (`mouse_log_viewer.m`)
- シリアルポート経由でログデータをリアルタイム受信
- 9つのサブプロットで各種パラメータを同時表示
- データの自動保存機能（MAT・CSV形式）

**表示グラフ：**
- 壁センサ値（前左、左、右、前右）
- 速度制御（現在速度 vs 目標速度）
- 角速度制御（現在角速度 vs 目標角速度）
- 走行距離（累積・現在・目標）
- 角度変化
- 加速度（並進・角加速度）
- 制御エラー（速度のP・I・D成分）
- モータDuty比（左右）
- エンコーダ値（左右）

### 2. 詳細分析ツール (`mouse_log_analyzer.m`)
- 保存済みCSV/MATファイルの解析
- 統計情報の計算・表示
- FFT解析による周波数特性分析
- 制御性能指標の算出

**分析機能：**
- 基本統計（平均、標準偏差、最大/最小、RMS）
- 制御性能指標（RMSE、IAE、ISE）
- 周波数スペクトラム解析
- 推定軌道表示
- モータ使用率分析

### 3. 簡易ビューア
- 軽量版のリアルタイム表示
- CPU負荷を抑えた4画面表示
- 基本パラメータのみ表示

## 使用方法

### 起動方法（推奨）
```matlab
>> start_mouse_logger
```

### 個別起動
```matlab
% リアルタイムビューア
>> mouse_log_viewer

% 詳細分析ツール
>> mouse_log_analyzer
```

### 手順

1. **ESP32側の準備**
   - マイクロマウスをPCにUSB接続
   - `log_print()`関数が呼ばれるようにプログラムを実行

2. **MATLAB側の操作**
   - `start_mouse_logger`を実行
   - シリアルポート名を入力（例：COM3、/dev/ttyUSB0）
   - リアルタイムグラフでデータを監視

3. **データ保存**
   - Ctrl+Cで受信停止
   - 自動的に保存オプションが表示される
   - MAT形式とCSV形式で保存可能

4. **詳細分析**
   - 保存したファイルを`mouse_log_analyzer`で分析
   - 制御性能の定量評価が可能

## データ形式

ログデータは以下の27カラムのCSV形式で出力されます：

| カラム | 項目 | 単位 | 説明 |
|--------|------|------|------|
| 1 | wall_fl | - | 前左壁センサ値 |
| 2 | wall_l | - | 左壁センサ値 |
| 3 | wall_r | - | 右壁センサ値 |
| 4 | wall_fr | - | 前右壁センサ値 |
| 5 | battery | mV | バッテリ電圧 |
| 6 | vel_current | mm/s | 現在速度 |
| 7 | vel_target | mm/s | 目標速度 |
| 8 | sum_len | mm | 累積走行距離 |
| 9 | ang_vel_current | mrad/s | 現在角速度 |
| 10 | ang_vel_target | mrad/s | 目標角速度 |
| 11 | rad_current | mrad | 現在角度 |
| 12 | accel_target | mm/s² | 目標加速度 |
| 13 | ang_accel_target | mrad/s² | 目標角加速度 |
| 14 | vel_error | mm/s | 速度エラー |
| 15 | vel_i_error | mm/s | 速度積分エラー |
| 16 | vel_d_error | mm/s | 速度微分エラー |
| 17 | ang_error | mrad/s | 角速度エラー |
| 18 | ang_i_error | mrad/s | 角速度積分エラー |
| 19 | ang_d_error | mrad/s | 角速度微分エラー |
| 20 | duty_l | ×1000 | 左モータDuty比 |
| 21 | duty_r | ×1000 | 右モータDuty比 |
| 22 | enc_l | - | 左エンコーダ値 |
| 23 | enc_r | - | 右エンコーダ値 |
| 24 | len_current | mm | 現在区間距離 |
| 25 | len_target | mm | 目標区間距離 |
| 26 | delta_time | μs | 処理時間 |
| 27 | thinking_flag | - | 思考フラグ |

## 必要な環境

- MATLAB R2020b以降（serialport関数使用）
- Instrument Control Toolbox（シリアル通信用）
- Signal Processing Toolbox（FFT解析用、オプション）

## トラブルシューティング

### シリアルポート接続エラー
```
シリアルポート接続エラー: Port COM3 is not available
```
- デバイスマネージャーでポート名を確認
- 他のアプリケーションがポートを使用していないか確認
- USBケーブルの接続を確認

### データ形式エラー
```
データ形式が正しくありません
```
- ESP32側のlog_print()関数の出力形式を確認
- ボーレートが115200に設定されているか確認

### MATLAB関数エラー
```
Undefined function 'serialport'
```
- MATLAB R2020b以降を使用
- Instrument Control Toolboxがインストールされているか確認

## カスタマイズ

### グラフ表示の変更
`update_plots`関数内のプロット設定を変更することで、表示するパラメータや色・線種をカスタマイズできます。

### データ保存形式の追加
`save_log_data`関数を編集して、独自のデータ保存形式を追加できます。

### サンプリング周波数の変更
ESP32側の制御周期を変更した場合は、`processed_data.time`の計算部分を修正してください。

## ライセンス

このソフトウェアはMITライセンスの下で公開されています。
