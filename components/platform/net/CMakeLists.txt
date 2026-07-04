# platform/net : WiFi(AP/STA/APSTA) 接続管理 + ネット基盤(NVS/netif/event) + mDNS。
#
# 本コンポーネントは WiFi 無効時、トップレベル CMakeLists の EXCLUDE_COMPONENTS で
# ビルドから完全に除外される。したがってここでは無条件に登録してよい。
#
# 注意: REQUIRES/PRIV_REQUIRES は early expansion(要件収集) で評価され、その時点では
#       CONFIG_* を参照できない。よって CONFIG による出し分けは行わない
#       (出し分けは上記 EXCLUDE_COMPONENTS とソースの #if で行う)。
# mdns(managed) は idf_component.yml の条件付き依存で取得する。
idf_component_register(
    SRCS "wifi_manager.cpp"
    INCLUDE_DIRS "include"
    PRIV_REQUIRES nvs_flash esp_wifi esp_netif esp_event
)
