function generate_wall_distance_lut()
    % 壁センサ値から距離への変換用ルックアップテーブル生成
    %
    % 使用方法:
    % 1. analyze_wall_sensor_distance.m を実行してデータを解析
    % 2. このスクリプトを実行してLUTを生成
    % 3. 生成されたヘッダーファイルをC++プロジェクトに追加
    %
    % 前提条件:
    % - ワークスペースに以下の変数が存在すること
    %   wall_fl, wall_l, wall_r, wall_fr: センサ値
    %   sum_len: 走行距離 [m]
    
    fprintf('=== 壁センサ距離変換LUT生成ツール ===\n\n');
    
    % ワークスペース変数の確認
    if ~evalin('base', 'exist(''wall_fl'', ''var'')')
        fprintf('エラー: ワークスペースにデータが見つかりません。\n');
        fprintf('まず analyze_wall_sensor_distance.m を実行してください。\n');
        return;
    end
    
    % データの取得
    wall_fl = evalin('base', 'wall_fl');
    wall_l = evalin('base', 'wall_l');
    wall_r = evalin('base', 'wall_r');
    wall_fr = evalin('base', 'wall_fr');
    sum_len = evalin('base', 'sum_len');
    
    fprintf('データサンプル数: %d\n\n', length(wall_fl));
    
    % ユーザー入力：キャリブレーション方法の選択
    fprintf('キャリブレーション方法を選択してください:\n');
    fprintf('1. 単純マッピング（センサ値 -> 走行距離）\n');
    fprintf('2. 既知距離でのキャリブレーション（手動入力）\n');
    fprintf('3. 計測データから距離推定（analyze結果を利用）\n');
    choice = input('選択 (1, 2, or 3): ');
    
    if choice == 3
        % 計測データから距離推定
        generate_lut_from_measured_data(wall_fl, wall_l, wall_r, wall_fr, sum_len);
    elseif choice == 2
        % 既知距離でのキャリブレーション
        generate_lut_with_known_distances(wall_fl, wall_l, wall_r, wall_fr);
    else
        % 単純マッピング
        generate_simple_lut(wall_fl, wall_l, wall_r, wall_fr, sum_len);
    end
    
    fprintf('\n=== LUT生成完了 ===\n');
end

function generate_simple_lut(wall_fl, wall_l, wall_r, wall_fr, sum_len)
    % 単純マッピング方式（走行距離をそのまま使用）
    % 注意: この方法は壁との実際の距離を正確に反映しません
    
    fprintf('\n--- 単純マッピング方式 ---\n');
    fprintf('※警告: この方法は参考値です。正確な距離推定には既知距離での\n');
    fprintf('  キャリブレーションが必要です。\n\n');
    
    % LUTのサイズを決定
    lut_size = input('LUTサイズを入力してください (推奨: 20-50): ');
    if isempty(lut_size) || lut_size < 10
        lut_size = 30;
    end
    
    % 4つのセンサ個別にLUTを生成（FL, FR, L, R）
    [lut_fl, sensor_values_fl] = create_lut_from_data(wall_fl, sum_len, lut_size);
    [lut_fr, sensor_values_fr] = create_lut_from_data(wall_fr, sum_len, lut_size);
    [lut_l, sensor_values_l] = create_lut_from_data(wall_l, sum_len, lut_size);
    [lut_r, sensor_values_r] = create_lut_from_data(wall_r, sum_len, lut_size);
    
    % C++ヘッダーファイルに出力（4センサ個別版）
    output_cpp_header_individual(lut_fl, sensor_values_fl, ...
                                  lut_fr, sensor_values_fr, ...
                                  lut_l, sensor_values_l, ...
                                  lut_r, sensor_values_r, lut_size);
    
    % MATファイルにも保存（4センサ個別版）
    save('wall_sensor_lut.mat', ...
         'lut_fl', 'sensor_values_fl', ...
         'lut_fr', 'sensor_values_fr', ...
         'lut_l', 'sensor_values_l', ...
         'lut_r', 'sensor_values_r', 'lut_size');
    
    fprintf('LUTをwall_sensor_lut.matに保存しました。\n');
    fprintf('4センサ個別: FL, FR (姿勢角推定用), L, R\n');
end

function [lut_distance, sensor_values] = create_lut_from_combined_data(sensor_fl, sensor_fr, distance_data, lut_size)
    % 前壁センサ(FL+FR)の平均からLUTを生成
    
    % FL と FR の平均を取る
    sensor_data = (sensor_fl + sensor_fr) / 2;
    
    % センサ値の範囲を決定
    min_sensor = min(sensor_data);
    max_sensor = max(sensor_data);
    
    % LUT用のセンサ値を等間隔で生成
    sensor_values = linspace(min_sensor, max_sensor, lut_size);
    lut_distance = zeros(1, lut_size);
    
    % 各センサ値に対応する距離を計算（平均値を使用）
    for i = 1:lut_size
        % 現在のセンサ値に近いデータを抽出
        tolerance = (max_sensor - min_sensor) / (lut_size * 2);
        idx = abs(sensor_data - sensor_values(i)) < tolerance;
        
        if sum(idx) > 0
            % 該当するデータの平均距離を使用
            lut_distance(i) = mean(distance_data(idx));
        else
            % 該当データがない場合は線形補間
            lut_distance(i) = interp1(sensor_data, distance_data, sensor_values(i), 'linear', 'extrap');
        end
    end
    
    % 距離を昇順または降順に整列（センサ特性に応じて）
    [sensor_values, sort_idx] = sort(sensor_values);
    lut_distance = lut_distance(sort_idx);
