#include "adachi.hpp"

static Buzzer::buzzer_score_t pc98[] = {{2000, 100}, {1000, 100}};
static Buzzer::buzzer_score_t pc98_2[] = {{1000, 100}, {2000, 100}};

#define MAZESIZE_X 32 // 迷路の大きさ(x方向)
#define MAZESIZE_Y 32 // 迷路の大きさ(y方向)
#define MASK_SEARCH 0x01
#define MASK_SECOND 0x03
#define CONV_SEN2WALL(w) ((w) ? WALL : NOWALL)

void Adachi::init_map(int x, int y)
{
	// 迷路の歩数Mapを初期化する。全体を0xff、引数の座標x,yは0で初期化する

	int i, j;

	for (i = 0; i < MAZESIZE_X; i++) // 迷路の大きさ分ループ(x座標)　
	{
		for (j = 0; j < MAZESIZE_Y; j++) // 迷路の大きさ分ループ(y座標)
		{
			map->size[i][j] = 255; // すべて255で埋める  ex)map[1][1] = 255,map[1][2] = 255, ...map[1][9] = 255,map[2][1] = 255...
		}
	}

	map->size[x][y] = 0; // ゴール座標の歩数を０に設定
}

void Adachi::init_map_all(int x, int y)
{
	// 迷路の歩数Mapを初期化する。全体を0xff、引数の座標x,yは0で初期化する

	int i, j;
	bool all_filed = true; // 全てのマスが埋まっているかどうかのフラグ

	for (i = 0; i < MAZESIZE_X; i++) // 迷路の大きさ分ループ(x座標)　
	{
		for (j = 0; j < MAZESIZE_Y; j++) // 迷路の大きさ分ループ(y座標)
		{
			if (is_unknown(i, j) == true)
			{
				map->size[i][j] = 0;
				all_filed = false;
			}
			else
			{
				map->size[i][j] = 255;
			}
		}
	}

	if (all_filed == true)
	{
		map->size[x][y] = 0; // ゴール座標の歩数を０に設定
	}
}

void Adachi::make_map(int x, int y, int mask) // 歩数マップを作成する
{
	// 座標x,yをゴールとした歩数Mapを作成する。
	// maskの値(MASK_SEARCH or MASK_SECOND)によって、
	// 探索用の歩数Mapを作るか、最短走行の歩数Mapを作るかが切り替わる
	int i, j;
	Bool change_flag; // Map作成終了を見極めるためのフラグ

	if (map->flag == SEARCH)
	{
		init_map(x, y);
	}
	else if (map->flag == ALL_SEARCH)
	{
		init_map_all(x, y);
	}

	do
	{
		change_flag = FALSE;			 // 変更がなかった場合にはループを抜ける
		for (i = 0; i < MAZESIZE_X; i++) // 迷路の大きさ分ループ(x座標)
		{
			for (j = 0; j < MAZESIZE_Y; j++) // 迷路の大きさ分ループ(y座標)
			{
				if (map->size[i][j] == 255) // 255の場合は次へ
				{
					continue;
				}

				if (j < MAZESIZE_Y - 1) // 範囲チェック
				{
					if ((map->wall[i][j].north & mask) == NOWALL) // 壁がなければ(maskの意味はstatic_parametersを参照)
					{
						if (map->size[i][j + 1] == 255) // まだ値が入っていなければ
						{
							map->size[i][j + 1] = map->size[i][j] + 1; // 値を代入
							change_flag = TRUE;						   // 値が更新されたことを示す
						}
					}
				}

				if (i < MAZESIZE_X - 1) // 範囲チェック
				{
					if ((map->wall[i][j].east & mask) == NOWALL) // 壁がなければ
					{
						if (map->size[i + 1][j] == 255) // 値が入っていなければ
						{
							map->size[i + 1][j] = map->size[i][j] + 1; // 値を代入
							change_flag = TRUE;						   // 値が更新されたことを示す
						}
					}
				}

				if (j > 0) // 範囲チェック
				{
					if ((map->wall[i][j].south & mask) == NOWALL) // 壁がなければ
					{
						if (map->size[i][j - 1] == 255) // 値が入っていなければ
						{
							map->size[i][j - 1] = map->size[i][j] + 1; // 値を代入
							change_flag = TRUE;						   // 値が更新されたことを示す
						}
					}
				}

				if (i > 0) // 範囲チェック
				{
					if ((map->wall[i][j].west & mask) == NOWALL) // 壁がなければ
					{
						if (map->size[i - 1][j] == 255) // 値が入っていなければ
						{
							map->size[i - 1][j] = map->size[i][j] + 1; // 値を代入
							change_flag = TRUE;						   // 値が更新されたことを示す
						}
					}
				}
			}
		}

	} while (change_flag == TRUE); // 全体を作り終わるまで待つ
}

void Adachi::set_wall(int x, int y) // 壁情報を記録
{
	// 引数の座標x,yに壁情報を書き込む
	int n_write = 0, s_write = 0, e_write = 0, w_write = 0;

	// 自分の方向に応じて書き込むデータを生成
	// CONV_SEN2WALL()はmacro.hを参照
	switch (map->pos.dir)
	{
	case NORTH: // 北を向いている時

		n_write = CONV_SEN2WALL(sens->wall.exist.fr || sens->wall.exist.fl); // 前壁の有無を判断
		e_write = CONV_SEN2WALL(sens->wall.exist.r);						 // 右壁の有無を判断
		w_write = CONV_SEN2WALL(sens->wall.exist.l);						 // 左壁の有無を判断
		s_write = NOWALL;													 // 後ろは必ず壁がない

		break;

	case EAST: // 東を向いているとき

		e_write = CONV_SEN2WALL(sens->wall.exist.fr || sens->wall.exist.fl); // 前壁の有無を判断
		s_write = CONV_SEN2WALL(sens->wall.exist.r);						 // 右壁の有無を判断
		n_write = CONV_SEN2WALL(sens->wall.exist.l);						 // 左壁の有無を判断
		w_write = NOWALL;													 // 後ろは必ず壁がない

		break;

	case SOUTH: // 南を向いているとき

		s_write = CONV_SEN2WALL(sens->wall.exist.fr || sens->wall.exist.fl); // 前壁の有無を判断
		w_write = CONV_SEN2WALL(sens->wall.exist.r);						 // 右壁の有無を判断
		e_write = CONV_SEN2WALL(sens->wall.exist.l);						 // 左壁の有無を判断
		n_write = NOWALL;													 // 後ろは必ず壁がない

		break;

	case WEST: // 西を向いているとき

		w_write = CONV_SEN2WALL(sens->wall.exist.fr || sens->wall.exist.fl); // 前壁の有無を判断
		n_write = CONV_SEN2WALL(sens->wall.exist.r);						 // 右壁の有無を判断
		s_write = CONV_SEN2WALL(sens->wall.exist.l);						 // 左壁の有無を判断
		e_write = NOWALL;													 // 後ろは必ず壁がない

		break;
	}

	map->wall[x][y].north = n_write; // 実際に壁情報を書き込み
	map->wall[x][y].south = s_write; // 実際に壁情報を書き込み
	map->wall[x][y].east = e_write;	 // 実際に壁情報を書き込み
	map->wall[x][y].west = w_write;	 // 実際に壁情報を書き込み

	// 壁情報が更新されたのでキャッシュを無効化
	map_cache_valid = false;

	if (y < MAZESIZE_Y - 1) // 範囲チェック
	{
		map->wall[x][y + 1].south = n_write; // 反対側から見た壁を書き込み
	}

	if (x < MAZESIZE_X - 1) // 範囲チェック
	{
		map->wall[x + 1][y].west = e_write; // 反対側から見た壁を書き込み
	}

	if (y > 0) // 範囲チェック
	{
		map->wall[x][y - 1].north = s_write; // 反対側から見た壁を書き込み
	}

	if (x > 0) // 範囲チェック
	{
		map->wall[x - 1][y].east = w_write; // 反対側から見た壁を書き込み
	}
	xSemaphoreGive(*on_logging);
	led->set(sens->wall.exist.fl + (sens->wall.exist.l << 1) + (sens->wall.exist.r << 2) + (sens->wall.exist.fr << 3));
}

