function [time_array, signal_array] = generate_m_series(n, amplitude, clock_period, sampling_period)
% GENERATE_M_SERIES M系列（最大長系列）信号を生成する関数
%
% 入力:
%   n               - シフトレジスタの段数（整数）
%   amplitude       - 信号の振幅（信号は±amplitudeの2値を取る）
%   clock_period    - M系列の各ビットを保持する時間 [秒]
%   sampling_period - 出力信号のサンプリング周期 [秒]
%
% 出力:
%   time_array      - 時間配列（列ベクトル）
%   signal_array    - M系列信号値の配列（列ベクトル）
%
% 詳細:
%   この関数は、System Identification Toolboxのidinput関数を使用して
%   PRBS（擬似ランダム二値信号）を生成します。
%   生成される信号の周期は(2^n - 1) * clock_periodとなります。

% 入力パラメータの検証
if ~isscalar(n) || n <= 0 || floor(n) ~= n
    error('nは正の整数である必要があります');
end

if ~isscalar(amplitude) || amplitude <= 0
    error('amplitudeは正のスカラ値である必要があります');
end

if ~isscalar(clock_period) || clock_period <= 0
    error('clock_periodは正のスカラ値である必要があります');
end

if ~isscalar(sampling_period) || sampling_period <= 0
    error('sampling_periodは正のスカラ値である必要があります');
end

if sampling_period > clock_period
    warning('サンプリング周期がクロック周期より大きいため、信号の忠実度が低下する可能性があります');
end

% M系列の周期を計算
m_sequence_period = 2^n - 1;

% 信号の全長（時間）を計算（1周期分）
total_time = m_sequence_period * clock_period;

% サンプル数を計算
num_samples = ceil(total_time / sampling_period);

% idinput関数のパラメータ設定
% Band: 周波数帯域を[0, 1/clock_period]に設定
% これにより、ビットレートが1/clock_periodになる
band = [0, 1/clock_period];

% Range: 信号の振幅範囲を設定
range = [-amplitude, amplitude];

% idinput関数を使用してPRBS信号を生成
% 'prbs': 擬似ランダム二値信号（M系列）
% Band引数で周波数特性を制御
% Range引数で振幅を制御
try
    % サンプリング周波数
    fs = 1 / sampling_period;
    
    % idinputを使用してPRBS信号を生成
    % 第1引数: サンプル数
    % 第2引数: 信号タイプ ('prbs')
    % 第3引数: Band（正規化周波数で指定: [0, fmax/(fs/2)]）
    % 第4引数: Levels（Range引数として振幅範囲を指定）
    normalized_band = band * sampling_period * 2;  % 正規化周波数に変換
    
    % PRBSのレジスタ長を指定してM系列を生成
    signal_array = idinput(num_samples, 'prbs', normalized_band, range);
    
catch ME
    error('idinput関数の実行に失敗しました。System Identification Toolboxがインストールされているか確認してください。\nエラー: %s', ME.message);
end

% 時間配列を生成（列ベクトル）
time_array = (0:num_samples-1)' * sampling_period;

% 信号配列が列ベクトルであることを保証
signal_array = signal_array(:);

% 時間配列と信号配列の長さを揃える（念のため）
min_length = min(length(time_array), length(signal_array));
time_array = time_array(1:min_length);
signal_array = signal_array(1:min_length);

% 生成された信号の情報を表示
fprintf('M系列信号生成完了:\n');
fprintf('  シフトレジスタ段数: %d\n', n);
fprintf('  系列周期: %d\n', m_sequence_period);
fprintf('  クロック周期: %.3f [s]\n', clock_period);
fprintf('  サンプリング周期: %.3f [s]\n', sampling_period);
fprintf('  信号振幅: ±%.2f\n', amplitude);
fprintf('  総時間長: %.2f [s]\n', total_time);
fprintf('  サンプル数: %d\n', length(signal_array));

end