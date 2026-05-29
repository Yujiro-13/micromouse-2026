function mouse_log_viewer_with_odom()
    % マイクロマウスのログデータをリアルタイムでグラフ表示するMATLABスクリプト（自己位置推定対応版）
    % log_print関数から出力されるCSVデータを解析してグラフ化
    % 35列のデータに対応（オドメトリデータ+セル座標含む、誤差はMATLAB側で計算）

    % シリアルポートの設定
    port = input('シリアルポート名を入力してください (例: COM3): ', 's');
    baudrate = 115200; % ESP32の標準ボーレート

    % グローバル変数でデータとシリアルポートを保護
    global g_data_buffer g_serial_port g_sample_count;
    g_data_buffer = [];
    g_serial_port = [];
    g_sample_count = 0;

    try
        % シリアルポート接続
        s = serialport(port, baudrate);
        configureTerminator(s, "LF"); % 改行文字で区切り
        g_serial_port = s; % グローバル変数に保存

        % onCleanup関数でCtrl+C時の処理を確実に実行
        cleanup_obj = onCleanup(@() cleanup_function());

        fprintf('シリアルポート %s に接続しました。\n', port);
        fprintf('ログデータの受信を開始します...\n');
        fprintf('終了するには Ctrl+C を押してください。\n\n');

        % データ保存用変数の初期化
        data_buffer = [];
        max_samples = 50000; % 最大サンプル数
        
        % === メイングラフウィンドウの設定（4x3に変更） ===
        fig_main = figure('Name', 'マイクロマウスログビューア', ...
                          'Position', [50, 100, 1400, 1200]);
        
        % === 位置推定専用ウィンドウの設定 ===
        fig_odom = figure('Name', '自己位置推定ビューア', ...
                          'Position', [1470, 100, 800, 1000]);
        
        % === メインウィンドウ：既存の12個のサブプロット ===
        figure(fig_main);
        subplot(4, 3, 1);
        h1 = plot(0, 0, 'b-', 'LineWidth', 1.5);
        title('壁センサ値');
        xlabel('サンプル数');
        ylabel('センサ値');
        legend({'前左', '左', '右', '前右'}, 'Location', 'best');
        grid on;
        
        subplot(4, 3, 2);
        h2 = plot(0, 0, 'r-', 0, 0, 'g-', 'LineWidth', 1.5);
        title('速度制御');
        xlabel('サンプル数');
        ylabel('速度 [m/s]');
        legend({'現在速度', '目標速度'}, 'Location', 'best');
        grid on;
        
        subplot(4, 3, 3);
        h3 = plot(0, 0, 'r-', 0, 0, 'g-', 'LineWidth', 1.5);
        title('角速度制御');
        xlabel('サンプル数');
        ylabel('角速度 [rad/s]');
        legend({'現在角速度', '目標角速度'}, 'Location', 'best');
        grid on;
        
        subplot(4, 3, 4);
        h4 = plot(0, 0, 'b-', 'LineWidth', 1.5);
        title('走行距離');
        xlabel('サンプル数');
        ylabel('距離 [m]');
        legend({'累積距離', '現在距離', '目標距離'}, 'Location', 'best');
        grid on;
        
        subplot(4, 3, 5);
        h5 = plot(0, 0, 'r-', 'LineWidth', 1.5);
        title('角度');
        xlabel('サンプル数');
        ylabel('角度 [rad]');
        legend({'現在角度'}, 'Location', 'best');
        grid on;
        
        subplot(4, 3, 6);
        h6 = plot(0, 0, 'r-', 0, 0, 'g-', 'LineWidth', 1.5);
        title('加速度');
        xlabel('サンプル数');
        ylabel('加速度 [m/s²]');
        legend({'並進加速度', '角加速度'}, 'Location', 'best');
        grid on;
        
        subplot(4, 3, 7);
        h7 = plot(0, 0, 'r-', 0, 0, 'g-', 0, 0, 'b-', 'LineWidth', 1.5);
        title('速度制御エラー');
        xlabel('サンプル数');
        ylabel('エラー値');
        legend({'速度エラー', '速度積分', '速度微分'}, 'Location', 'best');
        grid on;
        
        subplot(4, 3, 8);
        h8 = plot(0, 0, 'r-', 0, 0, 'g-', 'LineWidth', 1.5);
        title('モータDuty比');
        xlabel('サンプル数');
        ylabel('Duty比');
        legend({'左モータ', '右モータ'}, 'Location', 'best');
        grid on;
        
        subplot(4, 3, 9);
        h9 = plot(0, 0, 'r-', 0, 0, 'g-', 'LineWidth', 1.5);
        title('エンコーダ値');
        xlabel('サンプル数');
        ylabel('エンコーダ値');
        legend({'左エンコーダ', '右エンコーダ'}, 'Location', 'best');
        grid on;
        
        subplot(4, 3, 10);
        h10 = plot(0, 0, 'k-', 'LineWidth', 2);
        title('電源電圧');
        xlabel('サンプル数');
        ylabel('電圧 [V]');
        legend({'バッテリ電圧'}, 'Location', 'best');
        grid on;
        ylim([3.0 4.5]);
        
        subplot(4, 3, 11);
        h11 = plot(0, 0, 'r-', 0, 0, 'g-', 0, 0, 'b-', 'LineWidth', 1.5);
        title('角速度エラー');
        xlabel('サンプル数');
        ylabel('エラー値');
        legend({'角速度エラー', '角速度積分', '角速度微分'}, 'Location', 'best');
        grid on;
        
        subplot(4, 3, 12);
        h12 = plot(0, 0, 'm-', 'LineWidth', 1.5);
        title('処理時間');
        xlabel('サンプル数');
        ylabel('時間 [μs]');
        legend({'制御周期'}, 'Location', 'best');
        grid on;
        
        % === 位置推定専用ウィンドウ：4つのサブプロット ===
        figure(fig_odom);
        
        % 1. 2D軌跡プロット（Y vs X）
        subplot(2, 2, 1);
        h13 = plot(0, 0, 'b-', 0, 0, 'r--', 0, 0, 'go', 0, 0, 'rs', 'LineWidth', 1.5, 'MarkerSize', 8);
        title('2D軌跡 (真値 vs 推定値)');
        xlabel('X位置 [m]');
        ylabel('Y位置 [m]');
        legend({'真値(セル基準)', '推定値(センサ)', 'スタート', '現在位置'}, 'Location', 'best');
        grid on;
        axis equal;
        
        % 2. X位置の時系列
        subplot(2, 2, 2);
        h14 = plot(0, 0, 'b-', 0, 0, 'r--', 'LineWidth', 1.5);
        title('X位置の時系列');
        xlabel('サンプル数');
        ylabel('X位置 [m]');
        legend({'真値(セル基準)', '推定値(センサ)'}, 'Location', 'best');
        grid on;
        
        % 3. Y位置の時系列
        subplot(2, 2, 3);
        h15 = plot(0, 0, 'b-', 0, 0, 'r--', 'LineWidth', 1.5);
        title('Y位置の時系列');
        xlabel('サンプル数');
        ylabel('Y位置 [m]');
        legend({'真値(セル基準)', '推定値(センサ)'}, 'Location', 'best');
        grid on;
        
        % 4. 位置推定誤差
        subplot(2, 2, 4);
        h16 = plot(0, 0, 'r-', 0, 0, 'b--', 'LineWidth', 1.5);
        title('位置推定誤差');
        xlabel('サンプル数');
        ylabel('誤差 [mm]');
        legend({'位置誤差ノルム', '最大誤差'}, 'Location', 'best');
        grid on;
        
        % データ受信と描画のメインループ
        sample_count = 0;
        last_save_time = tic;

        while true
            if s.NumBytesAvailable > 0
                line = readline(s);
                line = strip(line);

                if ~isempty(line) && ~startsWith(line, 'E(')
                    data = parse_csv_line(line);

                    % ★37列データに対応（加速度データ2つ追加）
                    if ~isempty(data) && length(data) >= 37
                        sample_count = sample_count + 1;
                        g_sample_count = sample_count;

                        data_buffer = [data_buffer; data];
                        g_data_buffer = data_buffer;

                        if size(data_buffer, 1) > max_samples
                            data_buffer = data_buffer(end-max_samples+1:end, :);
                            g_data_buffer = data_buffer;
                        end

                        % グラフ更新（1000サンプルごと）
                        if mod(sample_count, 500) == 0
                            update_plots(data_buffer, h1, h2, h3, h4, h5, h6, h7, h8, h9, h10, h11, h12, h13, h14, h15, h16, fig_main, fig_odom);
                            drawnow;
                        end

                        if mod(sample_count, 1000) == 0
                            display_latest_data(data);
                        end

                        if toc(last_save_time) > 120
                            save_temp_data(data_buffer, sample_count);
                            last_save_time = tic;
                        end
                    end
                end
            end

            pause(0.001);
        end

    catch ME
        fprintf('シリアルポート接続エラー: %s\n', ME.message);
        fprintf('利用可能なポートを確認してください。\n');
        return;
    end
