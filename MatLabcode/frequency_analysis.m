%% ログデータの周波数解析ツール
% mouse_log_viewerで記録したemergency_save*.matファイルを読み込み、
% 各信号の周波数特性を解析してノイズフィルタ設計に必要な情報を取得する

clear; close all; clc;

%% パラメータ設定
% ログファイルの選択
[filename, filepath] = uigetfile('emergency_save*.mat', 'ログファイルを選択してください');
if isequal(filename, 0)
    disp('ファイルが選択されませんでした');
    return;
end

% ファイル読み込み
fullpath = fullfile(filepath, filename);
fprintf('読み込み中: %s\n', filename);
data = load(fullpath);

% サンプリング周波数の設定（実際の制御周期に合わせて調整）
fs = 1000; % Hz（デフォルト値、必要に応じて変更）
fprintf('サンプリング周波数: %d Hz\n', fs);

%% データフィールドの取得
fields = fieldnames(data);
fprintf('\n利用可能なデータフィールド:\n');
for i = 1:length(fields)
    fprintf('%d: %s\n', i, fields{i});
end

%% データバッファの解析とログ変数名へのマッピング
% mouse_log_viewerで保存される変数名を使用
if isfield(data, 'data_buffer')
    data_buffer = data.data_buffer;
else
    error('data_buffer が見つかりません。emergency_save*.mat ファイルを選択してください。');
end

% ログ変数名の定義（mouse_log_viewer.m の save_csv_file と同じ）
log_var_names = {'wall_fl', 'wall_l', 'wall_r', 'wall_fr', 'battery_mV', ...
                 'vel_current_mm_s', 'vel_target_mm_s', 'sum_len_mm', ...
                 'ang_vel_current_mrad_s', 'ang_vel_target_mrad_s', 'rad_current_mrad', ...
                 'accel_target_mm_s2', 'ang_accel_target_mrad_s2', ...
                 'vel_error_mm_s', 'vel_i_error_mm_s', 'vel_d_error_mm_s', ...
                 'ang_error_mrad_s', 'ang_i_error_mrad_s', 'ang_d_error_mrad_s', ...
                 'duty_l_1000', 'duty_r_1000', 'enc_l', 'enc_r', ...
                 'len_current_mm', 'len_target_mm', 'delta_time'};

% データバッファから各信号を抽出
num_cols_data = size(data_buffer, 2);
num_signals = min(length(log_var_names), num_cols_data);

signals_to_analyze = {};
signal_names = {};

for i = 1:num_signals
    signals_to_analyze{end+1} = data_buffer(:, i);
    signal_names{end+1} = log_var_names{i};
end

fprintf('\n解析対象信号: %d個\n', length(signals_to_analyze));

%% FFT解析結果を格納する構造体
fft_results = struct();

for i = 1:num_signals
    signal = signals_to_analyze{i};
    sig_name = signal_names{i};

    % FFT結果を保存
    fft_results.(matlab.lang.makeValidName(sig_name)) = perform_fft_analysis(signal(:), fs);
end

%% 時間波形と振幅スペクトルの表示（サブプロット）
num_plot_cols = 3;
num_plot_rows = ceil(num_signals / num_plot_cols);

figure('Name', '周波数解析 - 振幅スペクトル', 'Position', [100, 100, 1600, 1200]);
for i = 1:num_signals
    subplot(num_plot_rows, num_plot_cols, i);
    signal = signals_to_analyze{i};

    % FFT計算
    N = length(signal);
    Y = fft(signal);
    P2 = abs(Y/N);
    P1 = P2(1:floor(N/2)+1);
    P1(2:end-1) = 2*P1(2:end-1);
    f = fs*(0:floor(N/2))/N;

    % 振幅スペクトル
    plot(f, P1, 'LineWidth', 1.5);
    grid on;
    xlabel('周波数 [Hz]');
    ylabel('振幅');
    title(signal_names{i}, 'Interpreter', 'none');
    xlim([0, min(fs/2, 500)]); % 最大500Hzまで表示
