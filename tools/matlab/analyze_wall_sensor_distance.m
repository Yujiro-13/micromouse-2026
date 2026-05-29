function analyze_wall_sensor_distance()
    % 壁センサ値と走行距離の関係を解析するスクリプト
    % 
    % 使用方法:
    % 1. mouse_log_viewer_with_odom.mで計測したデータファイル(.mat)を読み込む
    % 2. data_bufferから壁センサ値と走行距離を抽出
    % 3. センサ値-距離の関係をプロット
    % 4. ルックアップテーブル用のデータを生成
    
    fprintf('=== 壁センサ距離推定データ解析ツール ===\n\n');
    
    % データファイルの選択
    [filename, filepath] = uigetfile('*.mat', '計測データファイルを選択してください');
    if isequal(filename, 0)
        fprintf('ファイルが選択されませんでした。\n');
        return;
    end
    
    fullpath = fullfile(filepath, filename);
    fprintf('読み込み中: %s\n', fullpath);
    
    % データの読み込み
    loaded_data = load(fullpath);
    
    % data_buffer変数を探す
    if isfield(loaded_data, 'data_buffer')
        data_buffer = loaded_data.data_buffer;
    else
        % 最初のフィールドを使用
        field_names = fieldnames(loaded_data);
        data_buffer = loaded_data.(field_names{1});
    end
    
    total_samples = size(data_buffer, 1);
    fprintf('データサンプル数: %d\n', total_samples);
    fprintf('データ列数: %d\n\n', size(data_buffer, 2));
    
    % サンプル範囲の指定
    fprintf('--- サンプル範囲の選択 ---\n');
    fprintf('全データ: 1 - %d サンプル\n', total_samples);
    fprintf('解析する範囲を指定してください（Enterで全データ使用）:\n');
    
    start_sample = input(sprintf('開始サンプル (1-%d) [1]: ', total_samples));
    if isempty(start_sample) || start_sample < 1
        start_sample = 1;
    end
    
    end_sample = input(sprintf('終了サンプル (%d-%d) [%d]: ', start_sample, total_samples, total_samples));
    if isempty(end_sample) || end_sample > total_samples
        end_sample = total_samples;
    end
    
    % 範囲の妥当性チェック
    if start_sample >= end_sample
        fprintf('エラー: 開始サンプルは終了サンプルより小さくする必要があります。\n');
        return;
    end
    
    % 指定範囲のデータを抽出
    data_buffer = data_buffer(start_sample:end_sample, :);
    fprintf('\n選択範囲: %d - %d サンプル (%d サンプル)\n\n', ...
            start_sample, end_sample, end_sample - start_sample + 1);
    
    % データ列の定義（mouse_log_viewer_with_odom.mに基づく）
    % MATLAB配列は1始まりなので+1している
    COL_WALL_FL = 1;   % 前左壁センサ (adcs[0])
    COL_WALL_L = 2;    % 左壁センサ (adcs[1])
    COL_WALL_R = 3;    % 右壁センサ (adcs[2])
    COL_WALL_FR = 4;   % 前右壁センサ (adcs[3])
    COL_SUM_LEN = 8;   % 累積走行距離 [mm] (adcs[7])
    COL_LEN_CURRENT = 24; % 現在走行距離 [mm] (adcs[23])
    
    % データが十分な列数を持っているか確認
    if size(data_buffer, 2) < 24
        fprintf('エラー: データ列数が不足しています（最低24列必要）\n');
        return;
    end
    
    % データ抽出
    wall_fl = data_buffer(:, COL_WALL_FL);
    wall_l = data_buffer(:, COL_WALL_L);
    wall_r = data_buffer(:, COL_WALL_R);
    wall_fr = data_buffer(:, COL_WALL_FR);
    sum_len = data_buffer(:, COL_SUM_LEN) / 1000; % mm -> m に変換
    len_current = data_buffer(:, COL_LEN_CURRENT) / 1000; % mm -> m に変換
    
    % サンプルインデックス
    sample_indices = 1:length(wall_fl);
    
    % 統計情報表示
    fprintf('--- センサ値統計 ---\n');
    fprintf('[前壁センサ]\n');
    fprintf('  前左センサ (FL): Min=%d, Max=%d, Mean=%.1f\n', ...
        min(wall_fl), max(wall_fl), mean(wall_fl));
    fprintf('  前右センサ (FR): Min=%d, Max=%d, Mean=%.1f\n', ...
        min(wall_fr), max(wall_fr), mean(wall_fr));
    fprintf('[側壁センサ]\n');
    fprintf('  左センサ   (L):  Min=%d, Max=%d, Mean=%.1f\n', ...
        min(wall_l), max(wall_l), mean(wall_l));
    fprintf('  右センサ   (R):  Min=%d, Max=%d, Mean=%.1f\n', ...
        min(wall_r), max(wall_r), mean(wall_r));
    fprintf('\n');
    fprintf('走行距離: Min=%.3fm, Max=%.3fm\n', min(sum_len), max(sum_len));
    fprintf('\n');
    
    % グラフ描画
    fig = figure('Name', '壁センサ距離キャリブレーション解析', ...
                 'Position', [100, 100, 1400, 900]);
    
    % 1. 時系列プロット（サンプル数 vs センサ値）
    subplot(3, 2, 1);
    plot(sample_indices, wall_fl, 'b-', 'LineWidth', 1.5);
    hold on;
    plot(sample_indices, wall_l, 'g-', 'LineWidth', 1.5);
    plot(sample_indices, wall_r, 'r-', 'LineWidth', 1.5);
    plot(sample_indices, wall_fr, 'm-', 'LineWidth', 1.5);
    hold off;
    title('壁センサ値の時系列変化');
    xlabel('サンプル数');
    ylabel('センサ値');
    legend({'前左(FL)', '左(L)', '右(R)', '前右(FR)'}, 'Location', 'best');
    grid on;
    
    % 2. 走行距離の時系列
    subplot(3, 2, 2);
    plot(sample_indices, sum_len, 'b-', 'LineWidth', 2);
    hold on;
    plot(sample_indices, len_current, 'r--', 'LineWidth', 1.5);
    hold off;
    title('走行距離の時系列変化');
    xlabel('サンプル数');
    ylabel('距離 [m]');
    legend({'累積距離', '現在距離'}, 'Location', 'best');
    grid on;
    
    % 3. 走行距離 vs 前左センサ値
    subplot(3, 2, 3);
    scatter(sum_len, wall_fl, 10, 'b', 'filled');
    title('走行距離 vs 前左センサ値 (FL)');
    xlabel('走行距離 [m]');
    ylabel('センサ値');
    grid on;
    
    % 4. 走行距離 vs 左センサ値
    subplot(3, 2, 4);
    scatter(sum_len, wall_l, 10, 'g', 'filled');
    title('走行距離 vs 左センサ値 (L)');
    xlabel('走行距離 [m]');
    ylabel('センサ値');
    grid on;
    
    % 5. 走行距離 vs 右センサ値
    subplot(3, 2, 5);
    scatter(sum_len, wall_r, 10, 'r', 'filled');
    title('走行距離 vs 右センサ値 (R)');
    xlabel('走行距離 [m]');
    ylabel('センサ値');
    grid on;
    
    % 6. 走行距離 vs 前右センサ値
    subplot(3, 2, 6);
    scatter(sum_len, wall_fr, 10, 'm', 'filled');
    title('走行距離 vs 前右センサ値 (FR)');
    xlabel('走行距離 [m]');
    ylabel('センサ値');
    grid on;
    
    % データをワークスペースに保存
    fprintf('解析結果をワークスペースに保存しています...\n');
    assignin('base', 'wall_fl', wall_fl);
    assignin('base', 'wall_l', wall_l);
    assignin('base', 'wall_r', wall_r);
    assignin('base', 'wall_fr', wall_fr);
    assignin('base', 'sum_len', sum_len);
    assignin('base', 'len_current', len_current);
    assignin('base', 'sample_indices', sample_indices);
    
    % 範囲情報も保存
    range_info.start_sample = start_sample;
    range_info.end_sample = end_sample;
    range_info.total_samples = end_sample - start_sample + 1;
    range_info.source_file = fullpath;
    assignin('base', 'range_info', range_info);
    
    % 追加の詳細解析図
    analyze_sensor_distance_relationship(wall_fl, wall_l, wall_r, wall_fr, sum_len, sample_indices);
    
    fprintf('\n=== 解析完了 ===\n');
    fprintf('解析範囲: サンプル %d - %d (%d サンプル)\n', ...
            start_sample, end_sample, end_sample - start_sample + 1);
    fprintf('\n次のステップ:\n');
    fprintf('1. グラフから壁との距離とセンサ値の関係を確認\n');
    fprintf('2. generate_wall_distance_lut.m を実行してルックアップテーブルを生成\n');
    fprintf('3. ワークスペース変数を使用して追加解析可能:\n');
    fprintf('   - wall_fl, wall_l, wall_r, wall_fr: センサ値\n');
    fprintf('   - sum_len: 走行距離 [m]\n');
    fprintf('   - range_info: 解析範囲情報\n');
