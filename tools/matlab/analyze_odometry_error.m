function analyze_odometry_error(mat_filename)
    % オドメトリ誤差の詳細分析スクリプト
    % 使用方法: analyze_odometry_error('mouse_log_20250126_123456.mat')
    % または: analyze_odometry_error('emergency_save_20250126_123456.mat')
    
    if nargin < 1
        % ファイル選択ダイアログを表示
        [file, path] = uigetfile('*.mat', 'MATファイルを選択してください');
        if isequal(file, 0)
            disp('ファイル選択がキャンセルされました。');
            return;
        end
        mat_filename = fullfile(path, file);
    end
    
    % データ読み込み
    fprintf('データを読み込んでいます: %s\n', mat_filename);
    load(mat_filename, 'data_buffer');
    
    % データサイズチェック
    if size(data_buffer, 2) < 35
        error('このデータファイルにはオドメトリ情報が含まれていません。35列必要ですが、%d列しかありません。', size(data_buffer, 2));
    end
    
    fprintf('データ読み込み完了: %d サンプル\n\n', size(data_buffer, 1));
    
    % データ抽出（位置情報）
    odom_x_est = data_buffer(:, 28) / 1000;         % 生センサ推定X [m]
    odom_y_est = data_buffer(:, 29) / 1000;         % 生センサ推定Y [m]
    odom_theta_est = data_buffer(:, 30) / 1000;     % 生センサ推定θ [rad]
    odom_x_corrected = data_buffer(:, 31) / 1000;   % セル補正後X [m]
    odom_y_corrected = data_buffer(:, 32) / 1000;   % セル補正後Y [m]
    odom_theta_corrected = data_buffer(:, 33) / 1000; % セル補正後θ [rad]
    cell_x = data_buffer(:, 34);                    % 真値セルX座標
    cell_y = data_buffer(:, 35);                    % 真値セルY座標
    
    % === セル基準位置（真値）を計算 ===
    % map->pos.x, map->pos.y からグローバル座標系の真値位置を計算
    CELL_SIZE = 0.09; % 90mm
    CELL_CENTER_OFFSET = CELL_SIZE / 2.0; % 45mm
    
    odom_x_ref = cell_x * CELL_SIZE + CELL_CENTER_OFFSET;  % 真値X [m]
    odom_y_ref = cell_y * CELL_SIZE + CELL_CENTER_OFFSET;  % 真値Y [m]
    odom_theta_ref = odom_theta_corrected;  % 姿勢角は補正後を使用
    
    % === 誤差を計算（MATLAB側で計算） ===
    % 生センサオドメトリの誤差
    odom_x_error = odom_x_est - odom_x_ref;
    odom_y_error = odom_y_est - odom_y_ref;
    odom_theta_error = odom_theta_est - odom_theta_ref;
    odom_pos_error = sqrt(odom_x_error.^2 + odom_y_error.^2);
    
    % セル補正オドメトリの誤差（理想的にはゼロに近い）
    odom_x_error_corrected = odom_x_corrected - odom_x_ref;
    odom_y_error_corrected = odom_y_corrected - odom_y_ref;
    odom_theta_error_corrected = odom_theta_corrected - odom_theta_ref;
    odom_pos_error_corrected = sqrt(odom_x_error_corrected.^2 + odom_y_error_corrected.^2);
    
    % 軌跡比較プロット
    figure('Name', 'オドメトリ誤差解析', 'Position', [100, 100, 1400, 800]);
    
    % === 2D軌跡プロット（3つのオドメトリを重ねて表示） ===
    subplot(2, 3, 1);
    plot(odom_x_ref, odom_y_ref, 'b-', 'LineWidth', 2.5);
    hold on;
    plot(odom_x_est, odom_y_est, 'r--', 'LineWidth', 1.5);
    plot(odom_x_corrected, odom_y_corrected, 'g:', 'LineWidth', 2);
    xlabel('X [m]');
    ylabel('Y [m]');
    title('軌跡比較 (2D) - 3種類のオドメトリ');
    legend({'真値（セル基準）', '生センサオドメトリ', 'セル補正オドメトリ'}, 'Location', 'best');
    grid on;
    axis equal;
    
    % 誤差ベクトル表示（間引き）- 生センサオドメトリの誤差
    step = max(1, floor(length(odom_x_ref) / 50));
    quiver(odom_x_ref(1:step:end), odom_y_ref(1:step:end), ...
           odom_x_error(1:step:end), odom_y_error(1:step:end), ...
           0, 'color', [1 0.5 0], 'LineWidth', 1);
    legend({'真値（セル基準）', '生センサオドメトリ', 'セル補正オドメトリ', '誤差ベクトル（生センサ）'}, 'Location', 'best');
    hold off;
    
    % === X誤差時系列 ===
    subplot(2, 3, 2);
    plot(odom_x_error * 1000, 'r-', 'LineWidth', 1);
    xlabel('サンプル数');
    ylabel('X誤差 [mm]');
    title(sprintf('X方向誤差 (平均: %.2f mm, 最大: %.2f mm)', ...
        mean(abs(odom_x_error)) * 1000, max(abs(odom_x_error)) * 1000));
    grid on;
    hold on;
    yline(0, 'k--', 'LineWidth', 1);
    hold off;
    
    % === Y誤差時系列 ===
    subplot(2, 3, 3);
    plot(odom_y_error * 1000, 'g-', 'LineWidth', 1);
    xlabel('サンプル数');
    ylabel('Y誤差 [mm]');
    title(sprintf('Y方向誤差 (平均: %.2f mm, 最大: %.2f mm)', ...
        mean(abs(odom_y_error)) * 1000, max(abs(odom_y_error)) * 1000));
    grid on;
    hold on;
    yline(0, 'k--', 'LineWidth', 1);
    hold off;
    
    % === 位置誤差ノルム時系列（生センサとセル補正の比較） ===
    subplot(2, 3, 4);
    plot(odom_pos_error * 1000, 'r-', 'LineWidth', 1.5);
    hold on;
    plot(odom_pos_error_corrected * 1000, 'g-', 'LineWidth', 1.5);
    xlabel('サンプル数');
    ylabel('位置誤差 [mm]');
    title(sprintf('位置誤差ノルム比較\n生センサ平均: %.2f mm, セル補正平均: %.2f mm', ...
        mean(odom_pos_error) * 1000, mean(odom_pos_error_corrected) * 1000));
    legend({'生センサオドメトリ誤差', 'セル補正オドメトリ誤差'}, 'Location', 'best');
    grid on;
    hold off;
    
    % === 誤差ヒストグラム ===
    subplot(2, 3, 5);
    histogram(odom_pos_error * 1000, 50, 'FaceColor', 'b', 'EdgeColor', 'k');
    xlabel('位置誤差 [mm]');
    ylabel('頻度');
    title('誤差分布');
    grid on;
    hold on;
    % 平均値に縦線を表示
    xline(mean(odom_pos_error) * 1000, 'r--', 'LineWidth', 2, ...
        'Label', sprintf('平均: %.2f mm', mean(odom_pos_error) * 1000));
    hold off;
    
    % === 統計情報テキスト ===
    subplot(2, 3, 6);
    axis off;
    stats_text = sprintf([...
        '=== オドメトリ誤差統計 ===\n\n' ...
        '【生センサオドメトリ】\n' ...
        'X誤差:\n' ...
        '  平均: %.3f mm\n' ...
        '  最大: %.3f mm\n' ...
        '  標準偏差: %.3f mm\n' ...
        'Y誤差:\n' ...
        '  平均: %.3f mm\n' ...
        '  最大: %.3f mm\n' ...
        '  標準偏差: %.3f mm\n' ...
        '位置誤差ノルム:\n' ...
        '  平均: %.3f mm\n' ...
        '  最大: %.3f mm\n\n' ...
        '【セル補正オドメトリ】\n' ...
        '位置誤差ノルム:\n' ...
        '  平均: %.3f mm\n' ...
        '  最大: %.3f mm\n' ...
        '  改善率: %.1f%%\n\n' ...
        '総サンプル数: %d\n'], ...
        mean(abs(odom_x_error)) * 1000, max(abs(odom_x_error)) * 1000, std(odom_x_error) * 1000, ...
        mean(abs(odom_y_error)) * 1000, max(abs(odom_y_error)) * 1000, std(odom_y_error) * 1000, ...
        mean(odom_pos_error) * 1000, max(odom_pos_error) * 1000, ...
        mean(odom_pos_error_corrected) * 1000, max(odom_pos_error_corrected) * 1000, ...
        (1 - mean(odom_pos_error_corrected) / mean(odom_pos_error)) * 100, ...
        length(odom_pos_error));
    
    text(0.05, 0.5, stats_text, 'FontSize', 9, 'FontName', 'Courier New', ...
         'VerticalAlignment', 'middle');
    
    % === コンソールに統計情報を出力 ===
    fprintf('=== オドメトリ誤差統計 ===\n\n');
    fprintf('【生センサオドメトリ】\n');
    fprintf('X誤差:\n');
    fprintf('  平均: %.3f mm\n', mean(abs(odom_x_error)) * 1000);
    fprintf('  最大: %.3f mm\n', max(abs(odom_x_error)) * 1000);
    fprintf('  標準偏差: %.3f mm\n', std(odom_x_error) * 1000);
    fprintf('Y誤差:\n');
    fprintf('  平均: %.3f mm\n', mean(abs(odom_y_error)) * 1000);
    fprintf('  最大: %.3f mm\n', max(abs(odom_y_error)) * 1000);
    fprintf('  標準偏差: %.3f mm\n', std(odom_y_error) * 1000);
    fprintf('位置誤差ノルム:\n');
    fprintf('  平均: %.3f mm\n', mean(odom_pos_error) * 1000);
    fprintf('  最大: %.3f mm\n', max(odom_pos_error) * 1000);
    fprintf('  RMS: %.3f mm\n', rms(odom_pos_error) * 1000);
    fprintf('\n');
    fprintf('【セル補正オドメトリ】\n');
    fprintf('位置誤差ノルム:\n');
    fprintf('  平均: %.3f mm\n', mean(odom_pos_error_corrected) * 1000);
    fprintf('  最大: %.3f mm\n', max(odom_pos_error_corrected) * 1000);
    fprintf('  RMS: %.3f mm\n', rms(odom_pos_error_corrected) * 1000);
    fprintf('  改善率: %.1f%%\n', (1 - mean(odom_pos_error_corrected) / mean(odom_pos_error)) * 100);
    fprintf('\n');
    fprintf('総サンプル数: %d\n', length(odom_pos_error));
    fprintf('総走行距離: %.3f m\n', sqrt((odom_x_ref(end) - odom_x_ref(1))^2 + (odom_y_ref(end) - odom_y_ref(1))^2));
    fprintf('\n解析完了\n');
    
    % === 追加の詳細プロット（別ウィンドウ） ===
    figure('Name', 'オドメトリ詳細解析', 'Position', [150, 150, 1200, 800]);
    
    % X-Y誤差の2D散布図（生センサとセル補正の比較）
    subplot(2, 2, 1);
    scatter(odom_x_error * 1000, odom_y_error * 1000, 10, 'r', 'filled', 'MarkerFaceAlpha', 0.4);
    hold on;
    scatter(odom_x_error_corrected * 1000, odom_y_error_corrected * 1000, 10, 'g', 'filled', 'MarkerFaceAlpha', 0.4);
    xlabel('X誤差 [mm]');
    ylabel('Y誤差 [mm]');
    title('X-Y誤差の分布比較');
    legend({'生センサ', 'セル補正'}, 'Location', 'best');
    grid on;
    axis equal;
    % 原点に十字を表示
    plot(0, 0, 'k+', 'MarkerSize', 20, 'LineWidth', 3);
    hold off;
    
    % 誤差の累積分布（生センサとセル補正の比較）
    subplot(2, 2, 2);
    sorted_errors = sort(odom_pos_error * 1000);
    cdf_values = (1:length(sorted_errors)) / length(sorted_errors) * 100;
    plot(sorted_errors, cdf_values, 'r-', 'LineWidth', 2);
    hold on;
    sorted_errors_corrected = sort(odom_pos_error_corrected * 1000);
    cdf_values_corrected = (1:length(sorted_errors_corrected)) / length(sorted_errors_corrected) * 100;
    plot(sorted_errors_corrected, cdf_values_corrected, 'g-', 'LineWidth', 2);
    xlabel('位置誤差 [mm]');
    ylabel('累積頻度 [%]');
    title('誤差の累積分布関数比較');
    legend({'生センサ', 'セル補正'}, 'Location', 'southeast');
    grid on;
    hold off;
    
    % 時間経過による誤差増加（生センサとセル補正の比較）
    subplot(2, 2, 3);
    time_axis = (1:length(odom_pos_error)) * 0.001; % 秒単位
    plot(time_axis, odom_pos_error * 1000, 'r-', 'LineWidth', 1);
    hold on;
    plot(time_axis, odom_pos_error_corrected * 1000, 'g-', 'LineWidth', 1);
    xlabel('時間 [s]');
    ylabel('位置誤差 [mm]');
    title('時間経過と誤差の関係');
    legend({'生センサ', 'セル補正'}, 'Location', 'best');
    grid on;
    hold off;
    
    % XY誤差の時系列比較
    subplot(2, 2, 4);
    plot(time_axis, odom_x_error * 1000, 'r-', 'LineWidth', 1);
    hold on;
    plot(time_axis, odom_y_error * 1000, 'g-', 'LineWidth', 1);
    xlabel('時間 [s]');
    ylabel('誤差 [mm]');
    title('X-Y誤差の時系列比較');
    legend({'X誤差', 'Y誤差'}, 'Location', 'best');
    grid on;
    yline(0, 'k--', 'LineWidth', 1);
    hold off;
    
    fprintf('\n全ての解析グラフが生成されました。\n');
end
