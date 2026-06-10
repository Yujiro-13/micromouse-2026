# platform/webserver : HTTP サーバ(esp_http_server)。GET / で最小ダッシュボードを配信、
#                      GET /api/info で静的デバイス情報 JSON(telemetry)を返す。
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

# --- 静的アセット(web/index.html)を gzip 圧縮してバイナリ埋め込み ---
# 配信時に Content-Encoding: gzip を付けて返す(ブラウザが透過展開する)。
# Flash 肥大(factory 超過リスク, 仕様書§13)対策として、後続フェーズで増える
# JS/CSS も同じパイプラインで圧縮埋め込みできるよう、最小ページで先に確立しておく。
# シェルのリダイレクトに依存しないよう「build へコピー → その場で gzip」する
# (gzip は -f で上書き、-n で名前/時刻を残さず再現性確保)。要 gzip(Linux 標準)。
set(_web_src "${COMPONENT_DIR}/web/index.html")
set(_web_gz "${CMAKE_CURRENT_BINARY_DIR}/index.html.gz")
add_custom_command(
    OUTPUT "${_web_gz}"
    COMMAND "${CMAKE_COMMAND}" -E copy "${_web_src}" "${CMAKE_CURRENT_BINARY_DIR}/index.html"
    COMMAND gzip -9 -f -n "${CMAKE_CURRENT_BINARY_DIR}/index.html"
    DEPENDS "${_web_src}"
    COMMENT "Gzipping web asset: index.html -> index.html.gz"
    VERBATIM
)
add_custom_target(rmouse_web_assets DEPENDS "${_web_gz}")
add_dependencies(${COMPONENT_LIB} rmouse_web_assets)

# gzip 済みファイルを .rodata へ埋め込む(BINARY=末尾 NUL を付けない)。
# 生成シンボル: _binary_index_html_gz_start / _binary_index_html_gz_end
target_add_binary_data(${COMPONENT_LIB} "${_web_gz}" BINARY)
