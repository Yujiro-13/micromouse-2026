# platform/webserver : HTTP サーバ(esp_http_server)。GET / でダッシュボード(index.html/app.js)、
#                      GET /api/info で静的デバイス情報 JSON、WS /ws で 20Hz テレメトリを配信。
#
# 本コンポーネントは WiFi 無効時、トップレベル CMakeLists の EXCLUDE_COMPONENTS で
# ビルドから完全に除外される。したがってここでは無条件に登録してよい。
#
# 注意: REQUIRES/PRIV_REQUIRES は early expansion(要件収集) で評価され、その時点では
#       CONFIG_* を参照できない。よって CONFIG による出し分けは行わない
#       (出し分けは上記 EXCLUDE_COMPONENTS とソースの #if で行う)。
idf_component_register(
    SRCS "telemetry.cpp" "http_server.cpp"
    INCLUDE_DIRS "include"
    PRIV_REQUIRES esp_http_server json esp_app_format esp_netif spi_flash esp_timer mouse_core
)

# --- 静的アセット(web/*)を gzip 圧縮してバイナリ埋め込み ---
# 配信時に Content-Encoding: gzip を付けて返す(ブラウザが透過展開する)。
# Flash 肥大(factory 超過リスク, 仕様書§13)対策と Flash 競合回避(NFR-5)のため、
# アセットは実行時の SPIFFS/FAT 読み出しではなく .rodata へ埋め込む。
# シェルのリダイレクトに依存しないよう「build へコピー → その場で gzip」する
# (gzip は -f で上書き、-n で名前/時刻を残さず再現性確保)。要 gzip(Linux 標準)。
# 生成シンボルは埋め込み元ファイル名由来('.'→'_'): 例 _binary_index_html_gz_start /
# _binary_app_js_gz_start。アセット追加時はこのリストへ 1 行足すだけでよい。
set(_web_assets "index.html" "app.js")
set(_web_gz_files "")
foreach(_asset ${_web_assets})
    set(_src "${COMPONENT_DIR}/web/${_asset}")
    set(_gz "${CMAKE_CURRENT_BINARY_DIR}/${_asset}.gz")
    add_custom_command(
        OUTPUT "${_gz}"
        COMMAND "${CMAKE_COMMAND}" -E copy "${_src}" "${CMAKE_CURRENT_BINARY_DIR}/${_asset}"
        COMMAND gzip -9 -f -n "${CMAKE_CURRENT_BINARY_DIR}/${_asset}"
        DEPENDS "${_src}"
        COMMENT "Gzipping web asset: ${_asset} -> ${_asset}.gz"
        VERBATIM
    )
    list(APPEND _web_gz_files "${_gz}")
endforeach()
add_custom_target(rmouse_web_assets DEPENDS ${_web_gz_files})
add_dependencies(${COMPONENT_LIB} rmouse_web_assets)
foreach(_gz ${_web_gz_files})
    target_add_binary_data(${COMPONENT_LIB} "${_gz}" BINARY)
endforeach()