end

function [lut_distance, sensor_values] = create_lut_from_data(sensor_data, distance_data, lut_size)
    % データからLUTを生成
    
    % センサ値の範囲を決定
    min_sensor = min(sensor_data);
    max_sensor = max(sensor_data);
    
    % LUT用のセンサ値を等間隔で生成
    sensor_values = linspace(min_sensor, max_sensor, lut_size);
    lut_distance = zeros(1, lut_size);
    
    % 各センサ値に対応する距離を計算（平均値を使用）
    for i = 1:lut_size
        % 現在のセンサ値に近いデータを抽出
        tolerance = (max_sensor - min_sensor) / (lut_size * 2);
        idx = abs(sensor_data - sensor_values(i)) < tolerance;
        
        if sum(idx) > 0
            % 該当するデータの平均距離を使用
            lut_distance(i) = mean(distance_data(idx));
        else
            % 該当データがない場合は線形補間
            lut_distance(i) = interp1(sensor_data, distance_data, sensor_values(i), 'linear', 'extrap');
        end
    end
    
    % 距離を昇順または降順に整列（センサ特性に応じて）
    % 通常、センサ値が大きい -> 壁が近い -> 距離が小さい
    [sensor_values, sort_idx] = sort(sensor_values);
    lut_distance = lut_distance(sort_idx);
end

function generate_lut_from_measured_data(wall_fl, wall_l, wall_r, wall_fr, sum_len)
    % 計測データから壁との距離を推定してLUTを生成
    % CalibrateWallSensorDistance()で取得したデータを想定
    
    fprintf('\n--- 計測データからの距離推定 ---\n');
    fprintf('この方法は、ロボットが壁に向かって直進した計測データを使用します。\n');
    fprintf('前提: CalibrateWallSensorDistance()で0.5m×5回の前進データ\n\n');
    
    % データの基本情報
    total_distance = max(sum_len) - min(sum_len);
    num_samples = length(sum_len);
    
    fprintf('計測データ情報:\n');
    fprintf('  総走行距離: %.3f m\n', total_distance);
    fprintf('  サンプル数: %d\n', num_samples);
    fprintf('  開始距離: %.3f m\n', min(sum_len));
    fprintf('  終了距離: %.3f m\n\n', max(sum_len));
    
    % ユーザー入力：初期壁距離の推定
    fprintf('=== 初期条件の設定 ===\n');
    fprintf('ロボットの初期位置での壁との距離を入力してください。\n');
    fprintf('（迷路寸法や実測値から推定）\n\n');
    
    % 前壁との初期距離
    fprintf('[前壁センサ (FL, FR)]\n');
    initial_front_distance = input('初期の前壁距離 [mm]: ') / 1000; % m に変換
    
    % 側壁との距離（通常は一定と仮定）
    fprintf('\n[側壁センサ (L, R)]\n');
    fprintf('側壁との距離は走行中一定と仮定しますか？\n');
    fprintf('1. はい（推奨 - 直進走行の場合）\n');
    fprintf('2. いいえ（距離変化を考慮）\n');
    side_wall_constant = input('選択 (1 or 2): ');
    
    if side_wall_constant == 1
        % 側壁距離は一定
        left_wall_distance = input('左壁との距離 [mm]: ') / 1000;
        right_wall_distance = input('右壁との距離 [mm]: ') / 1000;
        
        % 側壁距離配列を作成（全サンプルで同じ）
        distances_l = ones(num_samples, 1) * left_wall_distance;
        distances_r = ones(num_samples, 1) * right_wall_distance;
    else
        % 側壁距離も変化すると仮定
        fprintf('左壁の初期距離と最終距離を入力してください。\n');
        left_start = input('左壁初期距離 [mm]: ') / 1000;
        left_end = input('左壁最終距離 [mm]: ') / 1000;
        
        fprintf('右壁の初期距離と最終距離を入力してください。\n');
        right_start = input('右壁初期距離 [mm]: ') / 1000;
        right_end = input('右壁最終距離 [mm]: ') / 1000;
        
        % 線形補間で距離を計算
        distances_l = linspace(left_start, left_end, num_samples)';
        distances_r = linspace(right_start, right_end, num_samples)';
    end
    
    % 前壁距離の計算（走行距離に応じて減少）
    % 前壁距離 = 初期距離 - 走行距離
    distances_fl = initial_front_distance - (sum_len - min(sum_len));
    distances_fr = distances_fl; % FL と FR は同じ前壁を見ている
    
    % 負の距離をチェック
    if any(distances_fl < 0)
        fprintf('\n警告: 計算された前壁距離が負になりました。\n');
        fprintf('      初期距離が不足している可能性があります。\n');
        fprintf('      負の値は0にクリップされます。\n\n');
        distances_fl(distances_fl < 0) = 0;
        distances_fr(distances_fr < 0) = 0;
    end
    
    % データの可視化（推定距離との関係）
    visualize_measured_data_with_distance(wall_fl, wall_fr, wall_l, wall_r, ...
                                          distances_fl, distances_fr, distances_l, distances_r, ...
                                          sum_len);
    
    % LUTサイズ
    lut_size = input('\nLUTサイズ (推奨: 20-50): ');
    if isempty(lut_size) || lut_size < 10
        lut_size = 30;
    end
    
    % 各センサのデータから異常値を除去してLUT生成
    fprintf('\n異常値除去と平滑化処理中...\n');
    
    % FL センサ
    [wall_fl_clean, distances_fl_clean] = remove_outliers_and_smooth(wall_fl, distances_fl);
    [lut_fl, sensor_values_fl] = create_lut_from_measured_points(wall_fl_clean, distances_fl_clean, lut_size);
    
    % FR センサ
    [wall_fr_clean, distances_fr_clean] = remove_outliers_and_smooth(wall_fr, distances_fr);
    [lut_fr, sensor_values_fr] = create_lut_from_measured_points(wall_fr_clean, distances_fr_clean, lut_size);
    
    % L センサ
    [wall_l_clean, distances_l_clean] = remove_outliers_and_smooth(wall_l, distances_l);
    [lut_l, sensor_values_l] = create_lut_from_measured_points(wall_l_clean, distances_l_clean, lut_size);
    
    % R センサ
    [wall_r_clean, distances_r_clean] = remove_outliers_and_smooth(wall_r, distances_r);
    [lut_r, sensor_values_r] = create_lut_from_measured_points(wall_r_clean, distances_r_clean, lut_size);
    
    fprintf('LUT生成完了。\n');
    
    % 結果の可視化（生成されたLUT）
    visualize_generated_lut(sensor_values_fl, lut_fl, sensor_values_fr, lut_fr, ...
                            sensor_values_l, lut_l, sensor_values_r, lut_r, ...
                            wall_fl_clean, distances_fl_clean, ...
                            wall_fr_clean, distances_fr_clean, ...
                            wall_l_clean, distances_l_clean, ...
                            wall_r_clean, distances_r_clean);
    
    % C++ヘッダーファイルに出力
    output_cpp_header_individual(lut_fl, sensor_values_fl, ...
                                  lut_fr, sensor_values_fr, ...
                                  lut_l, sensor_values_l, ...
                                  lut_r, sensor_values_r, lut_size);
    
    % MATファイルにも保存
    save('wall_sensor_lut_from_measured.mat', ...
         'lut_fl', 'sensor_values_fl', ...
         'lut_fr', 'sensor_values_fr', ...
         'lut_l', 'sensor_values_l', ...
         'lut_r', 'sensor_values_r', ...
         'lut_size', ...
         'distances_fl_clean', 'distances_fr_clean', ...
         'distances_l_clean', 'distances_r_clean', ...
         'wall_fl_clean', 'wall_fr_clean', ...
         'wall_l_clean', 'wall_r_clean', ...
         'initial_front_distance');
    
    fprintf('\nキャリブレーションデータをwall_sensor_lut_from_measured.matに保存しました。\n');
    fprintf('4センサ個別: FL, FR (姿勢角推定用), L, R\n');