Bool Adachi::is_unknown(int x, int y) // 指定された区画が未探索か否かを判断する関数 未探索:TRUE　探索済:false
{
	// 座標x,yが未探索区間か否かを調べる

	if ((map->wall[x][y].north == UNKNOWN) || (map->wall[x][y].east == UNKNOWN) || (map->wall[x][y].south == UNKNOWN) || (map->wall[x][y].west == UNKNOWN))
	{				 // どこかの壁情報が不明のままであれば
		return TRUE; // 未探索
	}
	else
	{
		return FALSE; // 探索済
	}
}

int Adachi::get_priority(int x, int y, Direction dir) // そのマスの情報から、優先度を算出する
{
	// 座標x,yと、向いている方角dirから優先度を算出する

	// 未探索が一番優先度が高い.(4)
	// それに加え、自分の向きと、行きたい方向から、
	// 前(2)横(1)後(0)の優先度を付加する。

	int priority; // 優先度を記録する変数

	priority = 0;

	if (map->pos.dir == dir) // 行きたい方向が現在の進行方向と同じ場合
	{
		priority = 2;
	}
	else if (((4 + map->pos.dir - dir) % 4) == 2) // 行きたい方向が現在の進行方向と逆の場合
	{
		priority = 0;
	}
	else // それ以外(左右どちらか)の場合
	{
		priority = 1;
	}

	if (is_unknown(x, y) == TRUE)
	{
		priority += 4; // 未探索の場合優先度をさらに付加
	}

	return priority; // 優先度を返す
}

int Adachi::get_nextdir(int x, int y, int mask, Direction *dir)
{
	// ゴール座標x,yに向かう場合、今どちらに行くべきかを判断する。
	// 探索、最短の切り替えのためのmaskを指定、dirは方角を示す

	// キャッシュチェック
	if (map_cache_valid && cached_goal_x == x && cached_goal_y == y && cached_mask == mask)
	{
		// キャッシュヒット - make_mapを呼ばずに既存のsize mapを使用
	}
	else
	{
		// キャッシュミス - 新しい計算が必要
		make_map_fast(x, y, mask); // 高速版を使用

		// キャッシュを更新
		map_cache_valid = true;
		cached_goal_x = x;
		cached_goal_y = y;
		cached_mask = mask;
	}

	int min_steps, priority, tmp_priority; // 最小の値を探すために使用する変数
	min_steps = 255;						// 最小歩数を255歩(mapがunsigned char型なので)に設定
	priority = 0;						// 優先度の初期値は0

	// maskの意味はstatic_parameter.hを参照
	if ((map->wall[map->pos.x][map->pos.y].north & mask) == NOWALL) // 北に壁がなければ
	{
		tmp_priority = get_priority(map->pos.x, map->pos.y + 1, NORTH); // 優先度を算出
		if (map->size[map->pos.x][map->pos.y + 1] < min_steps)				// 一番歩数が小さい方向を見つける
		{
			min_steps = map->size[map->pos.x][map->pos.y + 1]; // ひとまず北が歩数が小さい事にする
			*dir = NORTH;									// 方向を保存
			priority = tmp_priority; // 優先度を保存
		}
		else if (map->size[map->pos.x][map->pos.y + 1] == min_steps) // 歩数が同じ場合は優先度から判断する
		{
			if (priority < tmp_priority) // 優先度を評価
			{
				*dir = NORTH; // 方向を更新
				priority = tmp_priority; // 優先度を保存
			}
		}
	}

	if ((map->wall[map->pos.x][map->pos.y].east & mask) == NOWALL) // 東に壁がなければ
	{
		tmp_priority = get_priority(map->pos.x + 1, map->pos.y, EAST); // 優先度を算出
		if (map->size[map->pos.x + 1][map->pos.y] < min_steps)			   // 一番歩数が小さい方向を見つける
		{
			min_steps = map->size[map->pos.x + 1][map->pos.y]; // ひとまず東が歩数が小さい事にする
			*dir = EAST;									// 方向を保存
			priority = tmp_priority; // 優先度を保存
		}
		else if (map->size[map->pos.x + 1][map->pos.y] == min_steps) // 歩数が同じ場合、優先度から判断
		{
			if (priority < tmp_priority) // 優先度を評価
			{
				*dir = EAST; // 方向を保存
				priority = tmp_priority; // 優先度を保存
			}
		}
	}

	if ((map->wall[map->pos.x][map->pos.y].south & mask) == NOWALL) // 南に壁がなければ
	{
		tmp_priority = get_priority(map->pos.x, map->pos.y - 1, SOUTH); // 優先度を算出
		if (map->size[map->pos.x][map->pos.y - 1] < min_steps)				// 一番歩数が小さい方向を見つける
		{
			min_steps = map->size[map->pos.x][map->pos.y - 1]; // ひとまず南が歩数が小さい事にする
			*dir = SOUTH;									// 方向を保存
			priority = tmp_priority; // 優先度を保存
		}
		else if (map->size[map->pos.x][map->pos.y - 1] == min_steps) // 歩数が同じ場合、優先度で評価
		{
			if (priority < tmp_priority) // 優先度を評価
			{
				*dir = SOUTH; // 方向を保存
				priority = tmp_priority; // 優先度を保存
			}
		}
	}

	if ((map->wall[map->pos.x][map->pos.y].west & mask) == NOWALL) // 西に壁がなければ
	{
		tmp_priority = get_priority(map->pos.x - 1, map->pos.y, WEST); // 優先度を算出
		if (map->size[map->pos.x - 1][map->pos.y] < min_steps)			   // 一番歩数が小さい方向を見つける
		{
			min_steps = map->size[map->pos.x - 1][map->pos.y]; // 西が歩数が小さい
			*dir = WEST;									// 方向を保存
			priority = tmp_priority; // 優先度を保存
		}
		else if (map->size[map->pos.x - 1][map->pos.y] == min_steps) // 歩数が同じ場合、優先度で評価
		{
			*dir = WEST; // 方向を保存
			priority = tmp_priority; // 優先度を保存
		}
	}

	return ((int)((4 + *dir - map->pos.dir) % 4)); // どっちに向かうべきかを返す。
												   // 演算の意味はmytyedef.h内のenum宣言から。
}

