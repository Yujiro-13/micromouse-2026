function mouse_log_analyzer()
    % 保存されたマイクロマウスログデータを詳細分析するMATLABスクリプト
    % リアルタイム受信とは別に、保存されたデータの詳細解析用
    
    fprintf('=== マイクロマウスログアナライザー ===\n');
    fprintf('1. リアルタイムログビューア\n');
    fprintf('2. 保存済みCSVファイル分析\n');
    fprintf('3. 保存済みMATファイル分析\n');
    choice = input('選択してください (1-3): ');
    
    switch choice
        case 1
            mouse_log_viewer(); % リアルタイムビューア呼び出し
            
        case 2
            analyze_csv_file();
            
        case 3
            analyze_mat_file();
            
        otherwise
            fprintf('無効な選択です。\n');
    end
end

function analyze_csv_file()
    % CSVファイルを選択して分析
    [filename, pathname] = uigetfile('*.csv', 'CSVログファイルを選択してください');
    
    if isequal(filename, 0)
        fprintf('ファイルが選択されませんでした。\n');
        return;
    end
    
    filepath = fullfile(pathname, filename);
    
    try
        % CSVファイル読み込み
        data_table = readtable(filepath);
        fprintf('ファイル読み込み完了: %s\n', filename);
        fprintf('データサンプル数: %d\n', height(data_table));
        
        % データを数値配列に変換
        data_buffer = table2array(data_table);
        
        % 分析とグラフ表示
        perform_detailed_analysis(data_buffer, filename);
        
    catch ME
        fprintf('ファイル読み込みエラー: %s\n', ME.message);
    end
end

function analyze_mat_file()
    % MATファイルを選択して分析
    [filename, pathname] = uigetfile('*.mat', 'MATログファイルを選択してください');
    
    if isequal(filename, 0)
        fprintf('ファイルが選択されませんでした。\n');
        return;
    end
    
    filepath = fullfile(pathname, filename);
    
    try
        % MATファイル読み込み
        loaded_data = load(filepath);
        data_buffer = loaded_data.data_buffer;
        
        fprintf('ファイル読み込み完了: %s\n', filename);
        fprintf('データサンプル数: %d\n', size(data_buffer, 1));
        
        % 分析とグラフ表示
        perform_detailed_analysis(data_buffer, filename);
        
    catch ME
        fprintf('ファイル読み込みエラー: %s\n', ME.message);
    end
end

function perform_detailed_analysis(data_buffer, filename)
    % 詳細分析とグラフ表示
    
    if size(data_buffer, 2) < 26
        fprintf('データ形式が正しくありません。\n');
        return;
    end
    
    % データの変換とラベル定義
    [processed_data, labels, units] = process_log_data(data_buffer);
    
    % 統計情報表示
    display_statistics(processed_data, labels, units);
    
    % グラフ表示
    create_analysis_plots(processed_data, labels, units, filename);
    
    % FFT解析
    perform_fft_analysis(processed_data, labels);
    
    % 制御性能分析
    analyze_control_performance(processed_data);
end