end

function data = parse_csv_line(line)
    try
        data_str = split(line, ',');
        data = zeros(1, length(data_str));
        
        for i = 1:length(data_str)
            data(i) = str2double(data_str{i});
        end
        
        if any(isnan(data))
            data = [];
        end
    catch
        data = [];
    end
end

function update_plots(data_buffer, h1, h2, h3, h4, h5, h6, h7, h8, h9, h10, h11, h12, h13, h14, h15, h16, fig_main, fig_odom)
    if isempty(data_buffer)
        return;
    end
    
    sample_indices = 1:size(data_buffer, 1);
    
    % === 既存データの変換 ===
    % 壁センサ値（0-3列）は uint16_t（0-65535）の範囲
    % CSVから読み込んだ値が負の場合は、uint16_t として再解釈は不要
    % （log.cpp で既に uint16_t としてprintf出力しているため、CSV上は正の値のまま）
    wall_fl = data_buffer(:, 1);
    wall_l = data_buffer(:, 2);
    wall_r = data_buffer(:, 3);
    wall_fr = data_buffer(:, 4);
    battery = data_buffer(:, 5) / 1000;
    vel_current = data_buffer(:, 6) / 1000;
    vel_target = data_buffer(:, 7) / 1000;
    sum_len = data_buffer(:, 8) / 1000;
    ang_vel_current = data_buffer(:, 9) / 1000;
    ang_vel_target = data_buffer(:, 10) / 1000;
    rad_current = data_buffer(:, 11) / 1000;
    accel_target = data_buffer(:, 12) / 1000;
    ang_accel_target = data_buffer(:, 13) / 100;  % 角加速度は100倍スケール
    vel_error = data_buffer(:, 14) / 1000;
    vel_i_error = data_buffer(:, 15) / 1000;
    vel_d_error = data_buffer(:, 16) / 1000;
    ang_error = data_buffer(:, 17) / 1000;
    ang_i_error = data_buffer(:, 18) / 1000;
    ang_d_error = data_buffer(:, 19) / 1000;
    duty_l = data_buffer(:, 20) / 1000;
    duty_r = data_buffer(:, 21) / 1000;
    enc_l = data_buffer(:, 22);
    enc_r = data_buffer(:, 23);
    len_current = data_buffer(:, 24) / 1000;
    len_target = data_buffer(:, 25) / 1000;
    
    if size(data_buffer, 2) >= 26
        delta_time = data_buffer(:, 26);
    else
        delta_time = zeros(size(sample_indices));
    end
    
    % === 加速度データの変換（37列対応） ===
    if size(data_buffer, 2) >= 37
        accel_y_raw = data_buffer(:, 36) / 1000;       % 列35 -> MATLAB列36 [m/s²]
        accel_y_filtered = data_buffer(:, 37) / 1000;  % 列36 -> MATLAB列37 [m/s²]
    else
        accel_y_raw = zeros(size(sample_indices));
        accel_y_filtered = zeros(size(sample_indices));
    end
    
    % === 新規：オドメトリデータの変換（35列対応） ===
    if size(data_buffer, 2) >= 35
        % 生センサ推定値
        odom_x_est = data_buffer(:, 28) / 1000;         % 列27 -> MATLAB列28
        odom_y_est = data_buffer(:, 29) / 1000;         % 列28 -> MATLAB列29
        odom_theta_est = data_buffer(:, 30) / 1000;     % 列29 -> MATLAB列30
        
        % セル補正値
        odom_x_corrected = data_buffer(:, 31) / 1000;   % 列30 -> MATLAB列31
        odom_y_corrected = data_buffer(:, 32) / 1000;   % 列31 -> MATLAB列32
        odom_theta_corrected = data_buffer(:, 33) / 1000; % 列32 -> MATLAB列33
        
        % 真値セル座標
        cell_x = data_buffer(:, 34);                    % 列33 -> MATLAB列34
        cell_y = data_buffer(:, 35);                    % 列35 -> MATLAB列35
        
        % === 真値基準位置を計算（セル座標から） ===
        CELL_SIZE = 0.09; % 90mm
        CELL_CENTER_OFFSET = CELL_SIZE / 2.0; % 45mm
        odom_x_ref = cell_x * CELL_SIZE + CELL_CENTER_OFFSET;
        odom_y_ref = cell_y * CELL_SIZE + CELL_CENTER_OFFSET;
        odom_theta_ref = odom_theta_corrected;  % 姿勢角は補正後を使用
        
        % === 誤差をMATLAB側で計算 ===
        % 生センサ推定値の誤差
        odom_x_error = odom_x_est - odom_x_ref;
        odom_y_error = odom_y_est - odom_y_ref;
        odom_theta_error = odom_theta_est - odom_theta_ref;
        odom_pos_error = sqrt(odom_x_error.^2 + odom_y_error.^2);
        
        % セル補正後の誤差
        odom_x_error_corrected = odom_x_corrected - odom_x_ref;
        odom_y_error_corrected = odom_y_corrected - odom_y_ref;
        odom_pos_error_corrected = sqrt(odom_x_error_corrected.^2 + odom_y_error_corrected.^2);
        
        odom_max_error = max(abs(odom_x_error), abs(odom_y_error));
    else
        odom_x_est = zeros(size(sample_indices));
        odom_y_est = zeros(size(sample_indices));
        odom_x_corrected = zeros(size(sample_indices));
        odom_y_corrected = zeros(size(sample_indices));
        odom_x_ref = zeros(size(sample_indices));
        odom_y_ref = zeros(size(sample_indices));
        odom_pos_error = zeros(size(sample_indices));
        odom_pos_error_corrected = zeros(size(sample_indices));
        odom_max_error = zeros(size(sample_indices));
    end
    
    % === メインウィンドウの既存グラフ更新 ===
    figure(fig_main);
    
    subplot(4, 3, 1);
    plot(sample_indices, wall_fl, 'b-', sample_indices, wall_l, 'g-', ...
         sample_indices, wall_r, 'r-', sample_indices, wall_fr, 'm-', 'LineWidth', 1.5);
    title('壁センサ値');
    xlabel('サンプル数');
    ylabel('センサ値');
    legend({'前左', '左', '右', '前右'}, 'Location', 'best');
    grid on;
    
    subplot(4, 3, 2);
    plot(sample_indices, vel_current, 'r-', sample_indices, vel_target, 'g-', 'LineWidth', 1.5);
    title('速度制御');
    xlabel('サンプル数');
    ylabel('速度 [m/s]');
    legend({'現在速度', '目標速度'}, 'Location', 'best');
    grid on;
    
    subplot(4, 3, 3);
    plot(sample_indices, ang_vel_current, 'r-', sample_indices, ang_vel_target, 'g-', 'LineWidth', 1.5);
    title('角速度制御');
    xlabel('サンプル数');
    ylabel('角速度 [rad/s]');
    legend({'現在角速度', '目標角速度'}, 'Location', 'best');
    grid on;
    
    subplot(4, 3, 4);
    plot(sample_indices, sum_len, 'b-', sample_indices, len_current, 'r-', ...
         sample_indices, len_target, 'g-', 'LineWidth', 1.5);
    title('走行距離');
    xlabel('サンプル数');
    ylabel('距離 [m]');
    legend({'累積距離', '現在距離', '目標距離'}, 'Location', 'best');
    grid on;
    
    subplot(4, 3, 5);
    plot(sample_indices, rad_current, 'r-', 'LineWidth', 1.5);
    title('角度');
    xlabel('サンプル数');
    ylabel('角度 [rad]');
    legend({'現在角度'}, 'Location', 'best');
    grid on;
    
    subplot(4, 3, 6);
    plot(sample_indices, accel_target, 'r-', sample_indices, ang_accel_target, 'g-', ...
         sample_indices, accel_y_raw, 'm--', sample_indices, accel_y_filtered, 'c-.', 'LineWidth', 1.5);
    title('加速度 (目標値 & IMU実測値)');
    xlabel('サンプル数');
    ylabel('加速度 [m/s², rad/s²]');
    legend({'並進加速度目標', '角加速度目標', 'IMU生加速度', 'IMU平均(30ms)'}, 'Location', 'best');
    grid on;
    
    if ~isempty(accel_y_raw)
        avg_accel = mean(abs(accel_y_raw));
        max_accel = max(abs(accel_y_raw));
        ylim_vals = ylim;
        text(length(sample_indices)*0.5, ylim_vals(2)*0.9, ...
             sprintf('IMU平均: %.2f m/s²\nIMU最大: %.2f m/s²', avg_accel, max_accel), ...
             'Color', 'blue', 'FontSize', 7, 'FontWeight', 'bold', ...
             'HorizontalAlignment', 'center', 'BackgroundColor', 'white');
    end
    
    subplot(4, 3, 7);
    plot(sample_indices, vel_error, 'r-', sample_indices, vel_i_error, 'g-', ...
         sample_indices, vel_d_error, 'b-', 'LineWidth', 1.5);
    title('速度制御エラー');
    xlabel('サンプル数');
    ylabel('エラー値');
    legend({'速度エラー', '速度積分', '速度微分'}, 'Location', 'best');
    grid on;
    
    subplot(4, 3, 8);
    plot(sample_indices, duty_l, 'r-', sample_indices, duty_r, 'g-', 'LineWidth', 1.5);
    title('モータDuty比');
    xlabel('サンプル数');
    ylabel('Duty比');
    legend({'左モータ', '右モータ'}, 'Location', 'best');
    grid on;
    
    subplot(4, 3, 9);
    plot(sample_indices, enc_l, 'r-', sample_indices, enc_r, 'g-', 'LineWidth', 1.5);
    title('エンコーダ値');
    xlabel('サンプル数');
    ylabel('エンコーダ値');
    legend({'左エンコーダ', '右エンコーダ'}, 'Location', 'best');
    grid on;
    
    subplot(4, 3, 10);
    plot(sample_indices, battery, 'k-', 'LineWidth', 2);
    title('電源電圧');
    xlabel('サンプル数');
    ylabel('電圧 [V]');
    legend({'バッテリ電圧'}, 'Location', 'best');
    grid on;
    ylim([3.0 4.5]);
    
    if ~isempty(battery) && min(battery) < 3.3
        text(length(sample_indices)*0.5, 3.35, '低電圧警告!', ...
             'Color', 'red', 'FontSize', 12, 'FontWeight', 'bold', ...
             'HorizontalAlignment', 'center');
    end
    
    subplot(4, 3, 11);
    plot(sample_indices, ang_error, 'r-', sample_indices, ang_i_error, 'g-', ...
         sample_indices, ang_d_error, 'b-', 'LineWidth', 1.5);
    title('角速度制御エラー');
    xlabel('サンプル数');
    ylabel('エラー値');
    legend({'角速度エラー', '角速度積分', '角速度微分'}, 'Location', 'best');
    grid on;
    
    subplot(4, 3, 12);
    plot(sample_indices, delta_time, 'm-', 'LineWidth', 1.5);
    title('制御処理時間');
    xlabel('サンプル数');
    ylabel('時間 [μs]');
    legend({'制御周期'}, 'Location', 'best');
    grid on;
    
    if ~isempty(delta_time) && max(delta_time) > 1000
        text(length(sample_indices)*0.5, max(delta_time)*0.9, '処理時間オーバー!', ...
             'Color', 'red', 'FontSize', 10, 'FontWeight', 'bold', ...
             'HorizontalAlignment', 'center');
    end
    
    % === 位置推定専用ウィンドウの更新 ===
    figure(fig_odom);
    
    % 1. 2D軌跡プロット（Y vs X、縦軸Y・横軸X）- 3つの軌跡を表示
    subplot(2, 2, 1);
    hold off;
    
    % 3つの軌跡を線で描画
    plot(odom_x_ref, odom_y_ref, 'b-', 'LineWidth', 2);
    hold on;
    plot(odom_x_est, odom_y_est, 'r--', 'LineWidth', 1.5);
    plot(odom_x_corrected, odom_y_corrected, 'g-.', 'LineWidth', 1.5);
    
    % スタート地点と現在位置をマーク
    if ~isempty(odom_x_ref)
        plot(odom_x_ref(1), odom_y_ref(1), 'ko', 'MarkerSize', 12, 'MarkerFaceColor', 'k', 'LineWidth', 2);
        plot(odom_x_ref(end), odom_y_ref(end), 'bs', 'MarkerSize', 10, 'MarkerFaceColor', 'b', 'LineWidth', 2);
        plot(odom_x_est(end), odom_y_est(end), 'rs', 'MarkerSize', 10, 'MarkerFaceColor', 'r', 'LineWidth', 2);
        plot(odom_x_corrected(end), odom_y_corrected(end), 'gs', 'MarkerSize', 10, 'MarkerFaceColor', 'g', 'LineWidth', 2);
    end
    
    title('2D軌跡 (真値 vs 生センサ vs セル補正後)');
    xlabel('X位置 [m]');
    ylabel('Y位置 [m]');
    legend({'真値(セル中心)', '生センサ推定', 'セル補正後', 'スタート', '真値現在', '生センサ現在', '補正後現在'}, 'Location', 'best', 'FontSize', 8);
    grid on;
    axis equal;
    hold off;
    
    % 2. X位置の時系列 - 3つの軌跡を線で表示
    subplot(2, 2, 2);
    hold off;
    plot(sample_indices, odom_x_ref, 'b-', 'LineWidth', 1.5);
    hold on;
    plot(sample_indices, odom_x_est, 'r--', 'LineWidth', 1);
    plot(sample_indices, odom_x_corrected, 'g-.', 'LineWidth', 1);
    title('X位置の時系列');
    xlabel('サンプル数');
    ylabel('X位置 [m]');
    legend({'真値(セル中心)', '生センサ推定', 'セル補正後'}, 'Location', 'best');
    grid on;
    hold off;
    
    if ~isempty(odom_x_error)
        avg_x_error_raw = mean(abs(odom_x_error)) * 1000;
        avg_x_error_corrected = mean(abs(odom_x_error_corrected)) * 1000;
        ylim_vals = ylim;
        text(length(sample_indices)*0.1, ylim_vals(2)*0.9, ...
             sprintf('生センサ誤差: %.2f mm\nセル補正後: %.2f mm', avg_x_error_raw, avg_x_error_corrected), ...
             'Color', 'red', 'FontSize', 8, 'FontWeight', 'bold', ...
             'BackgroundColor', 'white');
    end
    
    % 3. Y位置の時系列 - 3つの軌跡を線で表示
    subplot(2, 2, 3);
    hold off;
    plot(sample_indices, odom_y_ref, 'b-', 'LineWidth', 1.5);
    hold on;
    plot(sample_indices, odom_y_est, 'r--', 'LineWidth', 1);
    plot(sample_indices, odom_y_corrected, 'g-.', 'LineWidth', 1);
    title('Y位置の時系列');
    xlabel('サンプル数');
    ylabel('Y位置 [m]');
    legend({'真値(セル中心)', '生センサ推定', 'セル補正後'}, 'Location', 'best');
    grid on;
    hold off;
    
    if ~isempty(odom_y_error)
        avg_y_error_raw = mean(abs(odom_y_error)) * 1000;
        avg_y_error_corrected = mean(abs(odom_y_error_corrected)) * 1000;
        ylim_vals = ylim;
        text(length(sample_indices)*0.1, ylim_vals(2)*0.9, ...
             sprintf('生センサ誤差: %.2f mm\nセル補正後: %.2f mm', avg_y_error_raw, avg_y_error_corrected), ...
             'Color', 'red', 'FontSize', 8, 'FontWeight', 'bold', ...
             'BackgroundColor', 'white');
    end
    
    % 4. 位置推定誤差（生センサ vs セル補正後）
    subplot(2, 2, 4);
    plot(sample_indices, odom_pos_error * 1000, 'r-', 'LineWidth', 1.5);
    hold on;
    plot(sample_indices, odom_pos_error_corrected * 1000, 'g-', 'LineWidth', 1.5);
    plot(sample_indices, odom_max_error * 1000, 'b--', 'LineWidth', 1);
    hold off;
    title('位置推定誤差（補正効果の比較）');
    xlabel('サンプル数');
    ylabel('誤差 [mm]');
    legend({'生センサ誤差', 'セル補正後誤差', '最大誤差'}, 'Location', 'best');
    grid on;
    
    if ~isempty(odom_pos_error)
        avg_pos_error_raw = mean(odom_pos_error) * 1000;
        avg_pos_error_corrected = mean(odom_pos_error_corrected) * 1000;
        max_pos_error_raw = max(odom_pos_error) * 1000;
        max_pos_error_corrected = max(odom_pos_error_corrected) * 1000;
        improvement = ((avg_pos_error_raw - avg_pos_error_corrected) / avg_pos_error_raw) * 100;
        
        text(length(sample_indices)*0.5, max(max_pos_error_raw, max_pos_error_corrected)*0.8, ...
             sprintf('生センサ: 平均%.2fmm 最大%.2fmm\nセル補正: 平均%.2fmm 最大%.2fmm\n改善率: %.1f%%', ...
                     avg_pos_error_raw, max_pos_error_raw, ...
                     avg_pos_error_corrected, max_pos_error_corrected, improvement), ...
             'Color', 'blue', 'FontSize', 8, 'FontWeight', 'bold', ...
             'HorizontalAlignment', 'center', 'BackgroundColor', 'white');
    end
    
    if ~isempty(odom_pos_error_corrected) && max(odom_pos_error_corrected) * 1000 > 10
        text(length(sample_indices)*0.5, max(odom_pos_error_corrected)*1000*0.95, ...
             '補正後も高誤差!', 'Color', 'red', 'FontSize', 10, 'FontWeight', 'bold', ...
             'HorizontalAlignment', 'center');
    end