end

function generate_lut_with_known_distances(~, ~, ~, ~)
    % 既知距離でのキャリブレーション（4センサ個別対応）
    
    fprintf('\n--- 既知距離キャリブレーション（4センサ個別） ---\n');
    fprintf('センサ:\n');
    fprintf('  1. 前左センサ (FL) - 距離推定 + 姿勢角推定\n');
    fprintf('  2. 前右センサ (FR) - 距離推定 + 姿勢角推定\n');
    fprintf('  3. 左壁センサ (L)\n');
    fprintf('  4. 右壁センサ (R)\n');
    fprintf('\n壁からの既知距離でセンサ値を入力してください。\n');
    fprintf('（各センサ最低3点、推奨5点以上）\n\n');
    
    % 前左センサ (FL) のキャリブレーション
    fprintf('=== 前左センサ (FL) のキャリブレーション ===\n');
    num_points_fl = input('前左センサのポイント数: ');
    if isempty(num_points_fl) || num_points_fl < 3
        num_points_fl = 5;
    end
    
    distances_fl = zeros(num_points_fl, 1);
    sensor_fl = zeros(num_points_fl, 1);
    
    for i = 1:num_points_fl
        fprintf('\n--- FL ポイント %d/%d ---\n', i, num_points_fl);
        distances_fl(i) = input('前壁からの距離 [mm]: ') / 1000; % m に変換
        sensor_fl(i) = input('前左センサ値 (FL): ');
    end
    
    % 前右センサ (FR) のキャリブレーション
    fprintf('\n=== 前右センサ (FR) のキャリブレーション ===\n');
    num_points_fr = input('前右センサのポイント数: ');
    if isempty(num_points_fr) || num_points_fr < 3
        num_points_fr = 5;
    end
    
    distances_fr = zeros(num_points_fr, 1);
    sensor_fr = zeros(num_points_fr, 1);
    
    for i = 1:num_points_fr
        fprintf('\n--- FR ポイント %d/%d ---\n', i, num_points_fr);
        distances_fr(i) = input('前壁からの距離 [mm]: ') / 1000;
        sensor_fr(i) = input('前右センサ値 (FR): ');
    end
    
    % 左壁センサのキャリブレーション
    fprintf('\n=== 左壁センサ (L) のキャリブレーション ===\n');
    num_points_l = input('左壁センサのポイント数: ');
    if isempty(num_points_l) || num_points_l < 3
        num_points_l = 5;
    end
    
    distances_l = zeros(num_points_l, 1);
    sensor_l = zeros(num_points_l, 1);
    
    for i = 1:num_points_l
        fprintf('\n--- 左壁 ポイント %d/%d ---\n', i, num_points_l);
        distances_l(i) = input('左壁からの距離 [mm]: ') / 1000;
        sensor_l(i) = input('左センサ値 (L): ');
    end
    
    % 右壁センサのキャリブレーション
    fprintf('\n=== 右壁センサ (R) のキャリブレーション ===\n');
    num_points_r = input('右壁センサのポイント数: ');
    if isempty(num_points_r) || num_points_r < 3
        num_points_r = 5;
    end
    
    distances_r = zeros(num_points_r, 1);
    sensor_r = zeros(num_points_r, 1);
    
    for i = 1:num_points_r
        fprintf('\n--- 右壁 ポイント %d/%d ---\n', i, num_points_r);
        distances_r(i) = input('右壁からの距離 [mm]: ') / 1000;
        sensor_r(i) = input('右センサ値 (R): ');
    end
    
    % LUTサイズ
    lut_size = input('\nLUTサイズ (推奨: 20-50): ');
    if isempty(lut_size) || lut_size < 10
        lut_size = 30;
    end
    
    % 4センサ個別にLUTを生成（スプライン補間使用）
    [lut_fl, sensor_values_fl] = create_lut_from_known_points(sensor_fl, distances_fl, lut_size);
    [lut_fr, sensor_values_fr] = create_lut_from_known_points(sensor_fr, distances_fr, lut_size);
    [lut_l, sensor_values_l] = create_lut_from_known_points(sensor_l, distances_l, lut_size);
    [lut_r, sensor_values_r] = create_lut_from_known_points(sensor_r, distances_r, lut_size);
    
    % 結果の可視化（4センサ個別版）
    visualize_calibration_individual(sensor_fl, sensor_fr, sensor_l, sensor_r, ...
                                     distances_fl, distances_fr, distances_l, distances_r, ...
                                     sensor_values_fl, lut_fl, ...
                                     sensor_values_fr, lut_fr, ...
                                     sensor_values_l, lut_l, ...
                                     sensor_values_r, lut_r);
    
    % C++ヘッダーファイルに出力（4センサ個別版）
    output_cpp_header_individual(lut_fl, sensor_values_fl, ...
                                  lut_fr, sensor_values_fr, ...
                                  lut_l, sensor_values_l, ...
                                  lut_r, sensor_values_r, lut_size);
    
    % MATファイルにも保存
    save('wall_sensor_lut_calibrated.mat', ...
         'lut_fl', 'sensor_values_fl', ...
         'lut_fr', 'sensor_values_fr', ...
         'lut_l', 'sensor_values_l', ...
         'lut_r', 'sensor_values_r', ...
         'lut_size', ...
         'distances_fl', 'distances_fr', 'distances_l', 'distances_r', ...
         'sensor_fl', 'sensor_fr', 'sensor_l', 'sensor_r');
    
    fprintf('キャリブレーションデータをwall_sensor_lut_calibrated.matに保存しました。\n');
    fprintf('4センサ個別: FL, FR (姿勢角推定用), L, R\n');
