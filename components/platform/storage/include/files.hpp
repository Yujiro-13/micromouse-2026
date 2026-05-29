#ifndef FILES_HPP
#define FILES_HPP

#include <stdio.h>
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>

#include "esp_vfs.h"
#include "esp_vfs_fat.h"
#include "esp_system.h"

#include "structs.hpp"

void init_files();
void write_file_pid(FilePidGain *write_gain);
FilePidGain read_file_pid();
void write_file_wall_th(FileWallThreshold *write_th);
FileWallThreshold read_file_wall_th();

void map_write(MazeMap *map);
MazeMap map_read();
void write_file_center_sens_val(FileCenterSensValue *write_val);
FileCenterSensValue read_file_center_sens_val();

void unmount_fat();


#endif // FILES_HPP