end

function analyze_sensor_distance_relationship(wall_fl, wall_l, wall_r, wall_fr, sum_len, sample_indices)
    % センサ値と距離の関係をより詳細に解析
    
    fig2 = figure('Name', 'センサ値-距離関係の詳細解析', ...
                  'Position', [150, 50, 1400, 900]);
    
    % 前提：迷路の壁は固定位置にあり、ロボットが前進すると壁との距離が変化
    % ただし、実際の距離は走行距離から計算できない（壁の位置が不明）
    % ここでは、センサ値の変化パターンと走行距離の関係を可視化
    
    % 1. 前左センサの詳細分析
    subplot(2, 2, 1);
    scatter(sum_len, wall_fl, 10, sample_indices, 'filled');
    colormap(jet);
    colorbar;
    title('前左センサ (FL) - 時系列カラーマップ');
    xlabel('走行距離 [m]');
    ylabel('センサ値');
    grid on;
    
    % 2. 左センサの詳細分析
    subplot(2, 2, 2);
    scatter(sum_len, wall_l, 10, sample_indices, 'filled');
    colormap(jet);
    colorbar;
    title('左センサ (L) - 時系列カラーマップ');
    xlabel('走行距離 [m]');
    ylabel('センサ値');
    grid on;
    
    % 3. 右センサの詳細分析
    subplot(2, 2, 3);
    scatter(sum_len, wall_r, 10, sample_indices, 'filled');
    colormap(jet);
    colorbar;
    title('右センサ (R) - 時系列カラーマップ');
    xlabel('走行距離 [m]');
    ylabel('センサ値');
    grid on;
    
    % 4. 前右センサの詳細分析
    subplot(2, 2, 4);
    scatter(sum_len, wall_fr, 10, sample_indices, 'filled');
    colormap(jet);
    colorbar;
    title('前右センサ (FR) - 時系列カラーマップ');
    xlabel('走行距離 [m]');
    ylabel('センサ値');
    grid on;
    
    % センサ値変化の検出（壁の存在を推定）
    detect_wall_transitions(wall_fl, wall_l, wall_r, wall_fr, sum_len);