void Adachi::search_adachi2(int gx, int gy)
{

	wall_back_count = 0; // 壁を戻る回数を初期化
	// 引数gx,gyに向かって足立法で迷路を探索する
	Direction glob_nextdir; // 次に向かう方向を記録する変数

	bool hosei_flag = false;

	/*if ((map->pos.x == 0) && (map->pos.y == 0))
	{
		offset2();
	}*/

	switch (get_nextdir(gx, gy, MASK_SEARCH, &glob_nextdir)) // 次に行く方向を戻り値とする関数を呼ぶ
	{
	case FRONT:
		if ((map->pos.x == 0) && (map->pos.y == 0))
		{
			offset2();
		}
		run_half();
		// printf("run_half\n");
		break;

	case RIGHT:
		if ((map->pos.x == 0) && (map->pos.y == 0))
		{
			offset();
		}
		turn_right_2();
		run_half();
		// printf("turn_right\n");
		break;

	case LEFT:
		if ((map->pos.x == 0) && (map->pos.y == 0))
		{
			offset();
		}
		turn_left_2();
		run_half();
		// printf("turn_left\n");
		break;

	case REAR:
		if ((map->pos.x == 0) && (map->pos.y == 0))
		{
			offset();
		}
		turn_half();
		run_half();
		// printf("turn_half\n");
		break;
	}

	map->pos.dir = glob_nextdir; // 方向を更新

	// 向いた方向によって自分の座標を更新する
	switch (map->pos.dir)
	{
	case NORTH:
		map->pos.y++; // 北を向いた時はY座標を増やす
		break;

	case EAST:
		map->pos.x++; // 東を向いた時はX座標を増やす
		break;

	case SOUTH:
		map->pos.y--; // 南を向いた時はY座標を減らす
		break;

	case WEST:
		map->pos.x--; // 西を向いたときはX座標を減らす
		break;
	}
	// printf("map->pos.x = %d, map->pos.y = %d\n", map->pos.x, map->pos.y);

	while ((map->pos.x != gx) || (map->pos.y != gy))
	{ // ゴールするまで繰り返す
		control->start_search_time = esp_timer_get_time();

		set_wall(map->pos.x, map->pos.y); // 壁をセット

		switch (get_nextdir(gx, gy, MASK_SEARCH, &glob_nextdir)) // 次に行く方向を戻り値とする関数を呼ぶ
		{
		case FRONT:
			run2();
			// printf("run\n");
			break;

		case RIGHT: // バグあり
			stop();
			if ((sens->wall.val.fl > 30000) && (sens->wall.val.fr > 30000))
			{
				// bz->play_melody(pc98, 2);
				wall_back_count++;
			}

			if ((wall_back_count % 3 == 0))
			{
				if ((sens->wall.val.fl > 30000) && (sens->wall.val.fr > 30000))
				{ // 正面壁あり、半回転後壁当て
					// stop();
					turn_half();
					back();
					offset();
					turn_right_2();
					if ((sens->wall.val.fl > 30000) && (sens->wall.val.fr > 30000))
					{ // さらに左側壁あり、半回転後壁当て
						turn_half();
						back();
						offset2();
						run_half();
					}
					else
					{
						turn_half();
						run_half();
					}
				}
				else
				{
					turn_right_2();
					run_half();
				}
			}
			else
			{
				// stop();
				turn_right_2();
				run_half();
			}

			// stop2();
			/*
			stop();
			turn_right_2();
			run_half();
			*/

			// printf("turn_right\n");
			break;

		case LEFT:

			stop();
			if ((sens->wall.val.fl > 30000) && (sens->wall.val.fr > 30000))
			{
				// bz->play_melody(pc98, 2);
				wall_back_count++;
			}
			if ((wall_back_count % 3 == 0)) // L字かつ３回毎の場合、壁当て
			{
				if ((sens->wall.val.fl > 30000) && (sens->wall.val.fr > 30000))
				{
					// stop();
					turn_half();
					back();
					offset();
					turn_left_2();
					if ((sens->wall.val.fl > 30000) && (sens->wall.val.fr > 30000))
					{
						turn_half();
						back();
						offset2();
						run_half();
					}
					else
					{
						turn_half();
						run_half();
					}
				}
				else
				{
					turn_left_2();
					run_half();
				}
			}
			else
			{
				// stop();
				turn_left_2();
				run_half();
			}
			// stop2();
			/*
			stop();
			turn_left_2();
			run_half();
			*/
			// printf("turn_left\n");
			break;

		case REAR: // 袋小は、壁当て起きやすくするため閾値低め
			stop();
			turn_right_2();
			if ((sens->wall.val.fl > 30000) && (sens->wall.val.fr > 30000))
			{
				turn_half();
				back();
				offset();
				hosei_flag = true;
			}
			else
			{
				turn_half();
			}

			if ((sens->wall.val.fl > 30000) && (sens->wall.val.fr > 30000) && hosei_flag == false)
			{
				turn_half();
				back();
				offset();
				hosei_flag = true;
				turn_left_2();
			}
			else
			{
				turn_right_2();
			}
			if ((sens->wall.val.fl > 30000) && (sens->wall.val.fr > 30000))
			{
				turn_half();
				back();
				offset2();

				hosei_flag = false;
			}
			else
			{
				turn_half();
			}

			run_half();
			break;
		}

		map->pos.dir = glob_nextdir; // 方向を更新

		// 向いた方向によって自分の座標を更新する
		switch (map->pos.dir)
		{
		case NORTH:
			map->pos.y++; // 北を向いた時はY座標を増やす
			break;

		case EAST:
			map->pos.x++; // 東を向いた時はX座標を増やす
			break;

		case SOUTH:
			map->pos.y--; // 南を向いた時はY座標を減らす
			break;

		case WEST:
			map->pos.x--; // 西を向いたときはX座標を減らす
			break;
		}
		// printf("map->pos.x = %d, map->pos.y = %d\n", map->pos.x, map->pos.y);

		if (map->flag == ALL_SEARCH)
		{
			if (map->search_time > 60000)
			{
				// init_map(gx, gy);
			}
		}

		control->end_search_time = esp_timer_get_time();
		control->delta_search_time = control->end_search_time - control->start_search_time;
	}
	set_wall(map->pos.x, map->pos.y); // 壁をセット

	stop();
	//("stop\n");
	turn_half();
	// printf("turn_half\n");
	map->pos.dir = static_cast<Direction>((map->pos.dir + 6) % 4);
}

void Adachi::search_adachi(int gx, int gy)
{

	bool hosei_flag = false;

	// 引数gx,gyに向かって足立法で迷路を探索する
	Direction glob_nextdir; // 次に向かう方向を記録する変数

	/*if ((map->pos.x == 0) && (map->pos.y == 0))
	{
		offset2();
		// バグあり
		// 内容：一区画ほどすすんでからturnして壁に衝突
		// 起こる場面：ゴールY座標が0の場合
		// offset は停止を想定していないため、offset後にturnを実行しようとすると、止まらずにしばらく進んでからturnしてしまう
		// 停止用offsetの用意

		// 停止用offsetを使用すると、とまってからturnするようになったが、最初に右壁があっても、右旋回を優先してしまう
		// 初期位置の壁がセットされていない or 初期位置は、壁ありとしてみなされていないかも
	}*/

	switch (get_nextdir(gx, gy, MASK_SEARCH, &glob_nextdir)) // 次に行く方向を戻り値とする関数を呼ぶ
	{
	case FRONT:
		if ((map->pos.x == 0) && (map->pos.y == 0))
		{
			offset2();
		}
		run_half();
		// printf("run_half\n");
		break;

	case RIGHT:
		if ((map->pos.x == 0) && (map->pos.y == 0))
		{
			offset();
		}
		turn_right_2();
		run_half();
		// printf("turn_right\n");
		break;

	case LEFT:
		if ((map->pos.x == 0) && (map->pos.y == 0))
		{
			offset();
		}
		turn_left_2();
		run_half();
		// printf("turn_left\n");
		break;

	case REAR:
		if ((map->pos.x == 0) && (map->pos.y == 0))
		{
			offset();
		}
		turn_half();
		run_half();
		// printf("turn_half\n");
		break;
	}

	map->pos.dir = glob_nextdir; // 方向を更新

	// 向いた方向によって自分の座標を更新する
	switch (map->pos.dir)
	{
	case NORTH:
		map->pos.y++; // 北を向いた時はY座標を増やす
		break;

	case EAST:
		map->pos.x++; // 東を向いた時はX座標を増やす
		break;

	case SOUTH:
		map->pos.y--; // 南を向いた時はY座標を減らす
		break;

	case WEST:
		map->pos.x--; // 西を向いたときはX座標を減らす
		break;
	}
	// printf("map->pos.x = %d, map->pos.y = %d\n", map->pos.x, map->pos.y);

	while ((map->pos.x != gx) || (map->pos.y != gy))
	{ // ゴールするまで繰り返す

		set_wall(map->pos.x, map->pos.y); // 壁をセット

		switch (get_nextdir(gx, gy, MASK_SEARCH, &glob_nextdir)) // 次に行く方向を戻り値とする関数を呼ぶ
		{
		case FRONT:
			run();
			// printf("run\n");
			break;

		case RIGHT:
			stop();
			turn_right_2();
			run_half();
			// printf("turn_right\n");
			break;

		case LEFT:
			stop();
			turn_left_2();
			run_half();
			// printf("turn_left\n");
			break;

		case REAR:
			stop();
			turn_right_2();
			if ((sens->wall.val.fl > 30000) && (sens->wall.val.fr > 30000))
			{
				turn_half();
				back();
				offset();
				hosei_flag = true;
			}
			else
			{
				turn_half();
			}

			if ((sens->wall.val.fl > 30000) && (sens->wall.val.fr > 30000) && hosei_flag == false)
			{
				turn_half();
				back();
				offset();
				hosei_flag = true;
				turn_left_2();
			}
			else
			{
				turn_right_2();
			}
			if ((sens->wall.val.fl > 30000) && (sens->wall.val.fr > 30000))
			{
				turn_half();
				back();
				offset2();

				hosei_flag = false;
			}
			else
			{
				turn_half();
			}

			run_half();

			break;
		}

		map->pos.dir = glob_nextdir; // 方向を更新

		// 向いた方向によって自分の座標を更新する
		switch (map->pos.dir)
		{
		case NORTH:
			map->pos.y++; // 北を向いた時はY座標を増やす
			break;

		case EAST:
			map->pos.x++; // 東を向いた時はX座標を増やす
			break;

		case SOUTH:
			map->pos.y--; // 南を向いた時はY座標を減らす
			break;

		case WEST:
			map->pos.x--; // 西を向いたときはX座標を減らす
			break;
		}
		// printf("map->pos.x = %d, map->pos.y = %d\n", map->pos.x, map->pos.y);

		if (map->flag == ALL_SEARCH)
		{
			if (map->search_time > 60000)
			{
				// init_map(gx, gy);
			}
		}
	}
	set_wall(map->pos.x, map->pos.y); // 壁をセット

	stop();
	//("stop\n");
	turn_half();
	// printf("turn_half\n");
	map->pos.dir = static_cast<Direction>((map->pos.dir + 6) % 4);
}

