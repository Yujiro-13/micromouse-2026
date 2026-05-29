clear; close all; clc;

%% パラメータ定義
fprintf('=== 対向二輪型ロボット システム同定用入力信号生成 ===\n\n');

% 並進モデル用パラメータ
n_trans = 8;                % シフトレジスタ段数（周期: 2^6-1 = 63）
clock_period_trans = 0.18;   % クロック周期 [s]
amplitude_trans = 1;       % 信号振幅 [V]

% 回転モデル用パラメータ  
n_rot = 9;                   % シフトレジスタ段数（周期: 2^7-1 = 127）
clock_period_rot = 0.09;      % クロック周期 [s]
amplitude_rot = 1;         % 信号振幅 [V]

% 共通パラメータ
sampling_period = 0.001;      % サンプリング周期 [s] (1kHz)

% パラメータ表示
fprintf('【並進モデル用パラメータ】\n');
fprintf('  シフトレジスタ段数 n: %d (周期: %d)\n', n_trans, 2^n_trans-1);
fprintf('  クロック周期: %.2f [s]\n', clock_period_trans);
fprintf('  信号振幅: %.2f [V]\n', amplitude_trans);
fprintf('  信号全長: %.2f [s]\n\n', (2^n_trans-1)*clock_period_trans);

fprintf('【回転モデル用パラメータ】\n');
fprintf('  シフトレジスタ段数 n: %d (周期: %d)\n', n_rot, 2^n_rot-1);
fprintf('  クロック周期: %.2f [s]\n', clock_period_rot);
fprintf('  信号振幅: %.2f [V]\n', amplitude_rot);
fprintf('  信号全長: %.2f [s]\n\n', (2^n_rot-1)*clock_period_rot);

fprintf('【共通パラメータ】\n');
fprintf('  サンプリング周期: %.3f [s] (%.1f Hz)\n\n', sampling_period, 1/sampling_period);

%% M系列信号の生成

% 並進用M系列信号の生成
fprintf('--- 並進用M系列信号を生成中 ---\n');
[time_trans, m_series_trans] = generate_m_series(n_trans, amplitude_trans, ...
                                                 clock_period_trans, sampling_period);

% 回転用M系列信号の生成
fprintf('\n--- 回転用M系列信号を生成中 ---\n');
[time_rot, m_series_rot] = generate_m_series(n_rot, amplitude_rot, ...
                                             clock_period_rot, sampling_period);

%% モーター指令信号の作成

% 並進用信号（左右同相）
u_left_translation = m_series_trans;   % 左モーター用信号
u_right_translation = m_series_trans;  % 右モーター用信号（左と同一）

% 回転用信号（左右逆相）
u_left_rotation = m_series_rot;        % 左モーター用信号
u_right_rotation = -m_series_rot;      % 右モーター用信号（左と逆相）

fprintf('\n--- モーター指令信号の生成完了 ---\n');

%% 結果の可視化

% Figure 1: 並進用入力信号
figure('Name', '並進用入力信号', 'Position', [100, 100, 800, 500]);
plot(time_trans, u_left_translation, 'b-', 'LineWidth', 1.5, 'DisplayName', 'u_{left}');
hold on;
plot(time_trans, u_right_translation, 'r--', 'LineWidth', 1.5, 'DisplayName', 'u_{right}');
hold off;
title('並進モデル同定用入力信号（左右同相）', 'FontSize', 14);
xlabel('時間 [s]', 'FontSize', 12);
ylabel('モーター指令電圧 [V]', 'FontSize', 12);
legend('Location', 'best', 'FontSize', 11);
grid on;
xlim([0, min(10, max(time_trans))]);  % 最初の10秒または全体を表示
ylim([-amplitude_trans*1.2, amplitude_trans*1.2]);

% 拡大図（最初の2秒）
axes('Position', [0.6, 0.2, 0.25, 0.25]);
plot(time_trans, u_left_translation, 'b-', 'LineWidth', 1.5);
hold on;
plot(time_trans, u_right_translation, 'r--', 'LineWidth', 1.5);
hold off;
xlim([0, 2]);
ylim([-amplitude_trans*1.2, amplitude_trans*1.2]);
grid on;
title('拡大図 (0-2秒)', 'FontSize', 10);

% Figure 2: 回転用入力信号
figure('Name', '回転用入力信号', 'Position', [950, 100, 800, 500]);
plot(time_rot, u_left_rotation, 'b-', 'LineWidth', 1.5, 'DisplayName', 'u_{left}');
hold on;
plot(time_rot, u_right_rotation, 'r--', 'LineWidth', 1.5, 'DisplayName', 'u_{right}');
hold off;
title('回転モデル同定用入力信号（左右逆相）', 'FontSize', 14);
xlabel('時間 [s]', 'FontSize', 12);
ylabel('モーター指令電圧 [V]', 'FontSize', 12);
legend('Location', 'best', 'FontSize', 11);
grid on;
xlim([0, min(10, max(time_rot))]);  % 最初の10秒または全体を表示
ylim([-amplitude_rot*1.2, amplitude_rot*1.2]);