function [processed_data, labels, units] = process_log_data(data_buffer)
    % ログデータを処理して構造体に格納
    
    processed_data = struct();
    
    % データの変換（適切なスケーリング）
    % 壁センサ値（0-3列）は uint16_t（0-65535）の範囲
    % CSVから読み込んだ値は既に正の値として出力されているため、そのまま使用
    processed_data.wall_fl = data_buffer(:, 1);
    processed_data.wall_l = data_buffer(:, 2);
    processed_data.wall_r = data_buffer(:, 3);
    processed_data.wall_fr = data_buffer(:, 4);
    processed_data.battery = data_buffer(:, 5) / 1000; % V
    processed_data.vel_current = data_buffer(:, 6) / 1000; % m/s
    processed_data.vel_target = data_buffer(:, 7) / 1000; % m/s
    processed_data.sum_len = data_buffer(:, 8) / 1000; % m
    processed_data.ang_vel_current = data_buffer(:, 9) / 1000; % rad/s
    processed_data.ang_vel_target = data_buffer(:, 10) / 1000; % rad/s
    processed_data.rad_current = data_buffer(:, 11) / 1000; % rad
    processed_data.accel_target = data_buffer(:, 12) / 1000; % m/s²
    processed_data.ang_accel_target = data_buffer(:, 13) / 100; % 角加速度は100倍スケール (0.01 rad/s² to rad/s²)
    processed_data.vel_error = data_buffer(:, 14) / 1000; % m/s
    processed_data.vel_i_error = data_buffer(:, 15) / 1000; % m/s
    processed_data.vel_d_error = data_buffer(:, 16) / 1000; % m/s
    processed_data.ang_error = data_buffer(:, 17) / 1000; % rad/s
    processed_data.ang_i_error = data_buffer(:, 18) / 1000; % rad/s
    processed_data.ang_d_error = data_buffer(:, 19) / 1000; % rad/s
    processed_data.duty_l = data_buffer(:, 20) / 1000;
    processed_data.duty_r = data_buffer(:, 21) / 1000;
    processed_data.enc_l = data_buffer(:, 22);
    processed_data.enc_r = data_buffer(:, 23);
    processed_data.len_current = data_buffer(:, 24) / 1000; % m
    processed_data.len_target = data_buffer(:, 25) / 1000; % m
    processed_data.delta_time = data_buffer(:, 26); % μs
    
    if size(data_buffer, 2) >= 27
        processed_data.thinking_flag = data_buffer(:, 27);
    end
    
    % 時間軸作成（サンプリング周期1ms想定）
    processed_data.time = (0:length(processed_data.vel_current)-1) * 0.001; % 秒
    
    % ラベルと単位
    labels = fieldnames(processed_data);
    units = {'', '', '', '', 'V', 'm/s', 'm/s', 'm', 'rad/s', 'rad/s', 'rad', ...
             'm/s²', 'rad/s²', 'm/s', 'm/s', 'm/s', 'rad/s', 'rad/s', 'rad/s', ...
             '', '', '', '', 'm', 'm', 'μs', '', 's'};
end

function display_statistics(data, labels, units)
    % 統計情報を表示
    fprintf('\n=== 統計情報 ===\n');
    
    key_fields = {'vel_current', 'vel_target', 'ang_vel_current', 'ang_vel_target', ...
                  'vel_error', 'ang_error', 'duty_l', 'duty_r', 'battery'};
    
    for i = 1:length(key_fields)
        field = key_fields{i};
        if isfield(data, field)
            values = data.(field);
            idx = strcmp(labels, field);
            unit = units{idx};
            
            fprintf('%s:\n', field);
            fprintf('  平均: %.4f %s\n', mean(values), unit);
            fprintf('  標準偏差: %.4f %s\n', std(values), unit);
            fprintf('  最大: %.4f %s\n', max(values), unit);
            fprintf('  最小: %.4f %s\n', min(values), unit);
            fprintf('  RMS: %.4f %s\n', rms(values), unit);
            fprintf('\n');
        end
    end
end