void Adachi::fast_run(int gx, int gy)
{

	// 引数gx,gyに向かって足立法で迷路を探索する
	Direction glob_nextdir; // 次に向かう方向を記録する変数
	uint8_t straight_count = 0;

	// if ((map->pos.x == 0) && (map->pos.y == 0))
	//{
	// offset2();
	//}

	switch (get_nextdir(gx, gy, MASK_SECOND, &glob_nextdir)) // 次に行く方向を戻り値とする関数を呼ぶ
	{
	case FRONT:
		if ((map->pos.x == 0) && (map->pos.y == 0))
		{
			offset2();
		}
		straight_count++;
		// run_half();
		//  printf("run_half\n");
		break;

	case RIGHT:
		if ((map->pos.x == 0) && (map->pos.y == 0))
		{
			offset();
		}
		turn_right_2();
		straight_count = 1;
		// printf("turn_right\n");
		break;

	case LEFT:
		if ((map->pos.x == 0) && (map->pos.y == 0))
		{
			offset();
		}
		turn_left_2();
		straight_count = 1;
		// printf("turn_left\n");
		break;

	case REAR:
		if ((map->pos.x == 0) && (map->pos.y == 0))
		{
			offset();
		}
		turn_half();
		straight_count = 1;
		// printf("turn_half\n");
		break;
	}

	map->pos.dir = glob_nextdir; // 方向を更新

	// 向いた方向によって自分の座標を更新する
	switch (map->pos.dir)
	{
	case NORTH:
		map->pos.y++; // 北を向いた時はY座標を増やす
		break;

	case EAST:
		map->pos.x++; // 東を向いた時はX座標を増やす
		break;

	case SOUTH:
		map->pos.y--; // 南を向いた時はY座標を減らす
		break;

	case WEST:
		map->pos.x--; // 西を向いたときはX座標を減らす
		break;
	}
	// printf("map->pos.x = %d, map->pos.y = %d\n", map->pos.x, map->pos.y);

	while ((map->pos.x != gx) || (map->pos.y != gy))
	{ // ゴールするまで繰り返す

		// set_wall(map->pos.x, map->pos.y); // 壁をセット

		switch (get_nextdir(gx, gy, MASK_SECOND, &glob_nextdir)) // 次に行く方向を戻り値とする関数を呼ぶ
		{
		case FRONT:
			straight_count++;
			// run();
			//  printf("run\n");
			break;

		case RIGHT:
			fast_straight(straight_count);
			turn_right_2();
			straight_count = 1;
			// printf("turn_right\n");
			break;

		case LEFT:
			fast_straight(straight_count);
			turn_left_2();
			straight_count = 1;
			// printf("turn_left\n");
			break;

		case REAR:
			fast_straight(straight_count);
			turn_half();
			straight_count = 1;
			// printf("turn_half\n");
			break;
		}

		map->pos.dir = glob_nextdir; // 方向を更新

		// 向いた方向によって自分の座標を更新する
		switch (map->pos.dir)
		{
		case NORTH:
			map->pos.y++; // 北を向いた時はY座標を増やす
			break;

		case EAST:
			map->pos.x++; // 東を向いた時はX座標を増やす
			break;

		case SOUTH:
			map->pos.y--; // 南を向いた時はY座標を減らす
			break;

		case WEST:
			map->pos.x--; // 西を向いたときはX座標を減らす
			break;
		}
		// printf("map->pos.x = %d, map->pos.y = %d\n", map->pos.x, map->pos.y);
	}
	// set_wall(map->pos.x, map->pos.y); // 壁をセット

	fast_straight(straight_count);
	//("stop\n");
	map->pos.dir = static_cast<Direction>((map->pos.dir + 6) % 4);
	// printf("turn_half\n");
}