end

function [lut_distance, sensor_values] = create_lut_from_known_points(sensor_points, distance_points, lut_size)
    % 既知点からLUTを生成（スプライン補間）
    
    % センサ値の範囲を決定
    min_sensor = min(sensor_points);
    max_sensor = max(sensor_points);
    
    % LUT用のセンサ値を等間隔で生成
    sensor_values = linspace(min_sensor, max_sensor, lut_size);
    
    % スプライン補間で距離を計算
    lut_distance = interp1(sensor_points, distance_points, sensor_values, 'spline');
    
    % 負の距離を0にクリップ
    lut_distance(lut_distance < 0) = 0;
end

function visualize_calibration_individual(sensor_fl, sensor_fr, sensor_l, sensor_r, ...
                                          distances_fl, distances_fr, distances_l, distances_r, ...
                                          sensor_values_fl, lut_fl, ...
                                          sensor_values_fr, lut_fr, ...
                                          sensor_values_l, lut_l, ...
                                          sensor_values_r, lut_r)
    % キャリブレーション結果の可視化（4センサ個別版）
    
    figure('Name', 'キャリブレーション結果（4センサ個別）', 'Position', [100, 100, 1400, 900]);
    
    % 前左センサ (FL)
    subplot(2, 2, 1);
    plot(sensor_fl, distances_fl * 1000, 'ro', 'MarkerSize', 10, 'LineWidth', 2);
    hold on;
    plot(sensor_values_fl, lut_fl * 1000, 'b-', 'LineWidth', 1.5);
    hold off;
    title('前左センサ (FL)');
    xlabel('センサ値');
    ylabel('距離 [mm]');
    legend({'計測点', 'スプライン補間'}, 'Location', 'best');
    grid on;
    
    % 前右センサ (FR)
    subplot(2, 2, 2);
    plot(sensor_fr, distances_fr * 1000, 'ro', 'MarkerSize', 10, 'LineWidth', 2);
    hold on;
    plot(sensor_values_fr, lut_fr * 1000, 'm-', 'LineWidth', 1.5);
    hold off;
    title('前右センサ (FR)');
    xlabel('センサ値');
    ylabel('距離 [mm]');
    legend({'計測点', 'スプライン補間'}, 'Location', 'best');
    grid on;
    
    % 左センサ (L)
    subplot(2, 2, 3);
    plot(sensor_l, distances_l * 1000, 'ro', 'MarkerSize', 10, 'LineWidth', 2);
    hold on;
    plot(sensor_values_l, lut_l * 1000, 'g-', 'LineWidth', 1.5);
    hold off;
    title('左壁センサ (L)');
    xlabel('センサ値');
    ylabel('距離 [mm]');
    legend({'計測点', 'スプライン補間'}, 'Location', 'best');
    grid on;
    
    % 右センサ (R)
    subplot(2, 2, 4);
    plot(sensor_r, distances_r * 1000, 'ro', 'MarkerSize', 10, 'LineWidth', 2);
    hold on;
    plot(sensor_values_r, lut_r * 1000, 'r-', 'LineWidth', 1.5);
    hold off;
    title('右壁センサ (R)');
    xlabel('センサ値');
    ylabel('距離 [mm]');
    legend({'計測点', 'スプライン補間'}, 'Location', 'best');
    grid on;
