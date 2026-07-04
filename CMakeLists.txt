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

# WiFi テレメトリ機能(RMOUSE_WIFI_ENABLE)が無効なときは WiFi 系コンポーネント
# (net=接続/mDNS, webserver=HTTP)をビルドから完全に除外し、esp_wifi/lwip/mdns/
# esp_http_server 等を OFF ビルドへ一切持ち込まない(OFF=従来と完全同一を厳密に保証)。
# EXCLUDE_COMPONENTS は project() より前に確定する必要があり、その時点では
# CONFIG_* を参照できないため、sdkconfig / sdkconfig.defaults を先読みして判定する。
set(_rmouse_wifi_on OFF)
foreach(_cfg "${CMAKE_CURRENT_LIST_DIR}/sdkconfig" "${CMAKE_CURRENT_LIST_DIR}/sdkconfig.defaults")
    if(EXISTS "${_cfg}")
        file(STRINGS "${_cfg}" _hit REGEX "^CONFIG_RMOUSE_WIFI_ENABLE=y$")
        if(_hit)
            set(_rmouse_wifi_on ON)
        endif()
    endif()
endforeach()
if(NOT _rmouse_wifi_on)
    list(APPEND EXCLUDE_COMPONENTS net webserver)
endif()

project(reRoMouse)