function create_analysis_plots(data, labels, units, filename)
    % 詳細分析グラフを作成
    
    % メインの制御性能グラフ
    figure('Name', sprintf('制御性能分析 - %s', filename), 'Position', [50, 50, 1600, 1000]);
    
    % 速度追従性能
    subplot(2, 3, 1);
    plot(data.time, data.vel_target, 'g-', 'LineWidth', 2);
    hold on;
    plot(data.time, data.vel_current, 'r-', 'LineWidth', 1.5);
    title('速度追従性能');
    xlabel('時間 [s]');
    ylabel('速度 [m/s]');
    legend({'目標', '実際'}, 'Location', 'best');
    grid on;
    
    % 速度エラー
    subplot(2, 3, 2);
    plot(data.time, data.vel_error, 'r-', 'LineWidth', 1.5);
    title('速度エラー');
    xlabel('時間 [s]');
    ylabel('エラー [m/s]');
    grid on;
    
    % 角速度追従性能
    subplot(2, 3, 3);
    plot(data.time, data.ang_vel_target, 'g-', 'LineWidth', 2);
    hold on;
    plot(data.time, data.ang_vel_current, 'r-', 'LineWidth', 1.5);
    title('角速度追従性能');
    xlabel('時間 [s]');
    ylabel('角速度 [rad/s]');
    legend({'目標', '実際'}, 'Location', 'best');
    grid on;
    
    % 角速度エラー
    subplot(2, 3, 4);
    plot(data.time, data.ang_error, 'r-', 'LineWidth', 1.5);
    title('角速度エラー');
    xlabel('時間 [s]');
    ylabel('エラー [rad/s]');
    grid on;
    
    % モータ出力
    subplot(2, 3, 5);
    plot(data.time, data.duty_l, 'r-', data.time, data.duty_r, 'b-', 'LineWidth', 1.5);
    title('モータDuty比');
    xlabel('時間 [s]');
    ylabel('Duty比');
    legend({'左', '右'}, 'Location', 'best');
    grid on;
    
    % 軌道表示（概算）
    subplot(2, 3, 6);
    if isfield(data, 'sum_len') && isfield(data, 'rad_current')
        x_pos = cumsum(cos(data.rad_current) .* diff([0; data.sum_len]));
        y_pos = cumsum(sin(data.rad_current) .* diff([0; data.sum_len]));
        plot(x_pos, y_pos, 'b-', 'LineWidth', 2);
        title('推定軌道');
        xlabel('X位置 [m]');
        ylabel('Y位置 [m]');
        axis equal;
        grid on;
    end
    
    % 壁センサ・電源電圧分析グラフ
    figure('Name', sprintf('センサ・電源分析 - %s', filename), 'Position', [100, 100, 1600, 900]);
    
    subplot(2, 3, 1);
    plot(data.time, data.wall_fl, 'b-', data.time, data.wall_fr, 'r-', 'LineWidth', 1.5);
    title('前壁センサ');
    xlabel('時間 [s]');
    ylabel('センサ値');
    legend({'前左', '前右'}, 'Location', 'best');
    grid on;
    
    subplot(2, 3, 2);
    plot(data.time, data.wall_l, 'g-', data.time, data.wall_r, 'm-', 'LineWidth', 1.5);
    title('サイド壁センサ');
    xlabel('時間 [s]');
    ylabel('センサ値');
    legend({'左', '右'}, 'Location', 'best');
    grid on;
    
    subplot(2, 3, 3);
    wall_diff = data.wall_r - data.wall_l;
    plot(data.time, wall_diff, 'k-', 'LineWidth', 1.5);
    title('左右壁センサ差（壁制御用）');
    xlabel('時間 [s]');
    ylabel('センサ差');
    grid on;
    
    subplot(2, 3, 4);
    plot(data.time, data.battery, 'r-', 'LineWidth', 2);
    title('電源電圧');
    xlabel('時間 [s]');
    ylabel('電圧 [V]');
    ylim([3.0 4.5]);
    grid on;
    
    % 電圧統計情報を表示
    min_voltage = min(data.battery);
    max_voltage = max(data.battery);
    avg_voltage = mean(data.battery);
    voltage_drop = max_voltage - min_voltage;
    
    text(data.time(end)*0.02, 4.3, sprintf('最大: %.2fV', max_voltage), 'FontSize', 10);
    text(data.time(end)*0.02, 4.2, sprintf('最小: %.2fV', min_voltage), 'FontSize', 10);
    text(data.time(end)*0.02, 4.1, sprintf('平均: %.2fV', avg_voltage), 'FontSize', 10);
    text(data.time(end)*0.02, 4.0, sprintf('降下: %.2fV', voltage_drop), 'FontSize', 10);
    
    if min_voltage < 3.3
        text(data.time(end)*0.5, 3.35, '低電圧警告', ...
             'Color', 'red', 'FontSize', 12, 'FontWeight', 'bold', ...
             'HorizontalAlignment', 'center');
    end
    
    subplot(2, 3, 5);
    if isfield(data, 'delta_time')
        plot(data.time, data.delta_time, 'm-', 'LineWidth', 1.5);
        title('制御処理時間');
        xlabel('時間 [s]');
        ylabel('時間 [μs]');
        grid on;
        
        % 処理時間統計
        max_time = max(data.delta_time);
        avg_time = mean(data.delta_time);
        text(data.time(end)*0.02, max_time*0.9, sprintf('最大: %dμs', round(max_time)), 'FontSize', 10);
        text(data.time(end)*0.02, max_time*0.8, sprintf('平均: %dμs', round(avg_time)), 'FontSize', 10);
        
        if max_time > 1000
            text(data.time(end)*0.5, max_time*0.7, '処理時間オーバー!', ...
                 'Color', 'red', 'FontSize', 12, 'FontWeight', 'bold', ...
                 'HorizontalAlignment', 'center');
        end
    end
    
    subplot(2, 3, 6);
    % 電圧変化率
    if length(data.battery) > 1
        voltage_rate = diff(data.battery) ./ diff(data.time);
        plot(data.time(2:end), voltage_rate, 'b-', 'LineWidth', 1.5);
        title('電圧変化率');
        xlabel('時間 [s]');
        ylabel('変化率 [V/s]');
        grid on;
        
        % ゼロライン追加
        hold on;
        plot([data.time(1), data.time(end)], [0, 0], 'k--', 'LineWidth', 1);
        hold off;
    end
