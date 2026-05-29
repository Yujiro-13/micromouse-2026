function start_mouse_logger()
    % マイクロマウスログ解析システムのメインエントリーポイント
    % ESP32マイクロマウスからのlog_print出力をリアルタイムで受信・表示
    
    clc;
    fprintf('======================================\n');
    fprintf('   マイクロマウスログ解析システム\n');
    fprintf('======================================\n');
    fprintf('\n');
    fprintf('ESP32マイクロマウスのlog_print関数から出力される\n');
    fprintf('ログデータをリアルタイムで受信・グラフ表示します。\n');
    fprintf('\n');
    fprintf('機能一覧:\n');
    fprintf('1. リアルタイムログビューア - シリアル受信とグラフ表示\n');
    fprintf('2. 詳細分析ツール - 保存済みデータの解析\n');
    fprintf('3. 簡易ビューア - 軽量版グラフ表示\n');
    fprintf('\n');
    
    while true
        choice = input('選択してください (1-3, qで終了): ', 's');
        
        switch lower(choice)
            case '1'
                fprintf('\nリアルタイムログビューアを起動します...\n');
                mouse_log_viewer();
                break;
                
            case '2'
                fprintf('\n詳細分析ツールを起動します...\n');
                mouse_log_analyzer();
                break;
                
            case '3'
                fprintf('\n簡易ビューアを起動します...\n');
                simple_mouse_viewer();
                break;
                
            case 'q'
                fprintf('\n終了します。\n');
                break;
                
            otherwise
                fprintf('無効な選択です。1-3またはqを入力してください。\n');
        end
    end
end

function simple_mouse_viewer()
    % 軽量版のリアルタイムビューア
    % CPU負荷を抑えた簡易表示
    
    port = input('COM9', 's');
    baudrate = 115200;
    
    try
        s = serialport(port, baudrate);
        configureTerminator(s, "LF");
        
        fprintf('シリアルポート %s に接続しました。\n', port);
        fprintf('簡易ログ表示を開始します (Ctrl+Cで終了)...\n\n');
        
        % 簡易グラフウィンドウ
        figure('Name', '簡易ログビューア', 'Position', [200, 200, 1200, 800]);
        
        % データバッファ
        max_samples = 1000;
        data_buffer = [];
        sample_count = 0;
        
        % サブプロット
        subplot(2, 3, 1);
        h1 = plot(0, 0, 'b-');
        title('速度');
        ylabel('m/s');
        grid on;
        
        subplot(2, 3, 2);
        h2 = plot(0, 0, 'r-');
        title('角速度');
        ylabel('rad/s');
        grid on;
        
        subplot(2, 3, 3);
        h3 = plot(0, 0, 'g-');
        title('走行距離');
        ylabel('m');
        grid on;
        
        subplot(2, 3, 4);
        h4 = plot(0, 0, 'k-', 0, 0, 'm-');
        title('壁センサ (L/R)');
        legend({'左', '右'});
        grid on;
        
        subplot(2, 3, 5);
        h5 = plot(0, 0, 'r-', 'LineWidth', 2);
        title('電源電圧');
        ylabel('V');
        ylim([3.0 4.5]);
        grid on;
        
        subplot(2, 3, 6);
        h6 = plot(0, 0, 'c-', 0, 0, 'b-');
        title('モータDuty比');
        ylabel('Duty');
        legend({'左', '右'});
        grid on;
        
        while true
            try
                if s.NumBytesAvailable > 0
                    line = readline(s);
                    line = strip(line);
                    
                    if ~isempty(line) && ~startsWith(line, 'E(')
                        data = parse_simple_csv(line);
                        
                        if ~isempty(data) && length(data) >= 20
                            sample_count = sample_count + 1;
                            data_buffer = [data_buffer; data(1:20)];
                            
                            if size(data_buffer, 1) > max_samples
                                data_buffer = data_buffer(end-max_samples+1:end, :);
                            end
                            
                            % 50サンプルごとに更新
                            if mod(sample_count, 50) == 0
                                update_simple_plots(data_buffer);
                                drawnow;
                            end
                            
                            % コンソール出力
                            if mod(sample_count, 100) == 0
                                fprintf('速度: %.3f m/s, 角速度: %.3f rad/s, 距離: %.3f m, 電圧: %.2f V\n', ...
                                    data(6)/1000, data(9)/1000, data(8)/1000, data(5)/1000);
                            end
                        end
                    end
                end
                pause(0.001);
                
            catch ME
                if strcmp(ME.identifier, 'MATLAB:interruption')
                    fprintf('\n簡易ビューアを停止しました（ユーザー中断）。\n');
                    break;
                else
                    fprintf('エラー: %s\n', ME.message);
                end
            end
        end
        
    catch ME
        fprintf('接続エラー: %s\n', ME.message);
    end
    
    try
        clear s;
        fprintf('シリアルポートを正常に切断しました。\n');
        fprintf('簡易ビューア終了: 総サンプル数 %d個\n', sample_count);
    catch
    end
end

function data = parse_simple_csv(line)
    try
        data_str = split(line, ',');
        data = zeros(1, min(length(data_str), 35));  % 35列に対応（オドメトリ+セル座標）
        
        for i = 1:length(data)
            data(i) = str2double(data_str{i});
        end
        
        if any(isnan(data))
            data = [];
        end
    catch
        data = [];
    end
end

function update_simple_plots(data_buffer)
    if isempty(data_buffer)
        return;
    end
    
    samples = 1:size(data_buffer, 1);
    
    % 速度
    subplot(2, 3, 1);
    plot(samples, data_buffer(:, 6)/1000, 'b-', samples, data_buffer(:, 7)/1000, 'r--');
    title('速度');
    ylabel('m/s');
    legend({'実際', '目標'});
    grid on;
    
    % 角速度
    subplot(2, 3, 2);
    plot(samples, data_buffer(:, 9)/1000, 'r-', samples, data_buffer(:, 10)/1000, 'g--');
    title('角速度');
    ylabel('rad/s');
    legend({'実際', '目標'});
    grid on;
    
    % 距離
    subplot(2, 3, 3);
    plot(samples, data_buffer(:, 8)/1000, 'g-');
    title('累積走行距離');
    ylabel('m');
    grid on;
    
    % 壁センサ
    subplot(2, 3, 4);
    plot(samples, data_buffer(:, 2), 'k-', samples, data_buffer(:, 3), 'm-');
    title('左右壁センサ');
    legend({'左', '右'});
    grid on;
    
    % 電源電圧
    subplot(2, 3, 5);
    plot(samples, data_buffer(:, 5)/1000, 'r-', 'LineWidth', 2);
    title('電源電圧');
    ylabel('V');
    ylim([3.0 4.5]);
    grid on;
    
    % 低電圧警告
    battery_voltage = data_buffer(:, 5)/1000;
    if ~isempty(battery_voltage) && min(battery_voltage) < 3.3
        text(length(samples)*0.5, 3.35, '低電圧!', ...
             'Color', 'red', 'FontSize', 10, 'FontWeight', 'bold', ...
             'HorizontalAlignment', 'center');
    end
    
    % モータDuty比
    subplot(2, 3, 6);
    if size(data_buffer, 2) >= 21
        plot(samples, data_buffer(:, 20)/1000, 'c-', samples, data_buffer(:, 21)/1000, 'b-');
        title('モータDuty比');
        ylabel('Duty');
        legend({'左', '右'});
        grid on;
    end
end
