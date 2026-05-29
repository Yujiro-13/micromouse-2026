function extract_system_id_data()
    % mouse_log_viewer.mで保存された.matファイルからシステム同定用データを抽出
    % ワークスペース変数として保存する

    % ファイル選択
    [filename, pathname] = uigetfile('*.mat', 'ログファイル(.mat)を選択してください');
    if isequal(filename, 0)
        fprintf('ファイル選択がキャンセルされました。\n');
        return;
    end

    % データ読み込み
    filepath = fullfile(pathname, filename);
    data = load(filepath);

    if ~isfield(data, 'data_buffer')
        fprintf('エラー: data_bufferが見つかりません。\n');
        return;
    end

    buffer = data.data_buffer;
    fprintf('データ読み込み完了: %d サンプル\n', size(buffer, 1));

    % データ抽出メニュー
    fprintf('\n抽出するデータを選択してください:\n');
    fprintf('1. 速度データ (vel_current, vel_target, duty_l, duty_r)\n');
    fprintf('2. 角速度データ (ang_vel_current, ang_vel_target, duty_l, duty_r)\n');
    fprintf('3. カスタム選択\n');

    choice = input('選択 (1-3): ');

    switch choice
        case 1
            % 速度システム同定用データ
            vel_current = buffer(:, 6) / 1000;  % mm/s to m/s
            vel_target = buffer(:, 7) / 1000;
            duty_l = buffer(:, 20) / 1000;
            duty_r = buffer(:, 21) / 1000;

            % ワークスペースに保存
            assignin('base', 'vel_current', vel_current);
            assignin('base', 'vel_target', vel_target);
            assignin('base', 'duty_l', duty_l);
            assignin('base', 'duty_r', duty_r);

            fprintf('\n速度データをワークスペースに保存しました:\n');
            fprintf('  vel_current: 現在速度 [m/s]\n');
            fprintf('  vel_target: 目標速度 [m/s]\n');
            fprintf('  duty_l: 左モータDuty比\n');
            fprintf('  duty_r: 右モータDuty比\n');

        case 2
            % 角速度システム同定用データ
            ang_vel_current = buffer(:, 9) / 1000;   % mrad/s to rad/s
            ang_vel_target = buffer(:, 10) / 1000;
            duty_l = buffer(:, 20) / 1000;
            duty_r = buffer(:, 21) / 1000;

            % ワークスペースに保存
            assignin('base', 'ang_vel_current', ang_vel_current);
            assignin('base', 'ang_vel_target', ang_vel_target);
            assignin('base', 'duty_l', duty_l);
            assignin('base', 'duty_r', duty_r);

            fprintf('\n角速度データをワークスペースに保存しました:\n');
            fprintf('  ang_vel_current: 現在角速度 [rad/s]\n');
            fprintf('  ang_vel_target: 目標角速度 [rad/s]\n');
            fprintf('  duty_l: 左モータDuty比\n');
            fprintf('  duty_r: 右モータDuty比\n');

        case 3
            % カスタム選択
            show_data_info();
            columns = input('抽出する列番号をベクトルで入力 [例: [6 7 20 21]]: ');

            if isempty(columns) || any(columns > size(buffer, 2))
                fprintf('エラー: 無効な列番号です。\n');
                return;
            end

            for i = 1:length(columns)
                col = columns(i);
                var_name = sprintf('data_col_%d', col);
                assignin('base', var_name, buffer(:, col));
                fprintf('  %s: 列%d のデータ\n', var_name, col);
            end

        otherwise
            fprintf('無効な選択です。\n');
            return;
    end

    % サンプリング時間の計算（もし利用可能であれば）
    if size(buffer, 2) >= 26
        delta_time = buffer(:, 26); % μs
        avg_sample_time = mean(delta_time) / 1e6; % s
        assignin('base', 'sample_time', avg_sample_time);
        fprintf('  sample_time: 平均サンプリング時間 [%.6f s]\n', avg_sample_time);
    end

    fprintf('\nワークスペース変数として利用可能です。\n');
end

function show_data_info()
    % データ列の情報を表示
    fprintf('\nデータ列の構成:\n');
    fprintf(' 1: wall_fl          2: wall_l           3: wall_r          4: wall_fr\n');
    fprintf(' 5: battery_mV       6: vel_current_mm_s 7: vel_target_mm_s 8: sum_len_mm\n');
    fprintf(' 9: ang_vel_cur_mrad 10: ang_vel_tgt_mrd 11: rad_current_mrd 12: accel_tgt_mm_s2\n');
    fprintf('13: ang_accel_tgt    14: vel_error_mm_s  15: vel_i_error     16: vel_d_error\n');
    fprintf('17: ang_error        18: ang_i_error     19: ang_d_error     20: duty_l_1000\n');
    fprintf('21: duty_r_1000     22: enc_l           23: enc_r           24: len_current_mm\n');
    fprintf('25: len_target_mm   26: delta_time (μs)\n\n');
end