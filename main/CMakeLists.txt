# main コンポーネント登録。
# WiFi 関連の初期化・依存はすべて net コンポーネント(wifi_manager)へ集約したため、
# main 側に WiFi 専用の REQUIRES は不要。main は全プロジェクトコンポーネントへ
# 暗黙依存するので、WiFi 有効時は net の公開ヘッダ(wifi_manager.hpp)を参照できる。
idf_component_register(
    SRCS
    "main.cpp"
    )