end

function display_latest_data(data)
    if length(data) >= 37
        % 誤差はMATLAB側で計算するため、位置情報から算出
        cell_x = data(34);
        cell_y = data(35);
        CELL_SIZE = 0.09;
        CELL_CENTER_OFFSET = CELL_SIZE / 2.0;
        ref_x = cell_x * CELL_SIZE + CELL_CENTER_OFFSET;
        ref_y = cell_y * CELL_SIZE + CELL_CENTER_OFFSET;
        est_x = data(28) / 1000;
        est_y = data(29) / 1000;
        pos_error = sqrt((est_x - ref_x)^2 + (est_y - ref_y)^2) * 1000; % mm
        
        % 加速度データ
        accel_raw = data(36) / 1000;     % m/s²
        accel_filtered = data(37) / 1000; % m/s²
        
        fprintf('サンプル: 壁[%4d,%4d,%4d,%4d] 速度[%.3f] 位置誤差[%.2fmm] 加速度[%.2f|%.2f] バッテリ[%.2fV]\n', ...
            data(1), data(2), data(3), data(4), data(6)/1000, pos_error, accel_raw, accel_filtered, data(5)/1000);
    elseif length(data) >= 35
        % 後方互換性のため35列データにも対応
        cell_x = data(34);
        cell_y = data(35);
        CELL_SIZE = 0.09;
        CELL_CENTER_OFFSET = CELL_SIZE / 2.0;
        ref_x = cell_x * CELL_SIZE + CELL_CENTER_OFFSET;
        ref_y = cell_y * CELL_SIZE + CELL_CENTER_OFFSET;
        est_x = data(28) / 1000;
        est_y = data(29) / 1000;
        pos_error = sqrt((est_x - ref_x)^2 + (est_y - ref_y)^2) * 1000; % mm
        
        fprintf('サンプル: 壁[%4d,%4d,%4d,%4d] 速度[%.3f] 位置誤差[%.2fmm] バッテリ[%.2fV]\n', ...
            data(1), data(2), data(3), data(4), data(6)/1000, pos_error, data(5)/1000);
    end