void Adachi::search_adachi_sla(int gx, int gy)
{

	// 引数gx,gyに向かって足立法で迷路を探索する
	Direction glob_nextdir; // 次に向かう方向を記録する変数

	bool hosei_flag = false;

	/*if ((map->pos.x == 0) && (map->pos.y == 0))
	{
		offset2();
	}*/

	switch (get_nextdir(gx, gy, MASK_SEARCH, &glob_nextdir)) // 次に行く方向を戻り値とする関数を呼ぶ
	{
	case FRONT:
		if ((map->pos.x == 0) && (map->pos.y == 0))
		{
			offset2();
		}
		run_half();
		// printf("run_half\n");
		break;

	case RIGHT:
		if ((map->pos.x == 0) && (map->pos.y == 0))
		{
			offset();
		}
		turn_right_2();
		run_half();
		// printf("turn_right\n");
		break;

	case LEFT:
		if ((map->pos.x == 0) && (map->pos.y == 0))
		{
			offset();
		}
		turn_left_2();
		run_half();
		// printf("turn_left\n");
		break;

	case REAR:
		if ((map->pos.x == 0) && (map->pos.y == 0))
		{
			offset();
		}
		turn_half();
		run_half();
		// printf("turn_half\n");
		break;
	}

	map->pos.dir = glob_nextdir; // 方向を更新

	// 向いた方向によって自分の座標を更新する
	switch (map->pos.dir)
	{
	case NORTH:
		map->pos.y++; // 北を向いた時はY座標を増やす
		break;

	case EAST:
		map->pos.x++; // 東を向いた時はX座標を増やす
		break;

	case SOUTH:
		map->pos.y--; // 南を向いた時はY座標を減らす
		break;

	case WEST:
		map->pos.x--; // 西を向いたときはX座標を減らす
		break;
	}
	// printf("map->pos.x = %d, map->pos.y = %d\n", map->pos.x, map->pos.y);

	while ((map->pos.x != gx) || (map->pos.y != gy))
	{ // ゴールするまで繰り返す

		set_wall(map->pos.x, map->pos.y); // 壁をセット

		switch (get_nextdir(gx, gy, MASK_SEARCH, &glob_nextdir)) // 次に行く方向を戻り値とする関数を呼ぶ
		{
		case FRONT:
			run2();
			// printf("run\n");
			break;

		case RIGHT:
			//slalom_right();
			//slalom_time(SLA_RIGHT, 90, 85, 90);
			slalom_jerk(SLA_RIGHT, val->slalom_jerk_value, val->slalom_jerk_phase_ms, 9);
			// printf("turn_right\n");
			break;

		case LEFT:
			//slalom_left();
			//slalom_time(SLA_LEFT, 90, 85, 90);
			slalom_jerk(SLA_LEFT, val->slalom_jerk_value, val->slalom_jerk_phase_ms, 9);
    
			// printf("turn_left\n");
			break;

		case REAR:
			stop();
			turn_right_2();
			if ((sens->wall.val.fl > 30000) && (sens->wall.val.fr > 30000))
			{
				turn_half();
				back();
				offset();
				hosei_flag = true;
			}
			else
			{
				turn_half();
			}

			if ((sens->wall.val.fl > 30000) && (sens->wall.val.fr > 30000) && hosei_flag == false)
			{
				turn_half();
				back();
				offset();
				hosei_flag = true;
				turn_left_2();
			}
			else
			{
				turn_right_2();
			}
			if ((sens->wall.val.fl > 30000) && (sens->wall.val.fr > 30000))
			{
				turn_half();
				back();
				offset2();

				hosei_flag = false;
			}
			else
			{
				turn_half();
			}

			run_half();
			break;
		}

		map->pos.dir = glob_nextdir; // 方向を更新

		// 向いた方向によって自分の座標を更新する
		switch (map->pos.dir)
		{
		case NORTH:
			map->pos.y++; // 北を向いた時はY座標を増やす
			break;

		case EAST:
			map->pos.x++; // 東を向いた時はX座標を増やす
			break;

		case SOUTH:
			map->pos.y--; // 南を向いた時はY座標を減らす
			break;

		case WEST:
			map->pos.x--; // 西を向いたときはX座標を減らす
			break;
		}
		// printf("map->pos.x = %d, map->pos.y = %d\n", map->pos.x, map->pos.y);

		/*if (map->flag == ALL_SEARCH)
		{
			if (map->search_time > 60000)
			{
				//init_map(gx, gy);
			}

		}*/
	}
	set_wall(map->pos.x, map->pos.y); // 壁をセット

	stop();
	//("stop\n");
	turn_half();
	// printf("turn_half\n");
	map->pos.dir = static_cast<Direction>((map->pos.dir + 6) % 4);
}

void Adachi::fast_run_sla(int gx, int gy)
{

	// 引数gx,gyに向かって足立法で迷路を探索する
	Direction glob_nextdir; // 次に向かう方向を記録する変数

	/*if ((map->pos.x == 0) && (map->pos.y == 0))
	{
		offset2();
	}*/

	switch (get_nextdir(gx, gy, MASK_SECOND, &glob_nextdir)) // 次に行く方向を戻り値とする関数を呼ぶ
	{
	case FRONT:
		if ((map->pos.x == 0) && (map->pos.y == 0))
		{
			offset2();
		}
		run_half();
		// printf("run_half\n");
		break;

	case RIGHT:
		if ((map->pos.x == 0) && (map->pos.y == 0))
		{
			offset();
		}
		turn_right_2();
		// printf("turn_right\n");
		break;

	case LEFT:
		if ((map->pos.x == 0) && (map->pos.y == 0))
		{
			offset();
		}
		turn_left_2();
		// printf("turn_left\n");
		break;

	case REAR:
		if ((map->pos.x == 0) && (map->pos.y == 0))
		{
			offset();
		}
		turn_half();
		// printf("turn_half\n");
		break;
	}

	map->pos.dir = glob_nextdir; // 方向を更新

	// 向いた方向によって自分の座標を更新する
	switch (map->pos.dir)
	{
	case NORTH:
		map->pos.y++; // 北を向いた時はY座標を増やす
		break;

	case EAST:
		map->pos.x++; // 東を向いた時はX座標を増やす
		break;

	case SOUTH:
		map->pos.y--; // 南を向いた時はY座標を減らす
		break;

	case WEST:
		map->pos.x--; // 西を向いたときはX座標を減らす
		break;
	}
	// printf("map->pos.x = %d, map->pos.y = %d\n", map->pos.x, map->pos.y);

	while ((map->pos.x != gx) || (map->pos.y != gy))
	{ // ゴールするまで繰り返す

		// set_wall(map->pos.x, map->pos.y); // 壁をセット

		switch (get_nextdir(gx, gy, MASK_SECOND, &glob_nextdir)) // 次に行く方向を戻り値とする関数を呼ぶ
		{
		case FRONT:
			run2();
			// printf("run\n");
			break;

		case RIGHT:
			//slalom_time(SLA_RIGHT, 90, 85, 90);
			slalom_jerk(SLA_RIGHT, val->slalom_jerk_value, val->slalom_jerk_phase_ms, 9);
			// printf("turn_right\n");
			break;

		case LEFT:
			//slalom_time(SLA_LEFT, 90, 85, 90);
			slalom_jerk(SLA_LEFT, val->slalom_jerk_value, val->slalom_jerk_phase_ms, 9);
			// printf("turn_left\n");
			break;

		case REAR:
			stop();
			if (sens->wall.exist.fl == TRUE && sens->wall.exist.fr == TRUE && sens->wall.exist.l == TRUE)
			{
				// stop();
				turn_right_2();
				back();
				offset();
				turn_right_2();
				back();
				offset2();
			}
			else if (sens->wall.exist.fl == TRUE && sens->wall.exist.fr == TRUE && sens->wall.exist.r == TRUE)
			{
				// stop();
				turn_left_2();
				back();
				offset();
				turn_left_2();
				back();
				offset2();
			}
			else if (sens->wall.exist.fl == TRUE && sens->wall.exist.fr == TRUE)
			{
				// stop();
				turn_half();
				back();
				offset2();
			}
			else
			{
				// stop();
				turn_half();
			}
			/*stop();
			turn_right_2();
			back();
			offset();
			turn_right_2();
			back();
			offset2();
			*/

			run_half();
			// printf("turn_half\n");
			break;
		}

		map->pos.dir = glob_nextdir; // 方向を更新

		// 向いた方向によって自分の座標を更新する
		switch (map->pos.dir)
		{
		case NORTH:
			map->pos.y++; // 北を向いた時はY座標を増やす
			break;

		case EAST:
			map->pos.x++; // 東を向いた時はX座標を増やす
			break;

		case SOUTH:
			map->pos.y--; // 南を向いた時はY座標を減らす
			break;

		case WEST:
			map->pos.x--; // 西を向いたときはX座標を減らす
			break;
		}
		// printf("map->pos.x = %d, map->pos.y = %d\n", map->pos.x, map->pos.y);

		if (map->flag == ALL_SEARCH)
		{
			if (map->search_time > 60000)
			{
				// init_map(gx, gy);
			}
		}
	}
	// set_wall(map->pos.x, map->pos.y); // 壁をセット

	stop();
	//("stop\n");
	turn_half();
	// printf("turn_half\n");
	map->pos.dir = static_cast<Direction>((map->pos.dir + 6) % 4);
}