% 拡大図（最初の2秒）
axes('Position', [0.6, 0.2, 0.25, 0.25]);
plot(time_rot, u_left_rotation, 'b-', 'LineWidth', 1.5);
hold on;
plot(time_rot, u_right_rotation, 'r--', 'LineWidth', 1.5);
hold off;
xlim([0, 2]);
ylim([-amplitude_rot*1.2, amplitude_rot*1.2]);
grid on;
title('拡大図 (0-2秒)', 'FontSize', 10);

%% 信号の相関性チェック（検証用）

% 並進用信号の相関チェック
correlation_trans = corrcoef(u_left_translation, u_right_translation);
fprintf('\n【信号の相関性チェック】\n');
fprintf('並進用信号（左右）の相関係数: %.4f （期待値: 1.0）\n', correlation_trans(1,2));

% 回転用信号の相関チェック
correlation_rot = corrcoef(u_left_rotation, u_right_rotation);
fprintf('回転用信号（左右）の相関係数: %.4f （期待値: -1.0）\n', correlation_rot(1,2));

%% データの保存

fprintf('\n--- テキストファイルへの保存 ---\n');

% 並進用データの保存（テキストファイル形式）
% ヘッダー行を作成
header_line = 'time,u_left,u_right';

% データ行を作成（カンマ区切り）
fid = fopen('translation_input.txt', 'w');
if fid == -1
    error('ファイル translation_input.txt を開けませんでした');
end

% ヘッダー行を書き込み
fprintf(fid, '%s\n', header_line);

% 各行のデータを書き込み
for i = 1:length(time_trans)
    fprintf(fid, '%.6f,%.6f,%.6f\n', time_trans(i), u_left_translation(i), u_right_translation(i));
end

fclose(fid);
fprintf('並進用入力信号を translation_input.txt に保存しました\n');
fprintf('  ファイルサイズ: %d サンプル\n', length(time_trans));

% 回転用データの保存（テキストファイル形式）
fid = fopen('rotation_input.txt', 'w');
if fid == -1
    error('ファイル rotation_input.txt を開けませんでした');
end

% ヘッダー行を書き込み
fprintf(fid, '%s\n', header_line);

% 各行のデータを書き込み
for i = 1:length(time_rot)
    fprintf(fid, '%.6f,%.6f,%.6f\n', time_rot(i), u_left_rotation(i), u_right_rotation(i));
end

fclose(fid);
fprintf('回転用入力信号を rotation_input.txt に保存しました\n');
fprintf('  ファイルサイズ: %d サンプル\n', length(time_rot));

% 信号値のみのファイルも作成（時間情報なし、1行形式）
fprintf('\n--- 信号値のみのテキストファイル（1行形式）も作成 ---\n');

% 並進用 - 左モーター信号
fid = fopen('translation_left_values.txt', 'w');
fprintf(fid, '%.6f', u_left_translation(1));
for i = 2:length(u_left_translation)
    fprintf(fid, ',%.6f', u_left_translation(i));
end
fclose(fid);

% 並進用 - 右モーター信号
fid = fopen('translation_right_values.txt', 'w');
fprintf(fid, '%.6f', u_right_translation(1));
for i = 2:length(u_right_translation)
    fprintf(fid, ',%.6f', u_right_translation(i));
end
fclose(fid);

% 回転用 - 左モーター信号
fid = fopen('rotation_left_values.txt', 'w');
fprintf(fid, '%.6f', u_left_rotation(1));
for i = 2:length(u_left_rotation)
    fprintf(fid, ',%.6f', u_left_rotation(i));
end
fclose(fid);

% 回転用 - 右モーター信号
fid = fopen('rotation_right_values.txt', 'w');
fprintf(fid, '%.6f', u_right_rotation(1));
for i = 2:length(u_right_rotation)
    fprintf(fid, ',%.6f', u_right_rotation(i));
end
fclose(fid);

fprintf('信号値のみのファイルも作成しました:\n');
fprintf('  - translation_left_values.txt\n');
fprintf('  - translation_right_values.txt\n');
fprintf('  - rotation_left_values.txt\n');
fprintf('  - rotation_right_values.txt\n');

%% 統計情報の表示

fprintf('\n【生成された信号の統計情報】\n');
fprintf('並進用信号:\n');
fprintf('  最大値: %.2f [V]\n', max(u_left_translation));
fprintf('  最小値: %.2f [V]\n', min(u_left_translation));
fprintf('  RMS値: %.2f [V]\n', rms(u_left_translation));
fprintf('  信号長: %.2f [s]\n', max(time_trans));