end

function detect_wall_transitions(wall_fl, wall_l, wall_r, wall_fr, sum_len)
    % センサ値の急激な変化から壁との距離変化を検出
    
    fprintf('\n--- 壁検出解析 ---\n');
    
    % 移動平均フィルタでノイズ除去
    window_size = 50;
    wall_fl_smooth = movmean(wall_fl, window_size);
    wall_l_smooth = movmean(wall_l, window_size);
    wall_r_smooth = movmean(wall_r, window_size);
    wall_fr_smooth = movmean(wall_fr, window_size);
    
    % 微分して変化率を計算
    diff_fl = [0; diff(wall_fl_smooth)];
    diff_l = [0; diff(wall_l_smooth)];
    diff_r = [0; diff(wall_r_smooth)];
    diff_fr = [0; diff(wall_fr_smooth)];
    
    % 大きな変化を検出（閾値は経験的に設定）
    threshold = 100; % センサ値の変化閾値
    
    significant_change_fl = find(abs(diff_fl) > threshold);
    significant_change_l = find(abs(diff_l) > threshold);
    significant_change_r = find(abs(diff_r) > threshold);
    significant_change_fr = find(abs(diff_fr) > threshold);
    
    fprintf('前左センサ: %d箇所で大きな変化を検出\n', length(significant_change_fl));
    fprintf('左センサ:   %d箇所で大きな変化を検出\n', length(significant_change_l));
    fprintf('右センサ:   %d箇所で大きな変化を検出\n', length(significant_change_r));
    fprintf('前右センサ: %d箇所で大きな変化を検出\n', length(significant_change_fr));
    
    % 変化率のプロット
    fig3 = figure('Name', 'センサ値変化率解析', ...
                  'Position', [200, 100, 1400, 600]);
    
    subplot(2, 2, 1);
    plot(sum_len, diff_fl, 'b-', 'LineWidth', 1);
    hold on;
    plot(sum_len(significant_change_fl), diff_fl(significant_change_fl), 'ro', 'MarkerSize', 8);
    hold off;
    title('前左センサ (FL) の変化率');
    xlabel('走行距離 [m]');
    ylabel('センサ値変化率');
    grid on;
    
    subplot(2, 2, 2);
    plot(sum_len, diff_l, 'g-', 'LineWidth', 1);
    hold on;
    plot(sum_len(significant_change_l), diff_l(significant_change_l), 'ro', 'MarkerSize', 8);
    hold off;
    title('左センサ (L) の変化率');
    xlabel('走行距離 [m]');
    ylabel('センサ値変化率');
    grid on;
    
    subplot(2, 2, 3);
    plot(sum_len, diff_r, 'r-', 'LineWidth', 1);
    hold on;
    plot(sum_len(significant_change_r), diff_r(significant_change_r), 'ro', 'MarkerSize', 8);
    hold off;
    title('右センサ (R) の変化率');
    xlabel('走行距離 [m]');
    ylabel('センサ値変化率');
    grid on;
    
    subplot(2, 2, 4);
    plot(sum_len, diff_fr, 'm-', 'LineWidth', 1);
    hold on;
    plot(sum_len(significant_change_fr), diff_fr(significant_change_fr), 'ro', 'MarkerSize', 8);
    hold off;
    title('前右センサ (FR) の変化率');
    xlabel('走行距離 [m]');
    ylabel('センサ値変化率');
    grid on;
    
    fprintf('\n※注意: 実際の壁までの距離を推定するには、\n');
    fprintf('  迷路内で既知距離の壁に対して計測する必要があります。\n');
    fprintf('  例: 壁から5cm, 10cm, 15cmの位置でセンサ値を記録\n');
end