void Adachi::fast_run_sla2(int gx, int gy)
{
	// 引数gx,gyに向かって足立法で迷路を探索する
	Direction glob_nextdir; // 次に向かう方向を記録する変数
	uint8_t straight_count = 0;

	/*if ((map->pos.x == 0) && (map->pos.y == 0))
	{
		offset2();
	}*/

	switch (get_nextdir(gx, gy, MASK_SECOND, &glob_nextdir)) // 次に行く方向を戻り値とする関数を呼ぶ
	{
	case FRONT:
		if ((map->pos.x == 0) && (map->pos.y == 0))
		{
			offset2();
		}
		run_half();
		break;

	case RIGHT:
		if ((map->pos.x == 0) && (map->pos.y == 0))
		{
			offset();
		}
		turn_right_2();
		run_half();
		straight_count = 1;
		break;

	case LEFT:
		if ((map->pos.x == 0) && (map->pos.y == 0))
		{
			offset();
		}
		turn_left_2();
		run_half();
		straight_count = 1;
		break;

	case REAR:
		if ((map->pos.x == 0) && (map->pos.y == 0))
		{
			offset();
		}
		turn_half();
		run_half();
		straight_count = 1;
		break;
	}

	map->pos.dir = glob_nextdir; // 方向を更新

	// 向いた方向によって自分の座標を更新する
	switch (map->pos.dir)
	{
	case NORTH:
		map->pos.y++; // 北を向いた時はY座標を増やす
		break;

	case EAST:
		map->pos.x++; // 東を向いた時はX座標を増やす
		break;

	case SOUTH:
		map->pos.y--; // 南を向いた時はY座標を減らす
		break;

	case WEST:
		map->pos.x--; // 西を向いたときはX座標を減らす
		break;
	}
	// printf("map->pos.x = %d, map->pos.y = %d\n", map->pos.x, map->pos.y);

	while ((map->pos.x != gx) || (map->pos.y != gy))
	{ // ゴールするまで繰り返す

		// set_wall(map->pos.x, map->pos.y); // 壁をセット

		switch (get_nextdir(gx, gy, MASK_SECOND, &glob_nextdir)) // 次に行く方向を戻り値とする関数を呼ぶ
		{
		case FRONT:
			straight_count++;
			break;

		case RIGHT:
			fast_straight(straight_count);
			//slalom_right();
			slalom_jerk(SLA_RIGHT, val->slalom_jerk_value, val->slalom_jerk_phase_ms, 9);
			straight_count = 0;
			break;

		case LEFT:
			fast_straight(straight_count);
			//slalom_left();
			slalom_jerk(SLA_LEFT, val->slalom_jerk_value, val->slalom_jerk_phase_ms, 9);
			straight_count = 0;
			break;

		case REAR:
			fast_stop(straight_count);
			if (sens->wall.exist.fl == TRUE && sens->wall.exist.fr == TRUE && sens->wall.exist.l == TRUE)
			{
				// stop();
				turn_right_2();
				back();
				offset();
				turn_right_2();
				back();
				offset2();
			}
			else if (sens->wall.exist.fl == TRUE && sens->wall.exist.fr == TRUE && sens->wall.exist.r == TRUE)
			{
				// stop();
				turn_left_2();
				back();
				offset();
				turn_left_2();
				back();
				offset2();
			}
			else if (sens->wall.exist.fl == TRUE && sens->wall.exist.fr == TRUE)
			{
				// stop();
				turn_half();
				back();
				offset2();
			}
			else
			{
				// stop();
				turn_half();
			}

			// run_half();
			straight_count = 1;
			break;
		}

		map->pos.dir = glob_nextdir; // 方向を更新

		// 向いた方向によって自分の座標を更新する
		switch (map->pos.dir)
		{
		case NORTH:
			map->pos.y++; // 北を向いた時はY座標を増やす
			break;

		case EAST:
			map->pos.x++; // 東を向いた時はX座標を増やす
			break;

		case SOUTH:
			map->pos.y--; // 南を向いた時はY座標を減らす
			break;

		case WEST:
			map->pos.x--; // 西を向いたときはX座標を減らす
			break;
		}
		// printf("map->pos.x = %d, map->pos.y = %d\n", map->pos.x, map->pos.y);
	}
	fast_straight(straight_count);
	stop();
	// set_wall(map->pos.x, map->pos.y); // 壁をセット

	turn_half();

	map->pos.dir = static_cast<Direction>((map->pos.dir + 6) % 4);
}

void Adachi::init_maze()
{
	for (int x = 0; x < MAZESIZE_X; x++)
	{
		for (int y = 0; y < MAZESIZE_Y; y++)
		{
			map->wall[x][y].north = map->wall[x][y].east = map->wall[x][y].south = map->wall[x][y].west = UNKNOWN;
			if (x == 0)
				map->wall[x][y].west = WALL;
			if (x == MAZESIZE_X - 1)
				map->wall[x][y].east = WALL;
			if (y == 0)
				map->wall[x][y].south = WALL;
			if (y == MAZESIZE_Y - 1)
				map->wall[x][y].north = WALL;
		}
	}
	map->wall[0][0].east = map->wall[1][0].west = WALL; // スタート地点は北以外（西、東、南）に壁が存在する

	// キャッシュを初期化
	map_cache_valid = false;
	cached_goal_x = -1;
	cached_goal_y = -1;
	cached_mask = -1;
}

// オリジナル版のmake_map関数（比較用）
void Adachi::make_map_original(int x, int y, int mask)
{
	// 従来のBellman-Ford式実装
	int i, j;
	Bool change_flag;

	if (map->flag == SEARCH)
	{
		init_map(x, y);
	}
	else if (map->flag == ALL_SEARCH)
	{
		init_map_all(x, y);
	}

	do
	{
		change_flag = FALSE;
		for (i = 0; i < MAZESIZE_X; i++)
		{
			for (j = 0; j < MAZESIZE_Y; j++)
			{
				if (map->size[i][j] == 255)
				{
					continue;
				}

				if (j < MAZESIZE_Y - 1)
				{
					if ((map->wall[i][j].north & mask) == NOWALL)
					{
						if (map->size[i][j + 1] == 255)
						{
							map->size[i][j + 1] = map->size[i][j] + 1;
							change_flag = TRUE;
						}
					}
				}

				if (i < MAZESIZE_X - 1)
				{
					if ((map->wall[i][j].east & mask) == NOWALL)
					{
						if (map->size[i + 1][j] == 255)
						{
							map->size[i + 1][j] = map->size[i][j] + 1;
							change_flag = TRUE;
						}
					}
				}

				if (j > 0)
				{
					if ((map->wall[i][j].south & mask) == NOWALL)
					{
						if (map->size[i][j - 1] == 255)
						{
							map->size[i][j - 1] = map->size[i][j] + 1;
							change_flag = TRUE;
						}
					}
				}

				if (i > 0)
				{
					if ((map->wall[i][j].west & mask) == NOWALL)
					{
						if (map->size[i - 1][j] == 255)
						{
							map->size[i - 1][j] = map->size[i][j] + 1;
							change_flag = TRUE;
						}
					}
				}
			}
		}
	} while (change_flag == TRUE);
}