end

function perform_fft_analysis(data, labels)
    % FFT解析による周波数特性分析
    
    fprintf('\n=== FFT解析 ===\n');
    
    % サンプリング周波数（1kHz想定）
    fs = 1000; % Hz
    
    % 分析対象信号
    signals = {'vel_error', 'ang_error', 'vel_current', 'ang_vel_current'};
    
    figure('Name', 'FFT解析', 'Position', [150, 150, 1200, 800]);
    
    for i = 1:length(signals)
        signal_name = signals{i};
        if isfield(data, signal_name)
            signal = data.(signal_name);
            
            % FFT計算
            N = length(signal);
            Y = fft(signal);
            P2 = abs(Y/N);
            P1 = P2(1:N/2+1);
            P1(2:end-1) = 2*P1(2:end-1);
            f = fs*(0:(N/2))/N;
            
            subplot(2, 2, i);
            loglog(f(2:end), P1(2:end), 'LineWidth', 1.5);
            title(sprintf('%s の周波数スペクトラム', signal_name));
            xlabel('周波数 [Hz]');
            ylabel('振幅');
            grid on;
            
            % 主要周波数成分の特定
            [peaks, locs] = findpeaks(P1, 'MinPeakHeight', max(P1)*0.1);
            if ~isempty(locs)
                dominant_freq = f(locs(1));
                fprintf('%s の主要周波数成分: %.2f Hz\n', signal_name, dominant_freq);
            end
        end
    end
end

function analyze_control_performance(data)
    % 制御性能指標の計算
    
    fprintf('\n=== 制御性能指標 ===\n');
    
    % 速度制御性能
    if isfield(data, 'vel_error')
        vel_rmse = sqrt(mean(data.vel_error.^2));
        vel_iae = sum(abs(data.vel_error)) * 0.001; % 積分絶対誤差
        vel_ise = sum(data.vel_error.^2) * 0.001; % 積分二乗誤差
        
        fprintf('速度制御性能:\n');
        fprintf('  RMSE: %.6f m/s\n', vel_rmse);
        fprintf('  IAE: %.6f\n', vel_iae);
        fprintf('  ISE: %.6f\n', vel_ise);
    end
    
    % 角速度制御性能
    if isfield(data, 'ang_error')
        ang_rmse = sqrt(mean(data.ang_error.^2));
        ang_iae = sum(abs(data.ang_error)) * 0.001;
        ang_ise = sum(data.ang_error.^2) * 0.001;
        
        fprintf('角速度制御性能:\n');
        fprintf('  RMSE: %.6f rad/s\n', ang_rmse);
        fprintf('  IAE: %.6f\n', ang_iae);
        fprintf('  ISE: %.6f\n', ang_ise);
    end
    
    % モータ使用率
    if isfield(data, 'duty_l') && isfield(data, 'duty_r')
        max_duty = max([max(abs(data.duty_l)), max(abs(data.duty_r))]);
        avg_duty = mean([mean(abs(data.duty_l)), mean(abs(data.duty_r))]);
        
        fprintf('モータ使用率:\n');
        fprintf('  最大Duty比: %.3f (%.1f%%)\n', max_duty, max_duty*100);
        fprintf('  平均Duty比: %.3f (%.1f%%)\n', avg_duty, avg_duty*100);
    end
    
    % バッテリ消費
    if isfield(data, 'battery')
        battery_drop = data.battery(1) - data.battery(end);
        fprintf('バッテリ電圧降下: %.3f V\n', battery_drop);
    end
    
    % 実行時間
    if isfield(data, 'time')
        total_time = data.time(end);
        fprintf('総実行時間: %.3f 秒\n', total_time);
    end
    
    % 走行距離
    if isfield(data, 'sum_len')
        total_distance = data.sum_len(end);
        fprintf('総走行距離: %.3f m\n', total_distance);
    end
end