end

function output_cpp_header_individual(lut_fl, sensor_values_fl, ...
                                      lut_fr, sensor_values_fr, ...
                                      lut_l, sensor_values_l, ...
                                      lut_r, sensor_values_r, lut_size)
    % C++ヘッダーファイルに出力（4センサ個別版）
    
    filename = 'WallSensorLUT.hpp';
    fid = fopen(filename, 'w');
    
    if fid == -1
        fprintf('エラー: ファイル %s を開けませんでした。\n', filename);
        return;
    end
    
    fprintf('C++ヘッダーファイルを生成中: %s\n', filename);
    
    % ヘッダーガード
    fprintf(fid, '#ifndef WALL_SENSOR_LUT_HPP\n');
    fprintf(fid, '#define WALL_SENSOR_LUT_HPP\n\n');
    
    fprintf(fid, '/**\n');
    fprintf(fid, ' * @file WallSensorLUT.hpp\n');
    fprintf(fid, ' * @brief 壁センサ値から距離への変換用ルックアップテーブル（4センサ個別版）\n');
    fprintf(fid, ' * \n');
    fprintf(fid, ' * センサ構成:\n');
    fprintf(fid, ' *   - FL: 前左センサ（距離推定 + 姿勢角推定）\n');
    fprintf(fid, ' *   - FR: 前右センサ（距離推定 + 姿勢角推定）\n');
    fprintf(fid, ' *   - L:  左壁センサ（距離推定）\n');
    fprintf(fid, ' *   - R:  右壁センサ（距離推定）\n');
    fprintf(fid, ' * \n');
    fprintf(fid, ' * 姿勢角推定: FL と FR の距離差から壁に対する角度ずれを計算可能\n');
    fprintf(fid, ' * \n');
    fprintf(fid, ' * このファイルはMATLABスクリプト generate_wall_distance_lut.m により\n');
    fprintf(fid, ' * 自動生成されました。手動で編集しないでください。\n');
    fprintf(fid, ' * \n');
    fprintf(fid, ' * 生成日時: %s\n', datestr(now, 'yyyy-mm-dd HH:MM:SS'));
    fprintf(fid, ' */\n\n');
    
    fprintf(fid, '#include <stdint.h>\n\n');
    
    % LUTサイズ定数
    fprintf(fid, '// ルックアップテーブルのサイズ\n');
    fprintf(fid, 'constexpr int WALL_SENSOR_LUT_SIZE = %d;\n\n', lut_size);
    
    % センサ値範囲の定数
    fprintf(fid, '// センサ値の範囲\n');
    fprintf(fid, 'constexpr uint16_t SENSOR_FL_MIN = %d;  // 前左センサ\n', uint16(min(sensor_values_fl)));
    fprintf(fid, 'constexpr uint16_t SENSOR_FL_MAX = %d;\n', uint16(max(sensor_values_fl)));
    fprintf(fid, 'constexpr uint16_t SENSOR_FR_MIN = %d;  // 前右センサ\n', uint16(min(sensor_values_fr)));
    fprintf(fid, 'constexpr uint16_t SENSOR_FR_MAX = %d;\n', uint16(max(sensor_values_fr)));
    fprintf(fid, 'constexpr uint16_t SENSOR_L_MIN = %d;   // 左壁センサ\n', uint16(min(sensor_values_l)));
    fprintf(fid, 'constexpr uint16_t SENSOR_L_MAX = %d;\n', uint16(max(sensor_values_l)));
    fprintf(fid, 'constexpr uint16_t SENSOR_R_MIN = %d;   // 右壁センサ\n', uint16(min(sensor_values_r)));
    fprintf(fid, 'constexpr uint16_t SENSOR_R_MAX = %d;\n\n', uint16(max(sensor_values_r)));
    
    % LUT配列（センサ値）
    fprintf(fid, '// センサ値の配列（LUTのインデックス用）\n');
    write_array(fid, 'sensor_values_fl', sensor_values_fl, 'uint16_t');
    write_array(fid, 'sensor_values_fr', sensor_values_fr, 'uint16_t');
    write_array(fid, 'sensor_values_l', sensor_values_l, 'uint16_t');
    write_array(fid, 'sensor_values_r', sensor_values_r, 'uint16_t');
    
    % LUT配列（距離）
    fprintf(fid, '\n// 距離の配列 [m]\n');
    write_array(fid, 'lut_distance_fl', lut_fl, 'float');
    write_array(fid, 'lut_distance_fr', lut_fr, 'float');
    write_array(fid, 'lut_distance_l', lut_l, 'float');
    write_array(fid, 'lut_distance_r', lut_r, 'float');
    
    fprintf(fid, '\n#endif // WALL_SENSOR_LUT_HPP\n');
    
    fclose(fid);
    
    fprintf('C++ヘッダーファイル %s を生成しました。\n', filename);
    fprintf('4センサ個別: FL, FR (姿勢角推定対応), L, R\n');
    fprintf('このファイルを main/include/Motion/ にコピーしてください。\n');
