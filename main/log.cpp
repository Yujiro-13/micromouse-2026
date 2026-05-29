#include "include/UI/log.hpp"

#define MAZESIZE_X 32
#define MAZESIZE_Y 32

void Log::ptr_by_sensor(t_sens_data *_sens) { sens = _sens; }

void Log::ptr_by_motion(t_mouse_motion_val *_val) { val = _val; }

void Log::ptr_by_control(t_control *_control) { control = _control; }

void Log::ptr_by_map(t_map *_map) { map = _map; }

void Log::set_device_driver(std::shared_ptr<t_drivers> driver) {}

void Log::ref_by_motion(Adachi &_adachi) {}

void Log::log_print()
{
    const esp_partition_t *partition = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_ANY, "logs");
    if (partition == NULL)
    {
        ESP_LOGE("log", "Partition error");
        return;
    }

    uint32_t mem_offset = 0;
    int16_t data[37];  // 35列 → 37列に拡張（加速度データ2つ追加）
    int64_t run_time = 0;
    int64_t search_time = 0;

    while (1)
    {
        esp_partition_read(partition, mem_offset, data, sizeof(data));
        if (data[4] == -1)
        {
            break;
        }
        
        // 既存データ (0-26)
        // 壁センサ値（0-3）はuint16_tとして解釈（0-65535の範囲を保持）
        printf("%5u,%5u,%5u,%5u,%4d,", (uint16_t)data[0], (uint16_t)data[1], (uint16_t)data[2], (uint16_t)data[3], data[4]);
        printf("%1d,%1d,%1d,%1d,%1d,", data[5], data[6], data[7], data[8], data[9]);
        printf("%1d,%1d,%1d,%1d,%1d,", data[10], data[11], data[12], data[13], data[14]);
        printf("%1d,%1d,%1d,%1d,%1d,", data[15], data[16], data[17], data[18], data[19]);
        printf("%1d,%1d,%1d,%1d,%1d,", data[20], data[21], data[22], data[23], data[24]);
        printf("%1d,%1d,", data[25], data[26]);
        
        // オドメトリデータ (27-34)
        printf("%1d,%1d,%1d,", data[27], data[28], data[29]); // 生センサ x_pos, y_pos, theta
        printf("%1d,%1d,%1d,", data[30], data[31], data[32]); // セル補正 x_pos_corrected, y_pos_corrected, theta_corrected
        printf("%1d,%1d,", data[33], data[34]);              // 真値セル座標 cell_x, cell_y
        
        // 加速度データ (35-36)
        printf("%1d,%1d\n", data[35], data[36]);             // 生の加速度, 移動平均後の加速度

        mem_offset += sizeof(data);
        if (mem_offset >= partition->size)
        {
            break;
        }
    }
    // printf("\n");
    //  std::cout << "Log" << std::endl;
}

void Log::main_task() // Task Number 13
{
    log_print();
    // std::cout << "Log" << std::endl;
}

void Log1::ptr_by_sensor(t_sens_data *_sens) { sens = _sens; }

void Log1::ptr_by_motion(t_mouse_motion_val *_val) { val = _val; }

void Log1::ptr_by_control(t_control *_control) { control = _control; }

void Log1::ptr_by_map(t_map *_map) { map = _map; }

void Log1::set_device_driver(std::shared_ptr<t_drivers> driver) {}

void Log1::ref_by_motion(Adachi &_adachi) {}

void Log1::log_print()
{
    const esp_partition_t *partition = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_ANY, "logs");
    if (partition == NULL)
    {
        ESP_LOGE("log", "Partition error");
        return;
    }

    uint32_t mem_offset = 0;
    int16_t data[10];

    while (1)
    {
        esp_partition_read(partition, mem_offset, data, sizeof(data));
        if (data[4] == -1)
        {
            break;
        }
        printf("%4d,%4d,%4d,%4d,%4d,", data[0], data[1], data[2], data[3], data[4]);
        printf("%1d,%1d,%1d,%1d\n", data[5], data[6], data[7], data[8]);
        mem_offset += sizeof(data);
        if (mem_offset >= partition->size)
        {
            break;
        }
    }
    std::cout << "Log" << std::endl;
}

void Log1::map_print() // Task Number 14
{
    *map = map_read();
    signed char i, j;

    printf("\x1b[0;0H");
    printf("\n\r+");

    for (i = 0; i < MAZESIZE_X; i++)
    {
        switch (map->wall[i][MAZESIZE_X - 1].north)
        {
        case NOWALL:
            printf("\x1b[37m  +");
            break;

        case WALL:
            printf("\x1b[37m--+");
            break;

        case UNKNOWN:
            printf("\x1b[31m--+");
            break;
        default:
            printf("\x1b[33m--+");
            break;
        }
    }

    printf("\n\r");
    for (j = (MAZESIZE_Y - 1); j > -1; j--)
    {
        switch (map->wall[0][j].west)
        {
        case NOWALL:
            printf("\x1b[37m ");
            break;

        case WALL:
            printf("\x1b[37m|");
            break;

        case UNKNOWN:
            printf("\x1b[31m|");
            break;
        default:
            printf("\x1b[33m|");
            break;
        }

        /*if (map->pos.x == j && map->pos.y == i)
        {
            printf("\x1b[32m*\x1b[37m");
        }
        else if (map->GOAL_X == j && map->GOAL_Y == i)
        {
            printf("\x1b[32mG\x1b[37m");
        }
        else
        {
            printf(" ");
        }*/
        for (i = 0; i < MAZESIZE_X; i++)
        {
            switch (map->wall[i][j].east)
            {
            case NOWALL:
                printf("\x1b[37m   ");
                break;

            case WALL:
                printf("\x1b[37m  |");
                break;

            case UNKNOWN:
                printf("\x1b[31m  |");
                break;

            default:
                printf("\x1b[33m  |");
                break;
            }
        }

    printf("\n\r+");
        for (i = 0; i < MAZESIZE_X; i++)
        {
            switch (map->wall[i][j].south)
            {
            case NOWALL:
                printf("\x1b[37m  +");
                break;

            case WALL:
                printf("\x1b[37m--+");
                break;

            case UNKNOWN:
                printf("\x1b[31m--+");
                break;
            default:
                printf("\x1b[33m--+");
                break;
            }
        }
        printf("\n\r");
    }
}

void Log1::map_output_txt()
{
    *map = map_read();
     printf("Maze Size\n");
    for (int y = 0; y < 16; y++)
    {
        for (int x = 0; x < 16; x++)
        {
            printf("%d,", static_cast<int>(map->size[y][x]));
        }
        printf("\n");
    }

    printf("Wall Data (N, E, S, W)\n");
    for (int y = 0; y < 16; y++)
    {
        for (int x = 0; x < 16; x++)
        {
            printf("%d,%d,%d,%d,", 
                   static_cast<int>(map->wall[x][y].north),
                   static_cast<int>(map->wall[x][y].east),
                   static_cast<int>(map->wall[x][y].south),
                   static_cast<int>(map->wall[x][y].west));
        }
        printf("\n");
    }

    printf("Goal,%d,%d\n", static_cast<int>(map->GOAL_X), static_cast<int>(map->GOAL_Y));
}

void Log1::main_task()
{
    // log_print();
    map_output_txt();
    //std::cout << "Log1" << std::endl;
}