// BFS版の高速make_map実装
void Adachi::make_map_fast(int goal_x, int goal_y, int mask)
{
	static int debug_counter = 0;
	debug_counter++;
	bool debug_print = (debug_counter <= 3); // 最初の3回だけデバッグ出力

	if (debug_print)
	{
		printf("make_map_fast called (count: %d)\n", debug_counter);
	}

	// 初期化（元のinit_mapと同じロジック）
	if (map->flag == SEARCH)
	{
		// 通常の初期化：全て255、ゴールのみ0
		for (int i = 0; i < MAZESIZE_X; i++)
		{
			for (int j = 0; j < MAZESIZE_Y; j++)
			{
				map->size[i][j] = 255;
			}
		}
		map->size[goal_x][goal_y] = 0;
	}
	else if (map->flag == ALL_SEARCH)
	{
		// 全探索用初期化
		bool all_filled = true;
		for (int i = 0; i < MAZESIZE_X; i++)
		{
			for (int j = 0; j < MAZESIZE_Y; j++)
			{
				if (is_unknown(i, j) == true)
				{
					map->size[i][j] = 0;
					all_filled = false;
				}
				else
				{
					map->size[i][j] = 255;
				}
			}
		}
		if (all_filled == true)
		{
			map->size[goal_x][goal_y] = 0;
		}
	}

	// BFSキューを使用した高速実装
	struct Position
	{
		int x, y, step;
	};

	Position queue[MAZESIZE_X * MAZESIZE_Y];
	int queue_front = 0, queue_rear = 0;

	// ゴール地点をキューに追加（既に値が0のものを探す）
	for (int i = 0; i < MAZESIZE_X; i++)
	{
		for (int j = 0; j < MAZESIZE_Y; j++)
		{
			if (map->size[i][j] == 0)
			{
				queue[queue_rear++] = {i, j, 0};
			}
		}
	}

	int iterations = 0;
	int max_iterations = MAZESIZE_X * MAZESIZE_Y * 2; // 安全のための上限

	// BFS実行（4方向チェック）
	while (queue_front < queue_rear && iterations < max_iterations)
	{
		Position current = queue[queue_front++];
		iterations++;

		// 北方向
		if (current.y < MAZESIZE_Y - 1)
		{
			if ((map->wall[current.x][current.y].north & mask) == NOWALL)
			{
				int new_step = current.step + 1;
				if (map->size[current.x][current.y + 1] == 255)
				{
					map->size[current.x][current.y + 1] = new_step;
					queue[queue_rear++] = {current.x, current.y + 1, new_step};
				}
			}
		}

		// 東方向
		if (current.x < MAZESIZE_X - 1)
		{
			if ((map->wall[current.x][current.y].east & mask) == NOWALL)
			{
				int new_step = current.step + 1;
				if (map->size[current.x + 1][current.y] == 255)
				{
					map->size[current.x + 1][current.y] = new_step;
					queue[queue_rear++] = {current.x + 1, current.y, new_step};
				}
			}
		}

		// 南方向
		if (current.y > 0)
		{
			if ((map->wall[current.x][current.y].south & mask) == NOWALL)
			{
				int new_step = current.step + 1;
				if (map->size[current.x][current.y - 1] == 255)
				{
					map->size[current.x][current.y - 1] = new_step;
					queue[queue_rear++] = {current.x, current.y - 1, new_step};
				}
			}
		}

		// 西方向
		if (current.x > 0)
		{
			if ((map->wall[current.x][current.y].west & mask) == NOWALL)
			{
				int new_step = current.step + 1;
				if (map->size[current.x - 1][current.y] == 255)
				{
					map->size[current.x - 1][current.y] = new_step;
					queue[queue_rear++] = {current.x - 1, current.y, new_step};
				}
			}
		}
	}

	if (debug_print)
	{
		printf("BFS iterations: %d, queue processed: %d\n", iterations, queue_front);
		if (iterations >= max_iterations)
		{
			printf("WARNING: BFS hit iteration limit!\n");
		}
	}
}

// オリジナル版のget_nextdir関数（比較用）
int Adachi::get_nextdir_original(int x, int y, int mask, Direction *dir)
{
	int min_steps, priority, tmp_priority;

	make_map_original(x, y, mask); // オリジナル版使用
	min_steps = 255;
	priority = 0;

	if ((map->wall[map->pos.x][map->pos.y].north & mask) == NOWALL)
	{
		tmp_priority = get_priority(map->pos.x, map->pos.y + 1, NORTH);
		if (map->size[map->pos.x][map->pos.y + 1] < min_steps)
		{
			min_steps = map->size[map->pos.x][map->pos.y + 1];
			*dir = NORTH;
			priority = tmp_priority;
		}
		else if (map->size[map->pos.x][map->pos.y + 1] == min_steps)
		{
			if (priority < tmp_priority)
			{
				*dir = NORTH;
				priority = tmp_priority;
			}
		}
	}

	if ((map->wall[map->pos.x][map->pos.y].east & mask) == NOWALL)
	{
		tmp_priority = get_priority(map->pos.x + 1, map->pos.y, EAST);
		if (map->size[map->pos.x + 1][map->pos.y] < min_steps)
		{
			min_steps = map->size[map->pos.x + 1][map->pos.y];
			*dir = EAST;
			priority = tmp_priority;
		}
		else if (map->size[map->pos.x + 1][map->pos.y] == min_steps)
		{
			if (priority < tmp_priority)
			{
				*dir = EAST;
				priority = tmp_priority;
			}
		}
	}

	if ((map->wall[map->pos.x][map->pos.y].south & mask) == NOWALL)
	{
		tmp_priority = get_priority(map->pos.x, map->pos.y - 1, SOUTH);
		if (map->size[map->pos.x][map->pos.y - 1] < min_steps)
		{
			min_steps = map->size[map->pos.x][map->pos.y - 1];
			*dir = SOUTH;
			priority = tmp_priority;
		}
		else if (map->size[map->pos.x][map->pos.y - 1] == min_steps)
		{
			if (priority < tmp_priority)
			{
				*dir = SOUTH;
				priority = tmp_priority;
			}
		}
	}

	if ((map->wall[map->pos.x][map->pos.y].west & mask) == NOWALL)
	{
		tmp_priority = get_priority(map->pos.x - 1, map->pos.y, WEST);
		if (map->size[map->pos.x - 1][map->pos.y] < min_steps)
		{
			min_steps = map->size[map->pos.x - 1][map->pos.y];
			*dir = WEST;
			priority = tmp_priority;
		}
		else if (map->size[map->pos.x - 1][map->pos.y] == min_steps)
		{
			*dir = WEST;
			priority = tmp_priority;
		}
	}

	return ((int)((4 + *dir - map->pos.dir) % 4));
}