end

function output_cpp_header_grouped(lut_front, sensor_values_front, ...
                                   lut_l, sensor_values_l, ...
                                   lut_r, sensor_values_r, lut_size)
    % C++ヘッダーファイルに出力（3グループ版・旧版・互換性のため残す）
    
    filename = 'WallSensorLUT.hpp';
    fid = fopen(filename, 'w');
    
    if fid == -1
        fprintf('エラー: ファイル %s を開けませんでした。\n', filename);
        return;
    end
    
    fprintf('C++ヘッダーファイルを生成中: %s\n', filename);
    
    % ヘッダーガード
    fprintf(fid, '#ifndef WALL_SENSOR_LUT_HPP\n');
    fprintf(fid, '#define WALL_SENSOR_LUT_HPP\n\n');
    
    fprintf(fid, '/**\n');
    fprintf(fid, ' * @file WallSensorLUT.hpp\n');
    fprintf(fid, ' * @brief 壁センサ値から距離への変換用ルックアップテーブル（3グループ版）\n');
    fprintf(fid, ' * \n');
    fprintf(fid, ' * センサグループ:\n');
    fprintf(fid, ' *   - FRONT: 前壁センサ (FL + FR の平均)\n');
    fprintf(fid, ' *   - LEFT:  左壁センサ (L)\n');
    fprintf(fid, ' *   - RIGHT: 右壁センサ (R)\n');
    fprintf(fid, ' * \n');
    fprintf(fid, ' * このファイルはMATLABスクリプト generate_wall_distance_lut.m により\n');
    fprintf(fid, ' * 自動生成されました。手動で編集しないでください。\n');
    fprintf(fid, ' * \n');
    fprintf(fid, ' * 生成日時: %s\n', datestr(now, 'yyyy-mm-dd HH:MM:SS'));
    fprintf(fid, ' */\n\n');
    
    fprintf(fid, '#include <stdint.h>\n\n');
    
    % LUTサイズ定数
    fprintf(fid, '// ルックアップテーブルのサイズ\n');
    fprintf(fid, 'constexpr int WALL_SENSOR_LUT_SIZE = %d;\n\n', lut_size);
    
    % センサ値範囲の定数
    fprintf(fid, '// センサ値の範囲\n');
    fprintf(fid, 'constexpr uint16_t SENSOR_FRONT_MIN = %d;  // 前壁センサ (FL+FR平均)\n', uint16(min(sensor_values_front)));
    fprintf(fid, 'constexpr uint16_t SENSOR_FRONT_MAX = %d;\n', uint16(max(sensor_values_front)));
    fprintf(fid, 'constexpr uint16_t SENSOR_L_MIN = %d;      // 左壁センサ\n', uint16(min(sensor_values_l)));
    fprintf(fid, 'constexpr uint16_t SENSOR_L_MAX = %d;\n', uint16(max(sensor_values_l)));
    fprintf(fid, 'constexpr uint16_t SENSOR_R_MIN = %d;      // 右壁センサ\n', uint16(min(sensor_values_r)));
    fprintf(fid, 'constexpr uint16_t SENSOR_R_MAX = %d;\n\n', uint16(max(sensor_values_r)));
    
    % LUT配列（センサ値）
    fprintf(fid, '// センサ値の配列（LUTのインデックス用）\n');
    write_array(fid, 'sensor_values_front', sensor_values_front, 'uint16_t');
    write_array(fid, 'sensor_values_l', sensor_values_l, 'uint16_t');
    write_array(fid, 'sensor_values_r', sensor_values_r, 'uint16_t');
    
    % LUT配列（距離）
    fprintf(fid, '\n// 距離の配列 [m]\n');
    write_array(fid, 'lut_distance_front', lut_front, 'float');
    write_array(fid, 'lut_distance_l', lut_l, 'float');
    write_array(fid, 'lut_distance_r', lut_r, 'float');
    
    fprintf(fid, '\n#endif // WALL_SENSOR_LUT_HPP\n');
    
    fclose(fid);
    
    fprintf('C++ヘッダーファイル %s を生成しました。\n', filename);
    fprintf('3グループ: FRONT(前壁), LEFT(左壁), RIGHT(右壁)\n');
    fprintf('このファイルを main/include/Motion/ にコピーしてください。\n');
end