end

%% パワースペクトル密度の表示（サブプロット）
figure('Name', '周波数解析 - パワースペクトル密度', 'Position', [100, 100, 1600, 1200]);
for i = 1:num_signals
    subplot(num_plot_rows, num_plot_cols, i);
    signal = signals_to_analyze{i};
    [pxx, f] = pwelch(signal(:), [], [], [], fs);

    plot(f, 10*log10(pxx), 'LineWidth', 1.5);
    grid on;
    xlabel('周波数 [Hz]');
    ylabel('PSD [dB/Hz]');
    title(signal_names{i}, 'Interpreter', 'none');
    xlim([0, fs/2]);
end

%% ノイズ帯域の検出と推奨カットオフ周波数
fprintf('\n=== ノイズ帯域解析結果 ===\n');
for i = 1:num_signals
    sig_name = signal_names{i};
    result = fft_results.(sig_name);

    fprintf('\n[%s]\n', sig_name);
    fprintf('  支配的な周波数: %.2f Hz (%.2f dB)\n', ...
        result.dominant_freq, result.dominant_power);
    fprintf('  推奨ローパスフィルタカットオフ: %.2f Hz\n', result.suggested_cutoff);
    fprintf('  有効帯域幅（-3dB）: %.2f Hz\n', result.bandwidth_3db);
end

%% フィルタ設計の提案
fprintf('\n=== フィルタ設計の提案 ===\n');
fprintf('以下のパラメータでバターワースフィルタを設計することを推奨します:\n\n');

for i = 1:num_signals
    sig_name = signal_names{i};
    result = fft_results.(sig_name);

    fc = result.suggested_cutoff;
    fprintf('%s:\n', sig_name);
    fprintf('  [b, a] = butter(4, %.2f/(fs/2), ''low'');\n', fc);
    fprintf('  filtered_%s = filtfilt(b, a, %s);\n\n', sig_name, sig_name);
end

%% 結果の保存
[~, name, ~] = fileparts(filename);
save_name = sprintf('frequency_analysis_%s.mat', name);
save(save_name, 'fft_results', 'signal_names', 'fs');
fprintf('解析結果を保存: %s\n', save_name);

%% ローカル関数定義

function result = perform_fft_analysis(signal, fs)
    % FFT解析と特徴量抽出

    N = length(signal);
    Y = fft(signal);
    P2 = abs(Y/N);
    P1 = P2(1:floor(N/2)+1);
    P1(2:end-1) = 2*P1(2:end-1);
    f = fs*(0:floor(N/2))/N;

    % 支配的な周波数の検出
    [max_power, max_idx] = max(P1(2:end)); % DC成分を除外
    dominant_freq = f(max_idx + 1);

    % パワースペクトル密度
    [pxx, f_psd] = pwelch(signal, [], [], [], fs);
    pxx_db = 10*log10(pxx);

    % -3dB帯域幅の計算
    max_pxx_db = max(pxx_db);
    bw_threshold = max_pxx_db - 3;
    bw_indices = find(pxx_db >= bw_threshold);
    if ~isempty(bw_indices)
        bandwidth_3db = f_psd(bw_indices(end)) - f_psd(bw_indices(1));
    else
        bandwidth_3db = 0;
    end

    % 推奨カットオフ周波数（95%エネルギー）
    cumulative_power = cumsum(pxx) / sum(pxx);
    cutoff_idx = find(cumulative_power >= 0.95, 1);
    if ~isempty(cutoff_idx)
        suggested_cutoff = f_psd(cutoff_idx);
    else
        suggested_cutoff = fs / 4; % デフォルト値
    end

    % 結果を構造体に格納
    result.dominant_freq = dominant_freq;
    result.dominant_power = 20*log10(max_power);
    result.suggested_cutoff = suggested_cutoff;
    result.bandwidth_3db = bandwidth_3db;
    result.frequency = f;
    result.amplitude = P1;
    result.psd_freq = f_psd;
    result.psd_power = pxx_db;
end