end

function save_temp_data(data_buffer, sample_count)
    if ~isempty(data_buffer)
        try
            timestamp = datestr(now, 'yyyymmdd_HHMMSS');
            temp_filename = sprintf('temp_save_%s.mat', timestamp);
            save(temp_filename, 'data_buffer');
            fprintf('一時保存完了: %s (サンプル数: %d)\n', temp_filename, sample_count);
        catch
        end
    end
end

function cleanup_function()
    global g_data_buffer g_serial_port g_sample_count;

    fprintf('\n\n=== 緊急停止処理中 ===\n');

    try
        if ~isempty(g_serial_port)
            clear g_serial_port;
            fprintf('シリアルポートを切断しました。\n');
        end
    catch
    end

    if ~isempty(g_data_buffer)
        try
            fprintf('受信データを緊急保存しています...\n');
            timestamp = datestr(now, 'yyyymmdd_HHMMSS');

            mat_filename = sprintf('emergency_save_%s.mat', timestamp);
            data_buffer = g_data_buffer;
            save(mat_filename, 'data_buffer');

            csv_filename = sprintf('emergency_save_%s.csv', timestamp);
            save_csv_file(g_data_buffer, csv_filename);

            fprintf('緊急保存完了:\n');
            fprintf('  総サンプル数: %d個\n', g_sample_count);
            fprintf('  保存ファイル: %s, %s\n', mat_filename, csv_filename);

        catch ME
            fprintf('緊急保存エラー: %s\n', ME.message);
        end
    else
        fprintf('保存するデータがありません。\n');
    end

    fprintf('=== 緊急停止処理完了 ===\n');

    clear global g_data_buffer g_serial_port g_sample_count;