function output_cpp_header(lut_fl, sensor_values_fl, lut_l, sensor_values_l, ...
                            lut_r, sensor_values_r, lut_fr, sensor_values_fr, lut_size)
    % C++ヘッダーファイルに出力（旧版・互換性のため残す）
    
    filename = 'WallSensorLUT.hpp';
    fid = fopen(filename, 'w');
    
    if fid == -1
        fprintf('エラー: ファイル %s を開けませんでした。\n', filename);
        return;
    end
    
    fprintf('C++ヘッダーファイルを生成中: %s\n', filename);
    
    % ヘッダーガード
    fprintf(fid, '#ifndef WALL_SENSOR_LUT_HPP\n');
    fprintf(fid, '#define WALL_SENSOR_LUT_HPP\n\n');
    
    fprintf(fid, '/**\n');
    fprintf(fid, ' * @file WallSensorLUT.hpp\n');
    fprintf(fid, ' * @brief 壁センサ値から距離への変換用ルックアップテーブル\n');
    fprintf(fid, ' * \n');
    fprintf(fid, ' * このファイルはMATLABスクリプト generate_wall_distance_lut.m により\n');
    fprintf(fid, ' * 自動生成されました。手動で編集しないでください。\n');
    fprintf(fid, ' * \n');
    fprintf(fid, ' * 生成日時: %s\n', datestr(now, 'yyyy-mm-dd HH:MM:SS'));
    fprintf(fid, ' */\n\n');
    
    fprintf(fid, '#include <stdint.h>\n\n');
    
    % LUTサイズ定数
    fprintf(fid, '// ルックアップテーブルのサイズ\n');
    fprintf(fid, 'constexpr int WALL_SENSOR_LUT_SIZE = %d;\n\n', lut_size);
    
    % センサ値範囲の定数
    fprintf(fid, '// センサ値の範囲\n');
    fprintf(fid, 'constexpr uint16_t SENSOR_FL_MIN = %d;\n', uint16(min(sensor_values_fl)));
    fprintf(fid, 'constexpr uint16_t SENSOR_FL_MAX = %d;\n', uint16(max(sensor_values_fl)));
    fprintf(fid, 'constexpr uint16_t SENSOR_L_MIN = %d;\n', uint16(min(sensor_values_l)));
    fprintf(fid, 'constexpr uint16_t SENSOR_L_MAX = %d;\n', uint16(max(sensor_values_l)));
    fprintf(fid, 'constexpr uint16_t SENSOR_R_MIN = %d;\n', uint16(min(sensor_values_r)));
    fprintf(fid, 'constexpr uint16_t SENSOR_R_MAX = %d;\n', uint16(max(sensor_values_r)));
    fprintf(fid, 'constexpr uint16_t SENSOR_FR_MIN = %d;\n', uint16(min(sensor_values_fr)));
    fprintf(fid, 'constexpr uint16_t SENSOR_FR_MAX = %d;\n\n', uint16(max(sensor_values_fr)));
    
    % LUT配列（センサ値）
    fprintf(fid, '// センサ値の配列（LUTのインデックス用）\n');
    write_array(fid, 'sensor_values_fl', sensor_values_fl, 'uint16_t');
    write_array(fid, 'sensor_values_l', sensor_values_l, 'uint16_t');
    write_array(fid, 'sensor_values_r', sensor_values_r, 'uint16_t');
    write_array(fid, 'sensor_values_fr', sensor_values_fr, 'uint16_t');
    
    % LUT配列（距離）
    fprintf(fid, '\n// 距離の配列 [m]\n');
    write_array(fid, 'lut_distance_fl', lut_fl, 'float');
    write_array(fid, 'lut_distance_l', lut_l, 'float');
    write_array(fid, 'lut_distance_r', lut_r, 'float');
    write_array(fid, 'lut_distance_fr', lut_fr, 'float');
    
    fprintf(fid, '\n#endif // WALL_SENSOR_LUT_HPP\n');
    
    fclose(fid);
    
    fprintf('C++ヘッダーファイル %s を生成しました。\n', filename);
    fprintf('このファイルを main/include/Motion/ にコピーしてください。\n');
end

function [sensor_clean, distance_clean] = remove_outliers_and_smooth(sensor_data, distance_data)
    % 異常値除去と平滑化
    
    % 1. 距離が負または異常に大きい値を除去
    valid_idx = distance_data >= 0 & distance_data <= 1.0; % 最大1m
    sensor_clean = sensor_data(valid_idx);
    distance_clean = distance_data(valid_idx);
    
    % 2. センサ値が異常な値を除去（0または極端に大きい値）
    valid_idx = sensor_clean > 0 & sensor_clean < 4096;
    sensor_clean = sensor_clean(valid_idx);
    distance_clean = distance_clean(valid_idx);
    
    % 3. 移動平均でノイズを除去
    if length(sensor_clean) > 20
        window_size = 10;
        sensor_clean = movmean(sensor_clean, window_size);
        distance_clean = movmean(distance_clean, window_size);
    end
    
    fprintf('  異常値除去: %d -> %d サンプル\n', length(sensor_data), length(sensor_clean));
end

function [lut_distance, sensor_values] = create_lut_from_measured_points(sensor_data, distance_data, lut_size)
    % 計測データからLUTを生成（スプライン補間）
    
    % データをセンサ値でソート
    [sensor_sorted, sort_idx] = sort(sensor_data);
    distance_sorted = distance_data(sort_idx);
    
    % 重複するセンサ値を平均化
    [sensor_unique, ~, ic] = unique(sensor_sorted);
    distance_unique = accumarray(ic, distance_sorted, [], @mean);
    
    % センサ値の範囲を決定
    min_sensor = min(sensor_unique);
    max_sensor = max(sensor_unique);
    
    % LUT用のセンサ値を等間隔で生成
    sensor_values = linspace(min_sensor, max_sensor, lut_size);
    
    % 補間方法を選択（データ点数に応じて）
    if length(sensor_unique) > 10
        % データ点が十分ある場合はスプライン補間
        lut_distance = interp1(sensor_unique, distance_unique, sensor_values, 'pchip');
    else
        % データ点が少ない場合は線形補間
        lut_distance = interp1(sensor_unique, distance_unique, sensor_values, 'linear', 'extrap');
    end
    
    % 負の距離を0にクリップ
    lut_distance(lut_distance < 0) = 0;
end