// パフォーマンステスト関数（改良版）
void Adachi::benchmark_get_nextdir(int iterations)
{
	printf("=== get_nextdir Performance Benchmark (Improved) ===\n");

	// テスト用の迷路状態を設定
	init_maze();
	map->pos.x = 0;
	map->pos.y = 0;
	map->pos.dir = NORTH;
	map->flag = SEARCH;

	// いくつかの壁をランダムに設定（テスト用）
	for (int i = 0; i < 5; i++)
	{
		for (int j = 0; j < 5; j++)
		{
			if ((i + j) % 3 == 0)
			{
				map->wall[i][j].east = WALL;
				if (i < MAZESIZE_X - 1)
				{
					map->wall[i + 1][j].west = WALL;
				}
			}
			if ((i + j) % 4 == 0)
			{
				map->wall[i][j].north = WALL;
				if (j < MAZESIZE_Y - 1)
				{
					map->wall[i][j + 1].south = WALL;
				}
			}
		}
	}

	Direction dir_old, dir_new;
	int64_t start_time, end_time;

	printf("Testing with %d iterations (pure computation time)...\n", iterations);

	// === オリジナル版のテスト（printf削除版） ===
	start_time = esp_timer_get_time();

	for (int i = 0; i < iterations; i++)
	{
		// 純粋な計算のみ測定
		get_nextdir_original(8, 8, MASK_SEARCH, &dir_old);
		// 位置を少しずつ変えて実際の探索に近い状況をシミュレート
		map->pos.x = (map->pos.x + 1) % 5;
		map->pos.y = (map->pos.y + (i % 3)) % 5;
	}

	end_time = esp_timer_get_time();
	int64_t original_time = end_time - start_time;

	// === 最適化版のテスト（キャッシュ無し） ===
	map_cache_valid = false;
	map->pos.x = 0;
	map->pos.y = 0;

	start_time = esp_timer_get_time();

	for (int i = 0; i < iterations; i++)
	{
		map_cache_valid = false; // キャッシュを無効化して毎回計算
		get_nextdir(8, 8, MASK_SEARCH, &dir_new);
		map->pos.x = (map->pos.x + 1) % 5;
		map->pos.y = (map->pos.y + (i % 3)) % 5;
	}

	end_time = esp_timer_get_time();
	int64_t optimized_time = end_time - start_time;

	// === キャッシュ効果のテスト（同一パラメータ） ===
	map_cache_valid = false;
	map->pos.x = 0;
	map->pos.y = 0;

	start_time = esp_timer_get_time();

	for (int i = 0; i < iterations; i++)
	{
		get_nextdir(8, 8, MASK_SEARCH, &dir_new); // 同じゴールで連続実行
												  // 位置は変えない（キャッシュ効果を確認）
	}

	end_time = esp_timer_get_time();
	int64_t cached_time = end_time - start_time;

	// === より詳細なテスト：make_map単体 ===
	printf("\n--- Detailed Algorithm Comparison ---\n");

	// make_map_original単体
	start_time = esp_timer_get_time();
	for (int i = 0; i < 10; i++)
	{
		make_map_original(8, 8, MASK_SEARCH);
	}
	end_time = esp_timer_get_time();
	int64_t make_map_orig_time = end_time - start_time;

	// make_map_fast単体
	start_time = esp_timer_get_time();
	for (int i = 0; i < 10; i++)
	{
		make_map_fast(8, 8, MASK_SEARCH);
	}
	end_time = esp_timer_get_time();
	int64_t make_map_fast_time = end_time - start_time;

	// === 結果出力（計算完了後に一括出力） ===
	printf("\n=== Results ===\n");
	printf("Original (total): %lld us, avg: %.2f us\n",
		   original_time, (float)original_time / iterations);
	printf("Optimized (total): %lld us, avg: %.2f us\n",
		   optimized_time, (float)optimized_time / iterations);
	printf("Cached (total): %lld us, avg: %.2f us\n",
		   cached_time, (float)cached_time / iterations);

	printf("\n--- Algorithm Core Performance ---\n");
	printf("make_map_original (10 calls): %lld us, avg: %.2f us\n",
		   make_map_orig_time, (float)make_map_orig_time / 10);
	printf("make_map_fast (10 calls): %lld us, avg: %.2f us\n",
		   make_map_fast_time, (float)make_map_fast_time / 10);

	// 改善率計算
	if (original_time > 0)
	{
		float improvement_no_cache = ((float)(original_time - optimized_time) / original_time) * 100;
		float improvement_with_cache = ((float)(original_time - cached_time) / original_time) * 100;
		float algorithm_improvement = ((float)(make_map_orig_time - make_map_fast_time) / make_map_orig_time) * 100;

		printf("\n=== Performance Improvements ===\n");
		printf("Full function improvement (no cache): %.1f%%\n", improvement_no_cache);
		printf("Full function improvement (with cache): %.1f%%\n", improvement_with_cache);
		printf("Core algorithm improvement: %.1f%%\n", algorithm_improvement);
		printf("Speedup factor (algorithm): %.2fx\n", (float)make_map_orig_time / make_map_fast_time);
	}

	// 1ms目標チェック
	float avg_original_ms = (float)original_time / iterations / 1000.0;
	float avg_optimized_ms = (float)optimized_time / iterations / 1000.0;
	float avg_cached_ms = (float)cached_time / iterations / 1000.0;

	printf("\n=== Target Achievement (1ms goal) ===\n");
	printf("Original: %.3f ms %s\n", avg_original_ms,
		   (avg_original_ms <= 1.0) ? "(PASS)" : "(FAIL)");
	printf("Optimized: %.3f ms %s\n", avg_optimized_ms,
		   (avg_optimized_ms <= 1.0) ? "(PASS)" : "(FAIL)");
	printf("Cached: %.3f ms %s\n", avg_cached_ms,
		   (avg_cached_ms <= 1.0) ? "(PASS)" : "(FAIL)");
}

void Adachi::performance_test()
{
	printf("Starting Adachi Algorithm Performance Test...\n");

	// まず軽量テストで問題を切り分け
	printf("\n=== Lightweight Core Algorithm Test ===\n");

	init_maze();
	map->pos.x = 5;
	map->pos.y = 5;
	map->pos.dir = NORTH;
	map->flag = SEARCH;

	// 単純な迷路環境を設定
	for (int i = 0; i < 10; i++)
	{
		for (int j = 0; j < 10; j++)
		{
			map->wall[i][j].north = NOWALL;
			map->wall[i][j].east = NOWALL;
			map->wall[i][j].south = NOWALL;
			map->wall[i][j].west = NOWALL;
		}
	}

	int64_t start_time, end_time;
	Direction dir;

	// === make_map関数単体の比較（最も重要） ===
	printf("Testing make_map functions (10 iterations each)...\n");

	// Original make_map
	start_time = esp_timer_get_time();
	for (int i = 0; i < 10; i++)
	{
		make_map_original(15, 15, MASK_SEARCH);
	}
	end_time = esp_timer_get_time();
	int64_t orig_make_map_time = end_time - start_time;

	// Fast make_map
	start_time = esp_timer_get_time();
	for (int i = 0; i < 10; i++)
	{
		make_map_fast(15, 15, MASK_SEARCH);
	}
	end_time = esp_timer_get_time();
	int64_t fast_make_map_time = end_time - start_time;

	printf("Original make_map: %lld us (avg: %.1f us)\n",
		   orig_make_map_time, (float)orig_make_map_time / 10);
	printf("Fast make_map: %lld us (avg: %.1f us)\n",
		   fast_make_map_time, (float)fast_make_map_time / 10);

	if (orig_make_map_time > 0)
	{
		float improvement = ((float)(orig_make_map_time - fast_make_map_time) / orig_make_map_time) * 100;
		printf("Make_map improvement: %.1f%% (%.2fx faster)\n",
			   improvement, (float)orig_make_map_time / fast_make_map_time);
	}

	// === キャッシュ効果の詳細テスト ===
	printf("\n=== Cache Effect Test ===\n");

	// キャッシュをクリア
	map_cache_valid = false;
	printf("Cache cleared\n");

	// 1回目（キャッシュミス）
	start_time = esp_timer_get_time();
	get_nextdir(15, 15, MASK_SEARCH, &dir);
	end_time = esp_timer_get_time();
	int64_t first_call = end_time - start_time;
	printf("First call (cache miss): %lld us\n", first_call);

	// 2回目（キャッシュヒット期待）
	start_time = esp_timer_get_time();
	get_nextdir(15, 15, MASK_SEARCH, &dir);
	end_time = esp_timer_get_time();
	int64_t second_call = end_time - start_time;
	printf("Second call (cache hit): %lld us\n", second_call);

	// 3回目（確実にキャッシュヒット）
	start_time = esp_timer_get_time();
	get_nextdir(15, 15, MASK_SEARCH, &dir);
	end_time = esp_timer_get_time();
	int64_t third_call = end_time - start_time;
	printf("Third call (cache hit): %lld us\n", third_call);

	float cache_improvement = (float)first_call / second_call;
	printf("Cache effectiveness: %.2fx faster\n", cache_improvement);

	if (cache_improvement > 1.5)
	{
		printf("✓ Cache is working effectively\n");
	}
	else
	{
		printf("⚠ Cache effect is minimal\n");
	}

	// === 問題診断 ===
	printf("\n=== Problem Diagnosis ===\n");

	if (orig_make_map_time == fast_make_map_time)
	{
		printf("WARNING: No difference in make_map performance!\n");
		printf("Possible causes:\n");
		printf("1. Compiler optimization removed differences\n");
		printf("2. Timer resolution too low\n");
		printf("3. Algorithm not actually different\n");
	}

	if (first_call == second_call)
	{
		printf("WARNING: No cache effect detected!\n");
		printf("Cache mechanism may not be working\n");
	}

	// より大きなテストも実行
	printf("\n=== Extended Test ===\n");
	benchmark_get_nextdir(50); // 反復回数を減らして詳細を確認

	printf("Performance test completed.\n");
}
