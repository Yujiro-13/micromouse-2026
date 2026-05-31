#ifndef ADACHI_HPP
#define ADACHI_HPP

#include "motion.hpp"

class Adachi : public Motion
{
public:
    void search_adachi(int gx, int gy);
    void search_adachi2(int gx, int gy);
    void fast_run(int gx, int gy);
    void search_adachi_sla(int gx, int gy);
    void fast_run_sla(int gx, int gy);
    void fast_run_sla2(int gx, int gy);
    void init_maze();
    
    // パフォーマンス検証用関数
    void performance_test();
    void benchmark_get_nextdir(int iterations = 100);

private:
    void init_map(int x, int y);
    void init_map_all(int x, int y);
    void make_map(int x, int y, int mask);
    void make_map_fast(int x, int y, int mask);  // 高速版
    void make_map_original(int x, int y, int mask);  // 旧版（比較用）
    int get_nextdir_original(int x, int y, int mask, Direction *dir); // 旧版（比較用）
    void set_wall(int x, int y);
    Bool is_unknown(int x, int y);
    int get_priority(int x, int y, Direction dir);
    // get_nextdir の N/E/S/W 同型ブロックを集約。
    // 隣接セル(nx,ny)・方位 dir・壁フィールド wall_field を見て min_steps/priority/*out_dir を更新。
    // tie_break_guard=false のとき歩数同値で無条件上書き（西ブロックの現状挙動を厳密再現）。
    void evaluate_direction(int nx, int ny, Direction dir, int wall_field, int mask,
                            int &min_steps, int &priority, Direction *out_dir,
                            bool tie_break_guard);
    int get_nextdir(int x, int y, int mask, Direction *dir);
    uint8_t wall_back_count = 0;
    
    // キャッシュ用変数
    bool map_cache_valid = false;
    int cached_goal_x = -1;
    int cached_goal_y = -1;
    int cached_mask = -1;
};

#endif // ADACHI_HPP