function visualize_measured_data_with_distance(wall_fl, wall_fr, wall_l, wall_r, ...
                                               distances_fl, distances_fr, distances_l, distances_r, ...
                                               sum_len)
    % 計測データと推定距離の可視化
    
    figure('Name', '計測データと推定距離', 'Position', [100, 100, 1400, 900]);
    
    % FL センサ
    subplot(2, 4, 1);
    plot(sum_len, wall_fl, 'b-', 'LineWidth', 1.5);
    title('前左センサ (FL) vs 走行距離');
    xlabel('走行距離 [m]');
    ylabel('センサ値');
    grid on;
    
    subplot(2, 4, 5);
    scatter(distances_fl * 1000, wall_fl, 10, 'b', 'filled');
    title('前左センサ (FL) vs 推定壁距離');
    xlabel('推定壁距離 [mm]');
    ylabel('センサ値');
    grid on;
    
    % FR センサ
    subplot(2, 4, 2);
    plot(sum_len, wall_fr, 'm-', 'LineWidth', 1.5);
    title('前右センサ (FR) vs 走行距離');
    xlabel('走行距離 [m]');
    ylabel('センサ値');
    grid on;
    
    subplot(2, 4, 6);
    scatter(distances_fr * 1000, wall_fr, 10, 'm', 'filled');
    title('前右センサ (FR) vs 推定壁距離');
    xlabel('推定壁距離 [mm]');
    ylabel('センサ値');
    grid on;
    
    % L センサ
    subplot(2, 4, 3);
    plot(sum_len, wall_l, 'g-', 'LineWidth', 1.5);
    title('左センサ (L) vs 走行距離');
    xlabel('走行距離 [m]');
    ylabel('センサ値');
    grid on;
    
    subplot(2, 4, 7);
    scatter(distances_l * 1000, wall_l, 10, 'g', 'filled');
    title('左センサ (L) vs 推定壁距離');
    xlabel('推定壁距離 [mm]');
    ylabel('センサ値');
    grid on;
    
    % R センサ
    subplot(2, 4, 4);
    plot(sum_len, wall_r, 'r-', 'LineWidth', 1.5);
    title('右センサ (R) vs 走行距離');
    xlabel('走行距離 [m]');
    ylabel('センサ値');
    grid on;
    
    subplot(2, 4, 8);
    scatter(distances_r * 1000, wall_r, 10, 'r', 'filled');
    title('右センサ (R) vs 推定壁距離');
    xlabel('推定壁距離 [mm]');
    ylabel('センサ値');
    grid on;
end

function visualize_generated_lut(sensor_values_fl, lut_fl, sensor_values_fr, lut_fr, ...
                                 sensor_values_l, lut_l, sensor_values_r, lut_r, ...
                                 wall_fl_clean, distances_fl_clean, ...
                                 wall_fr_clean, distances_fr_clean, ...
                                 wall_l_clean, distances_l_clean, ...
                                 wall_r_clean, distances_r_clean)
    % 生成されたLUTの可視化
    
    figure('Name', '生成されたLUT（4センサ個別）', 'Position', [150, 50, 1400, 900]);
    
    % FL センサ
    subplot(2, 2, 1);
    scatter(wall_fl_clean, distances_fl_clean * 1000, 10, [0.7 0.7 0.7], 'filled');
    hold on;
    plot(sensor_values_fl, lut_fl * 1000, 'b-', 'LineWidth', 2);
    hold off;
    title('前左センサ (FL) LUT');
    xlabel('センサ値');
    ylabel('距離 [mm]');
    legend({'計測データ', 'LUT (補間)'}, 'Location', 'best');
    grid on;
    
    % FR センサ
    subplot(2, 2, 2);
    scatter(wall_fr_clean, distances_fr_clean * 1000, 10, [0.7 0.7 0.7], 'filled');
    hold on;
    plot(sensor_values_fr, lut_fr * 1000, 'm-', 'LineWidth', 2);
    hold off;
    title('前右センサ (FR) LUT');
    xlabel('センサ値');
    ylabel('距離 [mm]');
    legend({'計測データ', 'LUT (補間)'}, 'Location', 'best');
    grid on;
    
    % L センサ
    subplot(2, 2, 3);
    scatter(wall_l_clean, distances_l_clean * 1000, 10, [0.7 0.7 0.7], 'filled');
    hold on;
    plot(sensor_values_l, lut_l * 1000, 'g-', 'LineWidth', 2);
    hold off;
    title('左センサ (L) LUT');
    xlabel('センサ値');
    ylabel('距離 [mm]');
    legend({'計測データ', 'LUT (補間)'}, 'Location', 'best');
    grid on;
    
    % R センサ
    subplot(2, 2, 4);
    scatter(wall_r_clean, distances_r_clean * 1000, 10, [0.7 0.7 0.7], 'filled');
    hold on;
    plot(sensor_values_r, lut_r * 1000, 'r-', 'LineWidth', 2);
    hold off;
    title('右センサ (R) LUT');
    xlabel('センサ値');
    ylabel('距離 [mm]');
    legend({'計測データ', 'LUT (補間)'}, 'Location', 'best');
    grid on;
end

function write_array(fid, name, data, type)
    % 配列をC++形式で出力
    
    fprintf(fid, 'constexpr %s %s[WALL_SENSOR_LUT_SIZE] = {\n', type, name);
    
    for i = 1:length(data)
        if strcmp(type, 'float')
            fprintf(fid, '    %.6ff', data(i));
        else
            fprintf(fid, '    %d', uint16(data(i)));
        end
        
        if i < length(data)
            fprintf(fid, ',');
        end
        
        if mod(i, 5) == 0 || i == length(data)
            fprintf(fid, '\n');
        end
    end
    
    fprintf(fid, '};\n\n');
end