end

function save_csv_file(data_buffer, filename)
    try
        header = {'wall_fl', 'wall_l', 'wall_r', 'wall_fr', 'battery_mV', ...
                  'vel_current_mm_s', 'vel_target_mm_s', 'sum_len_mm', ...
                  'ang_vel_current_mrad_s', 'ang_vel_target_mrad_s', 'rad_current_mrad', ...
                  'accel_target_mm_s2', 'ang_accel_target_mrad_s2', ...
                  'vel_error_mm_s', 'vel_i_error_mm_s', 'vel_d_error_mm_s', ...
                  'ang_error_mrad_s', 'ang_i_error_mrad_s', 'ang_d_error_mrad_s', ...
                  'duty_l_1000', 'duty_r_1000', 'enc_l', 'enc_r', ...
                  'len_current_mm', 'len_target_mm', 'delta_time', 'thinking_flag', ...
                  'odom_x_est_mm', 'odom_y_est_mm', 'odom_theta_est_mrad', ...
                  'odom_x_corrected_mm', 'odom_y_corrected_mm', 'odom_theta_corrected_mrad', ...
                  'cell_x', 'cell_y', 'accel_y_raw_mm_s2', 'accel_y_filtered_mm_s2'};

        T = array2table(data_buffer, 'VariableNames', header);
        writetable(T, filename);
    catch
        csvwrite(filename, data_buffer);
    end
end