fprintf('\n回転用信号:\n');
fprintf('  最大値: %.2f [V]\n', max(u_left_rotation));
fprintf('  最小値: %.2f [V]\n', min(u_left_rotation));
fprintf('  RMS値: %.2f [V]\n', rms(u_left_rotation));
fprintf('  信号長: %.2f [s]\n', max(time_rot));

%% 周波数特性の確認

% Figure 3: 周波数特性
figure('Name', '周波数特性', 'Position', [100, 650, 1200, 400]);

% 並進用信号のパワースペクトル密度
subplot(1, 2, 1);
fs_trans = 1/sampling_period;
[pxx_trans, f_trans] = pwelch(u_left_translation, [], [], [], fs_trans);
plot(f_trans, 10*log10(pxx_trans), 'b-', 'LineWidth', 1.5);
title('並進用信号のパワースペクトル密度', 'FontSize', 12);
xlabel('周波数 [Hz]', 'FontSize', 11);
ylabel('PSD [dB/Hz]', 'FontSize', 11);
grid on;
xlim([0, fs_trans/2]);

% 回転用信号のパワースペクトル密度
subplot(1, 2, 2);
fs_rot = 1/sampling_period;
[pxx_rot, f_rot] = pwelch(u_left_rotation, [], [], [], fs_rot);
plot(f_rot, 10*log10(pxx_rot), 'r-', 'LineWidth', 1.5);
title('回転用信号のパワースペクトル密度', 'FontSize', 12);
xlabel('周波数 [Hz]', 'FontSize', 11);
ylabel('PSD [dB/Hz]', 'FontSize', 11);
grid on;
xlim([0, fs_rot/2]);

%% 自己相関関数の確認

% Figure 4: 自己相関関数
figure('Name', '自己相関関数', 'Position', [100, 200, 1200, 400]);

% 並進用信号の自己相関関数
subplot(1, 2, 1);
[acf_trans, lags_trans] = xcorr(u_left_translation, 'normalized');
% 時間軸に変換
lag_time_trans = lags_trans * sampling_period;
% 中心部分のみ表示（±5秒）
idx_trans = abs(lag_time_trans) <= 5;
plot(lag_time_trans(idx_trans), acf_trans(idx_trans), 'b-', 'LineWidth', 1.5);
title('並進用信号の自己相関関数', 'FontSize', 12);
xlabel('遅延時間 [s]', 'FontSize', 11);
ylabel('正規化自己相関', 'FontSize', 11);
grid on;
ylim([-0.5, 1.1]);
% ゼロ遅延での値を表示
hold on;
plot(0, acf_trans(lags_trans==0), 'ro', 'MarkerSize', 8, 'LineWidth', 2);
text(0.2, 0.9, sprintf('R(0) = %.3f', acf_trans(lags_trans==0)), 'FontSize', 10);
hold off;

% 回転用信号の自己相関関数
subplot(1, 2, 2);
[acf_rot, lags_rot] = xcorr(u_left_rotation, 'normalized');
% 時間軸に変換
lag_time_rot = lags_rot * sampling_period;
% 中心部分のみ表示（±5秒）
idx_rot = abs(lag_time_rot) <= 5;
plot(lag_time_rot(idx_rot), acf_rot(idx_rot), 'r-', 'LineWidth', 1.5);
title('回転用信号の自己相関関数', 'FontSize', 12);
xlabel('遅延時間 [s]', 'FontSize', 11);
ylabel('正規化自己相関', 'FontSize', 11);
grid on;
ylim([-0.5, 1.1]);
% ゼロ遅延での値を表示
hold on;
plot(0, acf_rot(lags_rot==0), 'ro', 'MarkerSize', 8, 'LineWidth', 2);
text(0.2, 0.9, sprintf('R(0) = %.3f', acf_rot(lags_rot==0)), 'FontSize', 10);
hold off;

fprintf('\n=== 処理完了 ===\n');
fprintf('生成されたファイル:\n');
fprintf('【メインデータファイル】\n');
fprintf('  1. translation_input.txt - 並進モデル同定用入力信号（時間, 左, 右）\n');
fprintf('  2. rotation_input.txt - 回転モデル同定用入力信号（時間, 左, 右）\n');
fprintf('【信号値のみのファイル（1行形式）】\n');
fprintf('  3. translation_left_values.txt - 並進用左モーター信号値\n');
fprintf('  4. translation_right_values.txt - 並進用右モーター信号値\n');
fprintf('  5. rotation_left_values.txt - 回転用左モーター信号値\n');
fprintf('  6. rotation_right_values.txt - 回転用右モーター信号値\n');
fprintf('\nこれらのテキストファイルをロボット実機にアップロードして使用してください。\n');