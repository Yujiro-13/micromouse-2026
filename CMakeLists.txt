# For more information about build system see
# https://docs.espressif.com/projects/esp-idf/en/latest/api-guides/build-system.html
# The following five lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.16)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)

# components/ は機能で3分類している。各グループ配下のサブディレクトリを
# コンポーネントとして探索させる（コンポーネント名はディレクトリ basename のまま）。
#   hardware: 物理デバイスドライバ（センサ・モータ・ピン定義等）
#   platform: ESP32(マイコン)機能（ファイル永続化・通信等）
#   mouse   : マイクロマウスのアルゴリズム/機能
set(EXTRA_COMPONENT_DIRS
    components/hardware
    components/platform
    components/mouse)

project(reRoMouse)
