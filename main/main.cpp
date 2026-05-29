#include <iostream>
#include <memory>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "driver/spi_master.h"
#include "driver/i2c.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_vfs.h"
#include "esp_vfs_fat.h"
#include "esp_heap_caps.h"
#include "esp_timer.h"
#include "structs.hpp"
#include "drivers.hpp"
#include "micromouse.hpp"
#include "files.hpp"

// #include "freertos/event_groups.h"
// #include "esp_system.h"
// #include "esp_bt.h"
// #include "esp_gap_ble_api.h"
// #include "esp_gatts_api.h"
// #include "esp_bt_main.h"
// #include "esp_gatt_common_api.h"
// #include "gatts_table_create.hpp"


t_sens_data sens;

// BLE GATT Server の設定 /////////////////////////////////////////////////

// uint8_t x = 0;

// #define GATTS_TABLE_TAG "GATTS_TABLE_DEMO" // ログ出力用のタグ

// // 定数定義 : プロファイル数、アプリケーションインデックス、デバイス名、サービスインスタンス ID
// #define PROFILE_NUM 1                       // プロファイルの数
// #define PROFILE_APP_IDX 0                   // プロファイルのインデックス
// #define ESP_APP_ID 0x55                     // アプリケーションID
// #define SAMPLE_DEVICE_NAME "ESP_GATTS_DEMO" // デバイス名
// #define SVC_INST_ID 0                       // サービスインスタンスID

// /* The max length of characteristic value. When the GATT client performs a write or prepare write operation,
//  *  the data length must be less than GATTS_DEMO_CHAR_VAL_LEN_MAX.
//  *
//  * Characteristic値の最大長。 GATT クライアントが書き込み操作または書き込み準備操作を実行するとき、
//  * データ長は GATTS_DEMO_CHAR_VAL_LEN_MAX 未満である必要があります。
//  */
// // Characteristic値の最大長さ定義とバッファサイズ
// #define GATTS_DEMO_CHAR_VAL_LEN_MAX 500         // Characteristic値の最大長
// #define PREPARE_BUF_MAX_SIZE 1024               // 書き込み準備用バッファの最大サイズ
// #define CHAR_DECLARATION_SIZE (sizeof(uint8_t)) // Characteristic宣言サイズ

// // Advertiseデータの設定フラグ
// #define ADV_CONFIG_FLAG (1 << 0)      // Advertiseデータ設定フラグ
// #define SCAN_RSP_CONFIG_FLAG (1 << 1) // スキャン応答データ設定フラグ

// // Advertise設定完了フラグ
// static uint8_t adv_config_done = 0; // Advertiseデータ設定が完了したかどうかを示す変数

// // Characteristicハンドルのテーブル
// uint16_t heart_rate_handle_table[HRS_IDX_NB]; // Characteristicハンドルを格納する配列

// // 構造体定義：書き込み準備環境
// typedef struct
// {
//     uint8_t *prepare_buf; // 書き込み準備用のバッファ
//     int prepare_len;      // 書き込み準備のデータ長
// } prepare_type_env_t;

// // 書き込み準備環境のインスタンス
// static prepare_type_env_t prepare_write_env;

// // Advertiseデータの定義（生データ形式）
// #define CONFIG_SET_RAW_ADV_DATA
// #ifdef CONFIG_SET_RAW_ADV_DATA
// static uint8_t raw_adv_data[] = {
//     // Advertiseデータ
//     /* flags */
//     0x02, 0x01, 0x06, // フラグ
//     /* tx power*/
//     0x02, 0x0a, 0xeb, // 送信電力
//     /* service uuid */
//     0x03, 0x03, 0xFF, 0x00, // サービスUUID
//     /* device name */
//     0x0f, 0x09, 'E', 'S', 'P', '_', 'G', 'A', 'T', 'T', 'S', '_', 'D', 'E', 'M', 'O' // デバイス名
// };
// // スキャン応答データの定義（生データ形式）
// static uint8_t raw_scan_rsp_data[] = {
//     // スキャン応答データ
//     /* flags */
//     0x02, 0x01, 0x06, // フラグ
//     /* tx power */
//     0x02, 0x0a, 0xeb, // 送信電力
//     /* service uuid */
//     0x03, 0x03, 0xFF, 0x00 // サービスUUID
// };

// #else
// // サービスUUID
// static uint8_t service_uuid[16] = {
//     // 16バイトのUUID（LSB <--> MSB）
//     0xfb,
//     0x34,
//     0x9b,
//     0x5f,
//     0x80,
//     0x00,
//     0x00,
//     0x80,
//     0x00,
//     0x10,
//     0x00,
//     0x00,
//     0xFF,
//     0x00,
//     0x00,
//     0x00,
// };

// // Advertiseデータの構造体
// static esp_ble_adv_data_t adv_data = {
//     .set_scan_rsp = false,                                                // スキャン応答を含めるかどうか
//     .include_name = true,                                                 // デバイス名を含めるかどうか
//     .include_txpower = true,                                              // 送信電力を含めるかどうか
//     .min_interval = 0x0006,                                               // 接続間隔の最小値
//     .max_interval = 0x0010,                                               // 接続間隔の最大値
//     .appearance = 0x00,                                                   // デバイスの外観
//     .manufacturer_len = 0,                                                // メーカー固有データの長さ
//     .p_manufacturer_data = NULL,                                          // メーカー固有データへのポインタ
//     .service_data_len = 0,                                                // サービスデータの長さ
//     .p_service_data = NULL,                                               // サービスデータへのポインタ
//     .service_uuid_len = sizeof(service_uuid),                             // サービスUUIDの長さ
//     .p_service_uuid = service_uuid,                                       // サービスUUIDへのポインタ
//     .flag = (ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT), // Advertiseフラグ
// };

// // スキャン応答データの構造体
// static esp_ble_adv_data_t scan_rsp_data = {
//     .set_scan_rsp = true,                                                 // スキャン応答データ
//     .include_name = true,                                                 // デバイス名を含めるかどうか
//     .include_txpower = true,                                              // 送信電力を含めるかどうか
//     .min_interval = 0x0006,                                               // 接続間隔の最小値
//     .max_interval = 0x0010,                                               // 接続間隔の最大値
//     .appearance = 0x00,                                                   // デバイスの外観
//     .manufacturer_len = 0,                                                // メーカー固有データの長さ
//     .p_manufacturer_data = NULL,                                          // メーカー固有データへのポインタ
//     .service_data_len = 0,                                                // サービスデータの長さ
//     .p_service_data = NULL,                                               // サービスデータへのポインタ
//     .service_uuid_len = sizeof(service_uuid),                             // サービスUUIDの長さ
//     .p_service_uuid = service_uuid,                                       // サービスUUIDへのポインタ
//     .flag = (ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT), // Advertiseフラグ
// };
// #endif /* CONFIG_SET_RAW_ADV_DATA */

// // Advertiseパラメータの定義
// static esp_ble_adv_params_t adv_params = {
//     .adv_int_min = 0x20,                                    // Advertise間隔の最小値
//     .adv_int_max = 0x40,                                    // Advertise間隔の最大値
//     .adv_type = ADV_TYPE_IND,                               // Advertiseタイプ
//     .own_addr_type = BLE_ADDR_TYPE_PUBLIC,                  // 自身のアドレスタイプ
//     .channel_map = ADV_CHNL_ALL,                            // 使用するチャネルマップ
//     .adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY, // Advertiseフィルターポリシー
// };

// // GATTプロファイル構造体
// struct gatts_profile_inst
// {
//     esp_gatts_cb_t gatts_cb;       // GATTコールバック関数
//     uint16_t gatts_if;             // GATTインターフェースID
//     uint16_t app_id;               // アプリケーションID
//     uint16_t conn_id;              // 接続ID
//     uint16_t service_handle;       // サービスハンドル
//     esp_gatt_srvc_id_t service_id; // サービスID
//     uint16_t char_handle;          // Characteristicハンドル
//     esp_bt_uuid_t char_uuid;       // CharacteristicUUID
//     esp_gatt_perm_t perm;          // 権限
//     esp_gatt_char_prop_t property; // Characteristicプロパティ
//     uint16_t descr_handle;         // 記述子ハンドル
//     esp_bt_uuid_t descr_uuid;      // 記述子UUID
// };

// esp_gatt_if_t current_gatts_if; // 現在の GATT インターフェースを保存するための変数
// uint16_t current_conn_id = 0;       // 現在の接続IDを保存
// // 通知タスクのハンドル
// TaskHandle_t notify_task_handle = NULL;
// static bool notify_enabled = false; // 通知が有効かどうかを示すフラグ

// // GATTプロファイルのイベントハンドラー関数の宣言
// static void gatts_profile_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param);
// void notify_task(void *pvparam); // 通知タスクの関数の宣言

// // GATTプロファイルのインスタンスを定義
// static struct gatts_profile_inst heart_rate_profile_tab[PROFILE_NUM] = {
//     [PROFILE_APP_IDX] = {
//         .gatts_cb = gatts_profile_event_handler, // GATTイベントハンドラー
//         .gatts_if = ESP_GATT_IF_NONE,            // インターフェースは未割り当て
//     },
// };

// /* GATT標準のUUIDを定義 */
// static const uint16_t primary_service_uuid = ESP_GATT_UUID_PRI_SERVICE;                // プライマリサービスUUID
// static const uint16_t character_declaration_uuid = ESP_GATT_UUID_CHAR_DECLARE;         // Characteristic宣言UUID
// static const uint16_t character_client_config_uuid = ESP_GATT_UUID_CHAR_CLIENT_CONFIG; // クライアント設定UUID

// /* Characteristicのプロパティ（読み取り、書き込み、通知など）を定義 */
// // static const uint8_t char_prop_read = ESP_GATT_CHAR_PROP_BIT_READ;                                                                             // 読み取り可能プロパティ
// // static const uint8_t char_prop_write = ESP_GATT_CHAR_PROP_BIT_WRITE;                                                                           // 書き込み可能プロパティ
// static const uint8_t char_prop_read_write_notify = ESP_GATT_CHAR_PROP_BIT_WRITE | ESP_GATT_CHAR_PROP_BIT_READ | ESP_GATT_CHAR_PROP_BIT_NOTIFY; // 読み取り、書き込み、通知可能プロパティ


// typedef struct
// {
//     bool notify_enable_A;
//     bool notify_enable_B;
//     bool notify_enable_C;
//     bool notify_enable_D;
//     bool notify_enable_E;
//     bool notify_enable_F;
//     bool notify_enable_G;
//     bool notify_enable_H;
//     bool notify_enable_I;
//     bool notify_enable_J;
// } bool_notify_enable_t;

// enum
// {
//     CHAR_A,
//     CHAR_B,
//     CHAR_C,
//     CHAR_D,
//     CHAR_E,
//     CHAR_F,
//     CHAR_G,
//     CHAR_H,
//     CHAR_I,
//     CHAR_J,
// };

// static esp_gatts_attr_db_t gatt_db[HRS_IDX_NB] = {
//     [IDX_SVC] =
//         {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&primary_service_uuid, ESP_GATT_PERM_READ, sizeof(uint16_t), sizeof(GATTS_SERVICE_UUID_TEST), (uint8_t *)&GATTS_SERVICE_UUID_TEST}},
// };

// void populate_gatt_db()
// {
//     for (int i = 0; i < sizeof(char_configs) / sizeof(char_configs[0]); i++)
//     {
//         const gatt_char_config_t *config = &char_configs[i];

//         // CharacteristicAの初期化
//         gatt_db[IDX_CHAR_A + i * 3] = (esp_gatts_attr_db_t){
//             .attr_control = {ESP_GATT_AUTO_RSP},
//             .att_desc = {
//                 .uuid_length = ESP_UUID_LEN_16,
//                 .uuid_p = (uint8_t *)&character_declaration_uuid,
//                 .perm = ESP_GATT_PERM_READ,
//                 .max_length = CHAR_DECLARATION_SIZE,
//                 .length = CHAR_DECLARATION_SIZE,
//                 .value = (uint8_t *)&char_prop_read_write_notify}};

//         // CharacteristicAの値の初期化
//         gatt_db[IDX_CHAR_VAL_A + i * 3] = (esp_gatts_attr_db_t){
//             .attr_control = {ESP_GATT_AUTO_RSP},
//             .att_desc = {
//                 .uuid_length = ESP_UUID_LEN_16,
//                 .uuid_p = (uint8_t *)&config->char_uuid,
//                 .perm = config->perm,
//                 .max_length = config->max_len,
//                 .length = config->max_len,
//                 .value = (uint8_t *)config->value}};

//         // CharacteristicAのクライアント設定の初期化
//         gatt_db[IDX_CHAR_CFG_A + i * 3] = (esp_gatts_attr_db_t){
//             .attr_control = {ESP_GATT_AUTO_RSP},
//             .att_desc = {
//                 .uuid_length = ESP_UUID_LEN_16,
//                 .uuid_p = (uint8_t *)&character_client_config_uuid,
//                 .perm = config->cfg_perm,
//                 .max_length = sizeof(uint16_t),
//                 .length = sizeof(uint16_t),
//                 .value = (uint8_t *)config->cfg_val}};
//     }
// }

// /* GAPイベントハンドラ */
// static void gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param)
// {
//     switch (event)
//     {
// #ifdef CONFIG_SET_RAW_ADV_DATA
//     // 生のAdvertiseデータ設定完了イベント
//     case ESP_GAP_BLE_ADV_DATA_RAW_SET_COMPLETE_EVT:
//         adv_config_done &= (~ADV_CONFIG_FLAG);
//         if (adv_config_done == 0)
//         {
//             esp_ble_gap_start_advertising(&adv_params); // Advertiseを開始
//         }
//         break;
//     // 生のスキャン応答データ設定完了イベント
//     case ESP_GAP_BLE_SCAN_RSP_DATA_RAW_SET_COMPLETE_EVT:
//         adv_config_done &= (~SCAN_RSP_CONFIG_FLAG);
//         if (adv_config_done == 0)
//         {
//             esp_ble_gap_start_advertising(&adv_params); // Advertiseを開始
//         }
//         break;
// #else
//     // Advertiseデータ設定完了イベント
//     case ESP_GAP_BLE_ADV_DATA_SET_COMPLETE_EVT:
//         adv_config_done &= (~ADV_CONFIG_FLAG);
//         if (adv_config_done == 0)
//         {
//             esp_ble_gap_start_advertising(&adv_params); // Advertiseを開始
//         }
//         break;
//     // スキャン応答データ設定完了イベント
//     case ESP_GAP_BLE_SCAN_RSP_DATA_SET_COMPLETE_EVT:
//         adv_config_done &= (~SCAN_RSP_CONFIG_FLAG);
//         if (adv_config_done == 0)
//         {
//             esp_ble_gap_start_advertising(&adv_params); // Advertiseを開始
//         }
//         break;
// #endif
//     // Advertise開始完了イベント
//     case ESP_GAP_BLE_ADV_START_COMPLETE_EVT:
//         if (param->adv_start_cmpl.status != ESP_BT_STATUS_SUCCESS)
//         {
//             ESP_LOGE(GATTS_TABLE_TAG, "advertising start failed"); // Advertise開始に失敗した場合
//         }
//         else
//         {
//             ESP_LOGI(GATTS_TABLE_TAG, "advertising start successfully"); // Advertise開始に成功した場合
//         }
//         break;
//     // Advertise停止完了イベント
//     case ESP_GAP_BLE_ADV_STOP_COMPLETE_EVT:
//         if (param->adv_stop_cmpl.status != ESP_BT_STATUS_SUCCESS)
//         {
//             ESP_LOGE(GATTS_TABLE_TAG, "Advertising stop failed"); // Advertise停止に失敗
//         }
//         else
//         {
//             ESP_LOGI(GATTS_TABLE_TAG, "Stop adv successfully"); // Advertise停止に成功
//         }
//         break;
//     // 接続パラメータの更新イベント
//     case ESP_GAP_BLE_UPDATE_CONN_PARAMS_EVT:
//         ESP_LOGI(GATTS_TABLE_TAG, "update connection params status = %d, min_int = %d, max_int = %d,conn_int = %d,latency = %d, timeout = %d",
//                  param->update_conn_params.status,
//                  param->update_conn_params.min_int,
//                  param->update_conn_params.max_int,
//                  param->update_conn_params.conn_int,
//                  param->update_conn_params.latency,
//                  param->update_conn_params.timeout); // 更新された接続パラメータの表示
//         break;
//     default:
//         break;
//     }
// }

// // 書き込み準備バッファの初期化関数
// static void example_prepare_write_event_env(esp_gatt_if_t gatts_if, prepare_type_env_t *prepare_write_env, esp_ble_gatts_cb_param_t *param)
// {
//     ESP_LOGI(GATTS_TABLE_TAG, "Preparing write event");
//     if (prepare_write_env->prepare_buf == NULL)
//     {
//         prepare_write_env->prepare_buf = (uint8_t *)malloc(PREPARE_BUF_MAX_SIZE); // 書き込みバッファをメモリに確保
//         prepare_write_env->prepare_len = 0;                                       // 初期化
//         if (prepare_write_env->prepare_buf == NULL)
//         {
//             ESP_LOGE(GATTS_TABLE_TAG, "No memory for prepare buffer");
//             return;
//         }
//     }

//     if (param->write.need_rsp)
//     { // 書き込み要求がACKを必要とする場合
//         if (param->write.is_prep)
//         {                                                                                                                  // 書き込みが部分的な場合
//             memcpy(prepare_write_env->prepare_buf + prepare_write_env->prepare_len, param->write.value, param->write.len); // バッファにデータをコピー
//             prepare_write_env->prepare_len += param->write.len;                                                            // バッファの長さを更新
//         }
//         else
//         {
//             esp_gatt_rsp_t rsp;                                                                                    // 応答を設定するための構造体
//             memset(&rsp, 0, sizeof(esp_gatt_rsp_t));                                                               // 応答構造体をゼロ初期化
//             rsp.attr_value.handle = param->write.handle;                                                           // 書き込みハンドルをセット
//             rsp.attr_value.len = param->write.len;                                                                 // 書き込みデータの長さ
//             memcpy(rsp.attr_value.value, param->write.value, param->write.len);                                    // 書き込みデータを応答にコピー
//             esp_ble_gatts_send_response(gatts_if, param->write.conn_id, param->write.trans_id, ESP_GATT_OK, &rsp); // クライアントに応答を送信
//         }
//     }
// }

// // 書き込み準備バッファの解放関数
// static void example_exec_write_event_env(prepare_type_env_t *prepare_write_env, esp_ble_gatts_cb_param_t *param)
// {
//     if (param->exec_write.exec_write_flag == ESP_GATT_PREP_WRITE_EXEC)
//     { // 書き込みの最終実行時
//         ESP_LOGI(GATTS_TABLE_TAG, "Execute write event");
//         ESP_LOG_BUFFER_HEX(GATTS_TABLE_TAG, prepare_write_env->prepare_buf, prepare_write_env->prepare_len); // 書き込みデータをログに出力
//     }
//     else
//     {
//         ESP_LOGI(GATTS_TABLE_TAG, "Cancel write event");
//     }

//     if (prepare_write_env->prepare_buf)
//     {                                         // バッファが存在する場合
//         free(prepare_write_env->prepare_buf); // バッファを解放
//         prepare_write_env->prepare_buf = NULL;
//     }
//     prepare_write_env->prepare_len = 0; // 長さをリセット
// }

// // 通知を送信する関数
// void send_notification(uint8_t value)
// {
//     uint8_t notify_data[1];
//     notify_data[0] = value;
//     uint8_t notify_data_B[1];
//     notify_data_B[0] = value + 10;

//     if (current_gatts_if != ESP_GATT_IF_NONE && notify_enabled != 0)
//     {
//         esp_err_t err = esp_ble_gatts_send_indicate(current_gatts_if, current_conn_id,
//                                                     heart_rate_handle_table[IDX_CHAR_VAL_A],
//                                                     sizeof(notify_data), notify_data, false);

//         esp_ble_gatts_send_indicate(current_gatts_if, current_conn_id, heart_rate_handle_table[IDX_CHAR_VAL_B],
//                                     sizeof(notify_data_B), notify_data_B, false);
//         if (err != ESP_OK)
//         {
//             ESP_LOGE(GATTS_TABLE_TAG, "send indicate failed, error code = %x", err);
//         }
//         else
//         {
//             ESP_LOGI(GATTS_TABLE_TAG, "send indicate success , value = %d", value);
//         }
//     }
// }

// void send_notification_2(uint8_t value, uint16_t _attr_handle)
// {
//     esp_err_t err = esp_ble_gatts_send_indicate(current_gatts_if, current_conn_id,_attr_handle,sizeof(value), &value, false);

//     if (err != ESP_OK)
//     {
//         ESP_LOGE(GATTS_TABLE_TAG, "send indicate failed, error code = %x", err);
//     }
//     else
//     {
//         ESP_LOGI(GATTS_TABLE_TAG, "send indicate success , value = %d", value);
//     }
// }

// void configure_notification(esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param, uint16_t *handle_table,
//                             uint16_t char_cfg_idx, uint16_t char_val_idx, bool *notify_enabled, TaskHandle_t *notify_task_handle,
//                             uint8_t notify_data[], size_t notify_data_len, uint8_t indicate_data[], size_t indicate_data_len)
// {
//     if (handle_table[char_cfg_idx] == param->write.handle && param->write.len == 2)
//     {
//         uint16_t descr_value = param->write.value[1] << 8 | param->write.value[0];
//         ESP_LOGI(GATTS_TABLE_TAG, "descr_value = %04x", descr_value);

//         if (descr_value == 0x0001) // 通知を有効にする
//         {
//             if (!notify_enabled)
//             {
//                 *notify_enabled = true;
//                 ESP_LOGI(GATTS_TABLE_TAG, "notify enable");
//             }

//             // 通知データを送信
//             esp_err_t err = esp_ble_gatts_send_indicate(gatts_if, param->write.conn_id, handle_table[char_val_idx],
//                                                         notify_data_len, notify_data, false);
//             if (err != ESP_OK)
//             {
//                 ESP_LOGE(GATTS_TABLE_TAG, "send notify failed, error code = %x", err);
//             }

//             if (notify_task_handle == NULL)
//             {
//                 xTaskCreatePinnedToCore(notify_task, "notify_task", 2048, NULL, configMAX_PRIORITIES - 1,
//                                         notify_task_handle, APP_CPU_NUM);
//             }
//         }
//         else if (descr_value == 0x0002) // インディケーションを有効にする
//         {
//             ESP_LOGI(GATTS_TABLE_TAG, "indicate enable");

//             // インディケーションデータを送信
//             esp_err_t err = esp_ble_gatts_send_indicate(gatts_if, param->write.conn_id, handle_table[char_val_idx],
//                                                         indicate_data_len, indicate_data, true);
//             if (err != ESP_OK)
//             {
//                 ESP_LOGE(GATTS_TABLE_TAG, "send indicate failed, error code = %x", err);
//             }
//         }
//         else if (descr_value == 0x0000) // 通知・インディケーションを無効にする
//         {
//             if (notify_enabled)
//             {
//                 *notify_enabled = false;
//                 ESP_LOGI(GATTS_TABLE_TAG, "notify/indicate disable");

//                 // Notifyタスクを停止
//                 if (notify_task_handle != NULL)
//                 {
//                     vTaskDelete(*notify_task_handle);
//                     notify_task_handle = NULL; // ハンドルをリセット
//                 }
//             }
//         }
//         else
//         {
//             ESP_LOGE(GATTS_TABLE_TAG, "unknown descr value : %04x", descr_value);
//             esp_log_buffer_hex(GATTS_TABLE_TAG, param->write.value, param->write.len);
//         }
//     }
// }

// // GATTのプロファイルイベントハンドラ関数
// static void gatts_profile_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param)
// {
//     esp_ble_conn_update_params_t conn_params;
//     // イベントに応じた処理を行う
//     switch (event)
//     {
//     // アプリケーション登録イベント
//     case ESP_GATTS_REG_EVT:
//     {
//         // デバイス名を設定
//         esp_err_t set_dev_name_ret = esp_ble_gap_set_device_name(SAMPLE_DEVICE_NAME);
//         if (set_dev_name_ret)
//         {
//             ESP_LOGE(GATTS_TABLE_TAG, "set device name failed, error code = %x", set_dev_name_ret);
//         }

// #ifdef CONFIG_SET_RAW_ADV_DATA
//         // 生のAdvertiseデータを設定
//         esp_err_t raw_adv_ret = esp_ble_gap_config_adv_data_raw(raw_adv_data, sizeof(raw_adv_data));
//         if (raw_adv_ret)
//         {
//             ESP_LOGE(GATTS_TABLE_TAG, "config raw adv data failed, error code = %x ", raw_adv_ret);
//         }
//         adv_config_done |= ADV_CONFIG_FLAG;

//         // 生のスキャン応答データを設定
//         esp_err_t raw_scan_ret = esp_ble_gap_config_scan_rsp_data_raw(raw_scan_rsp_data, sizeof(raw_scan_rsp_data));
//         if (raw_scan_ret)
//         {
//             ESP_LOGE(GATTS_TABLE_TAG, "config raw scan rsp data failed, error code = %x", raw_scan_ret);
//         }
//         adv_config_done |= SCAN_RSP_CONFIG_FLAG;
// #else
//         // 標準のAdvertiseデータを設定
//         esp_err_t ret = esp_ble_gap_config_adv_data(&adv_data);
//         if (ret)
//         {
//             ESP_LOGE(GATTS_TABLE_TAG, "config adv data failed, error code = %x", ret);
//         }
//         adv_config_done |= ADV_CONFIG_FLAG;

//         // スキャン応答データを設定
//         ret = esp_ble_gap_config_adv_data(&scan_rsp_data);
//         if (ret)
//         {
//             ESP_LOGE(GATTS_TABLE_TAG, "config scan response data failed, error code = %x", ret);
//         }
//         adv_config_done |= SCAN_RSP_CONFIG_FLAG;
// #endif
//         // GATTの属性テーブルを作成
//         esp_err_t create_attr_ret = esp_ble_gatts_create_attr_tab(gatt_db, gatts_if, HRS_IDX_NB, SVC_INST_ID);
//         if (create_attr_ret)
//         {
//             ESP_LOGE(GATTS_TABLE_TAG, "create attr table failed, error code = %x", create_attr_ret);
//         }
//     }
//     break;

//     // 読み取り要求イベント
//     case ESP_GATTS_READ_EVT:
//         ESP_LOGI(GATTS_TABLE_TAG, "ESP_GATTS_READ_EVT");
//         break;

//     // 書き込み要求イベント
//     case ESP_GATTS_WRITE_EVT:
//         // 準備書き込みでない場合
//         if (!param->write.is_prep)
//         {
//             // 書き込みの詳細をログに表示
//             ESP_LOGI(GATTS_TABLE_TAG, "GATT_WRITE_EVT, handle = %d, value len = %d, value :", param->write.handle, param->write.len);
//             esp_log_buffer_hex(GATTS_TABLE_TAG, param->write.value, param->write.len);

//             /*uint8_t notify_data[] = {x}; // 通知データ
//             uint8_t indicate_data[15];
//             for (int i = 0; i < sizeof(indicate_data); ++i)
//             {
//                 indicate_data[i] = i % 0xff;
//             }
//             configure_notification(gatts_if, param, heart_rate_handle_table, IDX_CHAR_CFG_A, IDX_CHAR_VAL_A,
//                                    notify_enabled, notify_task_handle, notify_data, sizeof(notify_data),
//                                    indicate_data, sizeof(indicate_data));*/

//             // 通知・インディケーションの設定
//             if (heart_rate_handle_table[IDX_CHAR_CFG_A] == param->write.handle && param->write.len == 2)
//             {
//                 uint16_t descr_value = param->write.value[1] << 8 | param->write.value[0];
//                 ESP_LOGI(GATTS_TABLE_TAG, "descr_value = %04x", descr_value);
//                 if (descr_value == 0x0001) // if文のインデントに注意 (正しく条件分岐できず、disconnect時にエラーが発生していたのを修正)
//                 {
//                     if (!notify_enabled)
//                     {
//                         notify_enabled = true;
//                         // Notifyタスクを開始
//                     }
//                     ESP_LOGI(GATTS_TABLE_TAG, "notify enable");
//                     // notify_enabled = true; // 通知を有効にするフラグをセット
//                     //  uint8_t notify_data[15];
//                     uint8_t notify_data[1];
//                     notify_data[0] = x;

//                     for (int i = 0; i < sizeof(notify_data); ++i)
//                     {
//                         notify_data[i] = i % 0xff; // 通知データを設定
//                     }

//                     // 通知を送信
//                     esp_err_t err = esp_ble_gatts_send_indicate(gatts_if, param->write.conn_id, heart_rate_handle_table[IDX_CHAR_VAL_A],
//                                                                 sizeof(notify_data), notify_data, false);
//                     if (err != ESP_OK)
//                     {
//                         ESP_LOGE(GATTS_TABLE_TAG, "send indicate failed, error code = %x", err);
//                     }

//                     if (notify_task_handle == NULL)
//                     {
//                         xTaskCreatePinnedToCore(notify_task, "notify_task", 2048, NULL, configMAX_PRIORITIES - 1, &notify_task_handle, APP_CPU_NUM);
//                     }
//                 }
//                 else if (descr_value == 0x0002)
//                 {
//                     ESP_LOGI(GATTS_TABLE_TAG, "indicate enable");
//                     uint8_t indicate_data[15];
//                     for (int i = 0; i < sizeof(indicate_data); ++i)
//                     {
//                         indicate_data[i] = i % 0xff; // インディケーションデータを設定
//                     }
//                     // インディケーションを送信
//                     esp_ble_gatts_send_indicate(gatts_if, param->write.conn_id, heart_rate_handle_table[IDX_CHAR_VAL_A],
//                                                 sizeof(indicate_data), indicate_data, true);
//                 }
//                 else if (descr_value == 0x0000)
//                 {
//                     if (notify_enabled)
//                     {
//                         notify_enabled = false;
//                         ESP_LOGI(GATTS_TABLE_TAG, "notify disabled");
//                         // Notifyタスクを停止
//                         if (notify_task_handle != NULL)
//                         {
//                             vTaskDelete(notify_task_handle);
//                             notify_task_handle = NULL; // ハンドルをリセット
//                         }
//                     }
//                     ESP_LOGI(GATTS_TABLE_TAG, "notify/indicate disable ");
//                 }
//                 else
//                 {
//                     ESP_LOGE(GATTS_TABLE_TAG, "unknown descr value : %04x", descr_value);
//                     esp_log_buffer_hex(GATTS_TABLE_TAG, param->write.value, param->write.len);
//                 }
//             }
//             // キャラクタリスティックBの通知・インディケーション設定
//             if (heart_rate_handle_table[IDX_CHAR_CFG_B] == param->write.handle && param->write.len == 2)
//             {
//                 uint16_t descr_value = param->write.value[1] << 8 | param->write.value[0];
//                 ESP_LOGI(GATTS_TABLE_TAG, "descr_value for B = %04x", descr_value);
//                 if (descr_value == 0x0001)
//                 {
//                     if (!notify_enabled)
//                     {
//                         notify_enabled = true;
//                         ESP_LOGI(GATTS_TABLE_TAG, "notify enable for B");

//                         uint8_t notify_data_B[1];
//                         notify_data_B[0] = x; // 送信データを設定
//                         esp_err_t err = esp_ble_gatts_send_indicate(gatts_if, param->write.conn_id, heart_rate_handle_table[IDX_CHAR_VAL_B],
//                                                                     sizeof(notify_data_B), notify_data_B, false);
//                         if (err != ESP_OK)
//                         {
//                             ESP_LOGE(GATTS_TABLE_TAG, "send indicate for B failed, error code = %x", err);
//                         }
//                     }
//                 }
//                 else if (descr_value == 0x0000)
//                 {
//                     if (notify_enabled)
//                     {
//                         notify_enabled = false;
//                         ESP_LOGI(GATTS_TABLE_TAG, "notify disabled for B");
//                     }
//                 }
//             }
//             // 応答が必要な場合、レスポンスを送信
//             if (param->write.need_rsp)
//             {
//                 esp_ble_gatts_send_response(gatts_if, param->write.conn_id, param->write.trans_id, ESP_GATT_OK, NULL);
//             }
//         }
//         else
//         {
//             // 準備書き込みの処理
//             example_prepare_write_event_env(gatts_if, &prepare_write_env, param);
//         }

//         break;

//     // 書き込み実行イベント
//     case ESP_GATTS_EXEC_WRITE_EVT:
//         ESP_LOGI(GATTS_TABLE_TAG, "ESP_GATTS_EXEC_WRITE_EVT");
//         // 準備書き込みデータの処理
//         example_exec_write_event_env(&prepare_write_env, param);
//         break;

//     // MTU変更イベント
//     case ESP_GATTS_MTU_EVT:
//         ESP_LOGI(GATTS_TABLE_TAG, "ESP_GATTS_MTU_EVT, MTU %d", param->mtu.mtu);
//         break;

//     // 確認イベント（インディケーションの応答など）
//     case ESP_GATTS_CONF_EVT:
//         ESP_LOGI(GATTS_TABLE_TAG, "ESP_GATTS_CONF_EVT, status = %d, attr_handle %d", param->conf.status, param->conf.handle);
//         break;

//     // サービス開始イベント
//     case ESP_GATTS_START_EVT:
//         ESP_LOGI(GATTS_TABLE_TAG, "SERVICE_START_EVT, status %d, service_handle %d", param->start.status, param->start.service_handle);
//         break;

//     // 接続イベント
//     case ESP_GATTS_CONNECT_EVT:
//         ESP_LOGI(GATTS_TABLE_TAG, "ESP_GATTS_CONNECT_EVT, conn_id = %d", param->connect.conn_id);

//         current_conn_id = param->connect.conn_id; // 現在の接続IDを保存
//         current_gatts_if = gatts_if;              // 現在のGATTインターフェースを保存

//         ESP_LOGI(GATTS_TABLE_TAG, "ESP_GATTS_CONNECT_EVT, current_conn_id : %d, gatts_if : %d", current_conn_id, current_gatts_if);

//         esp_log_buffer_hex(GATTS_TABLE_TAG, param->connect.remote_bda, 6);
//         conn_params = {0};
//         memcpy(conn_params.bda, param->connect.remote_bda, sizeof(esp_bd_addr_t));
//         conn_params.latency = 0;
//         conn_params.max_int = 0x20; // 最大接続間隔 = 0x20 * 1.25ms = 40ms
//         conn_params.min_int = 0x10; // 最小接続間隔 = 0x10 * 1.25ms = 20ms
//         conn_params.timeout = 400;  // タイムアウト = 400 * 10ms = 4000ms
//         // 接続パラメータを更新
//         esp_ble_gap_update_conn_params(&conn_params);
//         break;

//     // 切断イベント
//     case ESP_GATTS_DISCONNECT_EVT:
//         ESP_LOGI(GATTS_TABLE_TAG, "ESP_GATTS_DISCONNECT_EVT, reason = 0x%x", param->disconnect.reason);

//         current_conn_id = 0; // 接続が切断されたのでIDをリセット

//         // 再度Advertiseを開始
//         esp_ble_gap_start_advertising(&adv_params);
//         break;

//     // 属性テーブル作成イベント
//     case ESP_GATTS_CREAT_ATTR_TAB_EVT:
//     {
//         if (param->add_attr_tab.status != ESP_GATT_OK)
//         {
//             ESP_LOGE(GATTS_TABLE_TAG, "create attribute table failed, error code=0x%x", param->add_attr_tab.status);
//         }
//         else if (param->add_attr_tab.num_handle != HRS_IDX_NB)
//         {
//             ESP_LOGE(GATTS_TABLE_TAG, "create attribute table abnormally, num_handle (%d) doesn't equal to HRS_IDX_NB(%d)", param->add_attr_tab.num_handle, HRS_IDX_NB);
//         }
//         else
//         {
//             ESP_LOGI(GATTS_TABLE_TAG, "create attribute table successfully, the number handle = %d", param->add_attr_tab.num_handle);
//             // 作成したハンドルを保存し、サービスを開始
//             memcpy(heart_rate_handle_table, param->add_attr_tab.handles, sizeof(heart_rate_handle_table));
//             esp_ble_gatts_start_service(heart_rate_handle_table[IDX_SVC]);
//         }
//         break;
//     }

//     // その他のイベント
//     case ESP_GATTS_STOP_EVT:
//     case ESP_GATTS_OPEN_EVT:
//     case ESP_GATTS_CANCEL_OPEN_EVT:
//     case ESP_GATTS_CLOSE_EVT:
//     case ESP_GATTS_LISTEN_EVT:
//     case ESP_GATTS_CONGEST_EVT:
//     case ESP_GATTS_UNREG_EVT:
//     case ESP_GATTS_DELETE_EVT:
//     default:
//         break;
//     }
// }

// // GATTの全体イベントハンドラ関数
// static void gatts_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param)
// {
//     // アプリケーション登録イベントの場合、プロファイルのインターフェースを保存
//     if (event == ESP_GATTS_REG_EVT)
//     {
//         if (param->reg.status == ESP_GATT_OK)
//         {
//             heart_rate_profile_tab[PROFILE_APP_IDX].gatts_if = gatts_if;
//         }
//         else
//         {
//             ESP_LOGE(GATTS_TABLE_TAG, "reg app failed, app_id %04x, status %d",
//                      param->reg.app_id, param->reg.status);
//             return;
//         }
//     }

//     // 各プロファイルに対してイベントを送信
//     do
//     {
//         int idx;
//         for (idx = 0; idx < PROFILE_NUM; idx++)
//         {
//             // GATT_IF_NONEの場合はすべてのプロファイルに対してコールバックを呼び出す
//             if (gatts_if == ESP_GATT_IF_NONE || gatts_if == heart_rate_profile_tab[idx].gatts_if)
//             {
//                 if (heart_rate_profile_tab[idx].gatts_cb)
//                 {
//                     heart_rate_profile_tab[idx].gatts_cb(event, gatts_if, param);
//                 }
//             }
//         }
//     } while (0);
// }

// void notify_task(void *pvparam)
// {
//     uint8_t f = 0;
//     while (true)
//     {
//         int64_t start_time = esp_timer_get_time(); // 現在時刻を取得 (us単位)
//         if (notify_enabled)
//         {
//             if (f > 100)
//             {
//                 f = 0;
//             }
//             x = uint8_t(sens.BatteryVoltage*10);
//             f++;

//             // ESP_LOGI(GATTS_TABLE_TAG, "x = %d", x);

//             send_notification(x);                  // 現在の x の値を通知

            
//             vTaskDelay(1000 / portTICK_PERIOD_MS); // 1秒待機
//         }
//         else
//         {
//             vTaskDelay(100 / portTICK_PERIOD_MS); // 0.1秒待機
//         }
//         int64_t end_time = esp_timer_get_time(); // 終了時刻を取得
//         int64_t duration = end_time - start_time;

//         //send_notification_2(duration,heart_rate_handle_table[IDX_CHAR_VAL_C]);

//         ESP_LOGI(GATTS_TABLE_TAG, "duration = %lld", duration); // 処理時間 約1000800us = 1.0008s (1000ms + 0.8ms)
//     }
// }




////////////////////////////////////////////////////////////////////////////////////////

std::shared_ptr<t_drivers> driver = std::make_shared<t_drivers>();


static SemaphoreHandle_t wallCharged;

static void timer_chargeCompleted(void *arg)
{
    BaseType_t _highPriorityTask = pdFALSE;
    xSemaphoreGiveFromISR(wallCharged, &_highPriorityTask);
    portYIELD_FROM_ISR(_highPriorityTask);
}

void myTaskAdc(void *pvpram)
{
    // pvpram が指す既存の ADS7066 ポインタを std::shared_ptr に変換
    /*ADS7066* raw_adc_ptr = static_cast<ADS7066 *>(pvpram);
    std::shared_ptr<ADS7066> adc(raw_adc_ptr);
    driver->led->set(0b1010);

    // t_drivers 構造体を作成し、adc ポインタを設定
    std::shared_ptr<t_drivers> driver = std::make_shared<t_drivers>();
    driver->adc = adc;*/

    ESP_LOGI("ADC", "ADC Task Start");
    driver->led->set(0b1000);

    for (int i = 0; i < 4; i++)
    {
        gpio_set_level(driver->adc->LED[i], 1);
    }
    driver->adc->_off = driver->adc->readOnTheFly(4);

    esp_timer_create_args_t chargeTimerSetting = {
        .callback = &timer_chargeCompleted,
        .name = "wallCharge"};
    esp_timer_handle_t chargeTimer;
    ESP_ERROR_CHECK(esp_timer_create(&chargeTimerSetting, &chargeTimer));

    wallCharged = xSemaphoreCreateBinary();
    driver->led->set(0b1111);

    // センサの設定 (コンデンサ充電時間、放電時間 値のオーバーフロー対策必須（時間設定するか、例外処理追加するか）)　ｒが怪しい
    uint16_t charge_us = 500; // コンデンサへの充電時間
    uint16_t rise_us = 30;    // 放電してからセンサの読み取りを開始するまでの時間

    while (1)
    {
        sens.BatteryVoltage = driver->adc->BatteryVoltage();
        for (int i = 0; i < 4; i++)
        {
            if (i > 0) // i = 0 のときは _on が初期化されていないため、読み取りを行わない
            {
                driver->adc->_on = driver->adc->readOnTheFly(driver->adc->SENS[i]); // readOnTheFly は 送ったアドレスのひとつ前に送った値を返す
            }
            gpio_set_level(driver->adc->LED[i], 0);
            esp_timer_start_once(chargeTimer, charge_us);
            xSemaphoreTake(wallCharged, portMAX_DELAY);
            gpio_set_level(driver->adc->LED[i], 1);
            esp_rom_delay_us(rise_us);
            if (i > 0) // i = 0 のときは _on が初期化されていないため、読み取りを行わない
            {
                // 何も無いところを見ていると、on,offの値が逆転することがあるため対策
                if (driver->adc->_on - driver->adc->_off > 0) // on, off の差分が正のとき(on時の値のほうが大きいとき)
                {
                    driver->adc->value[i - 1] = driver->adc->_on - driver->adc->_off;
                }
                else
                {
                    driver->adc->value[i - 1] = driver->adc->_off - driver->adc->_on;
                }
            }
            driver->adc->_off = driver->adc->readOnTheFly(driver->adc->SENS[i]);
        }
        driver->adc->_on = driver->adc->readOnTheFly(4);
        if (driver->adc->_on - driver->adc->_off > 0) // on, off の差分が正のとき(on時の値のほうが大きいとき)
        {
            driver->adc->value[3] = driver->adc->_on - driver->adc->_off;
        }
        else
        {
            driver->adc->value[3] = driver->adc->_off - driver->adc->_on;
        }

        sens.wall.val.fr = driver->adc->value[0];
        sens.wall.val.r = driver->adc->value[2];
        sens.wall.val.l = driver->adc->value[1];
        sens.wall.val.fl = driver->adc->value[3];
        
        // === 壁センサローパスフィルタ（指数移動平均） ===
        static float wall_fl_filtered = 0.0;
        static float wall_fr_filtered = 0.0;
        static float wall_l_filtered = 0.0;
        static float wall_r_filtered = 0.0;
        static const float wall_filter_alpha = 0.5; // 指数移動平均の重み（0.0-1.0、小さいほど平滑化が強い）
        
        // 生の壁センサ値を取得
        float raw_fl = sens.wall.val.fl;
        float raw_fr = sens.wall.val.fr;
        float raw_l = sens.wall.val.l;
        float raw_r = sens.wall.val.r;
        
        // 指数移動平均（EMAフィルタ）
        wall_fl_filtered = wall_filter_alpha * raw_fl + (1.0 - wall_filter_alpha) * wall_fl_filtered;
        wall_fr_filtered = wall_filter_alpha * raw_fr + (1.0 - wall_filter_alpha) * wall_fr_filtered;
        wall_l_filtered = wall_filter_alpha * raw_l + (1.0 - wall_filter_alpha) * wall_l_filtered;
        wall_r_filtered = wall_filter_alpha * raw_r + (1.0 - wall_filter_alpha) * wall_r_filtered;
        
        // フィルタ後の値を構造体に書き戻す
        sens.wall.val.fl = (int)wall_fl_filtered;
        sens.wall.val.fr = (int)wall_fr_filtered;
        sens.wall.val.l = (int)wall_l_filtered;
        sens.wall.val.r = (int)wall_r_filtered;

        vTaskDelay(1 / portTICK_PERIOD_MS);
    }
}

extern "C" void app_main(void)
{
    gpio_config_t io_conf = {};
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask =
        (1ULL << 10) | (1ULL << 17) | (1ULL << 18) | (1ULL << 21);
    io_conf.pull_down_en = GPIO_PULLDOWN_ENABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;

    ESP_ERROR_CHECK(gpio_config(&io_conf));

    // IMU SPIバスの設定
    spi_bus_config_t bus_imu_adc;
    memset(&bus_imu_adc, 0, sizeof(bus_imu_adc));
    bus_imu_adc.miso_io_num = GPIO_NUM_2;
    bus_imu_adc.mosi_io_num = GPIO_NUM_4;
    bus_imu_adc.sclk_io_num = GPIO_NUM_3;
    bus_imu_adc.quadwp_io_num = -1;
    bus_imu_adc.quadhd_io_num = -1;

    ESP_ERROR_CHECK(spi_bus_initialize(SPI2_HOST, &bus_imu_adc, SPI_DMA_CH_AUTO));

    driver->adc = std::make_shared<ADS7066>(SPI2_HOST, GPIO_NUM_5);
    driver->imu = std::make_shared<MPU6500>(SPI2_HOST, GPIO_NUM_1);

    // Encoder SPIバスの設定
    spi_bus_config_t bus_enc;
    memset(&bus_enc, 0, sizeof(bus_enc));
    bus_enc.mosi_io_num = GPIO_NUM_9;
    bus_enc.miso_io_num = GPIO_NUM_8;
    bus_enc.sclk_io_num = GPIO_NUM_7;
    bus_enc.quadwp_io_num = -1;
    bus_enc.quadhd_io_num = -1;
    bus_enc.max_transfer_sz = 4;
    bus_enc.flags = SPICOMMON_BUSFLAG_MASTER;
    bus_enc.intr_flags = 0;

    ESP_ERROR_CHECK(spi_bus_initialize(SPI3_HOST, &bus_enc, SPI_DMA_DISABLED));

    driver->encL = std::make_shared<MA730>(SPI3_HOST, GPIO_NUM_6, 1);
    driver->encR = std::make_shared<MA730>(SPI3_HOST, GPIO_NUM_14, 0);

    // LED driver I2Cバスの設定
    i2c_config_t led_conf;
    memset(&led_conf, 0, sizeof(led_conf));
    led_conf.mode = I2C_MODE_MASTER;
    led_conf.sda_io_num = GPIO_NUM_38;
    led_conf.scl_io_num = GPIO_NUM_39;
    led_conf.sda_pullup_en = GPIO_PULLUP_DISABLE;
    led_conf.scl_pullup_en = GPIO_PULLUP_DISABLE;
    led_conf.master.clk_speed = 1000000;
    led_conf.clk_flags = 0;

    ESP_ERROR_CHECK(i2c_param_config(I2C_NUM_0, &led_conf));
    ESP_ERROR_CHECK(i2c_driver_install(I2C_NUM_0, I2C_MODE_MASTER, 0, 0, 0));

    driver->led = std::make_shared<PCA9632>(I2C_NUM_0, 0x62);

    driver->led->set(0b1111);

    // Buzzer GPIOの設定
    driver->bz = std::make_shared<BUZZER>(GPIO_NUM_15);
    static BUZZER::buzzer_score_t pc98[] = {
        {2000, 100}, {1000, 100}};
    driver->bz->play_melody(pc98, 2);

    // NeoPixel GPIOの設定
    driver->np = std::make_shared<NeoPixel>(GPIO_NUM_13, 1);
    driver->np->set_hsv({0, 0, 0}, 0, 1);
    driver->np->show();

    // Motor driver Fan Moter GPIOの設定
    driver->mot = std::make_shared<Motor>(GPIO_NUM_41, GPIO_NUM_42, GPIO_NUM_45, GPIO_NUM_46, GPIO_NUM_11, GPIO_NUM_40);

    driver->led->set(0b1110);
    ADS7066 *adc = driver->adc.get();
    driver->led->set(0b1100);

    xTaskCreatePinnedToCore(myTaskAdc,
                            "adc", 8192, adc, configMAX_PRIORITIES - 2, NULL, APP_CPU_NUM);

    /////////////////////////// BLE ///////////////////////////

    // populate_gatt_db();

    // esp_err_t ret;

    // /* Initialize NVS. */
    // ret = nvs_flash_init();
    // if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    // {
    //     ESP_ERROR_CHECK(nvs_flash_erase());
    //     ret = nvs_flash_init();
    // }
    // ESP_ERROR_CHECK(ret);

    // // Initialize Bluetooth
    // ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT)); // クラシックBTモードのメモリ解放

    // esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    // ret = esp_bt_controller_init(&bt_cfg); // BTコントローラーの初期化
    // if (ret)
    // {
    //     ESP_LOGE(GATTS_TABLE_TAG, "%s enable controller failed: %s", __func__, esp_err_to_name(ret));
    //     return;
    // }

    // ret = esp_bt_controller_enable(ESP_BT_MODE_BLE); // BLEモードを有効化
    // if (ret)
    // {
    //     ESP_LOGE(GATTS_TABLE_TAG, "%s enable controller failed: %s", __func__, esp_err_to_name(ret));
    //     return;
    // }

    // esp_bluedroid_config_t bluedroid_cfg = BT_BLUEDROID_INIT_CONFIG_DEFAULT();
    // ret = esp_bluedroid_init_with_cfg(&bluedroid_cfg); // ブルードロイドスタックの初期化
    // if (ret)
    // {
    //     ESP_LOGE(GATTS_TABLE_TAG, "%s init bluetooth failed: %s", __func__, esp_err_to_name(ret));
    //     return;
    // }

    // ret = esp_bluedroid_enable(); // ブルードロイドを有効化
    // if (ret)
    // {
    //     ESP_LOGE(GATTS_TABLE_TAG, "%s enable bluetooth failed: %s", __func__, esp_err_to_name(ret));
    //     return;
    // }

    // // GATTサーバの登録
    // ret = esp_ble_gatts_register_callback(gatts_event_handler); // GATTイベントハンドラの登録
    // if (ret)
    // {
    //     ESP_LOGE(GATTS_TABLE_TAG, "gatts register error, error code = %x", ret);
    //     return;
    // }

    // ret = esp_ble_gap_register_callback(gap_event_handler); // GAPイベントハンドラの登録
    // if (ret)
    // {
    //     ESP_LOGE(GATTS_TABLE_TAG, "gap register error, error code = %x", ret);
    //     return;
    // }

    // ret = esp_ble_gatts_app_register(ESP_APP_ID); // アプリケーションをGATTサーバに登録
    // if (ret)
    // {
    //     ESP_LOGE(GATTS_TABLE_TAG, "gatts app register error, error code = %x", ret);
    //     return;
    // }

    // esp_err_t local_mtu_ret = esp_ble_gatt_set_local_mtu(500); // 最大MTUサイズを設定
    // if (local_mtu_ret)
    // {
    //     ESP_LOGE(GATTS_TABLE_TAG, "set local  MTU failed, error code = %x", local_mtu_ret);
    // }

    /////////////////////////////////////////////////

    //uint32_t h = 0, h1 = 0;
    //float t = 0.0;
    //float rad = 0.0;
    init_files();
    
    while (1)
    {
        // h = driver->imu->accelZ() * 360;
        //driver->np->set_hsv({h, 100, 10}, 0, 1);
        driver->np->set_hsv({240, 100, 100}, 0, 1);
        driver->np->show();
        //driver->np->gaming_mouse();
        // printf("BAT : %f\n", sens.BatteryVoltage);
        // printf("sens.wall.val.fl:%d  sens.wall.val.l:%d  sens.wall.val.r:%d  sens.wall.val.fr:%d\n", sens.wall.val.fl, sens.wall.val.l, sens.wall.val.r, sens.wall.val.fr);
        //   printf("driver->adc->off:%d\n", driver->adc->_off);
        MICROMOUSE(driver, &sens);

        //driver->mot->setMotorSpeed((0.2), (0.2));

        

        // printf("Z : %ld\n", h);
        /*
        driver->mot->setMotorSpeed(1.0 * sin(t), 1.0 * sin(t));
        t = t + 0.01;
        if (t > 2 * M_PI)
            t = 0.0;
        */

        //printf("gyroZ : %f\n", driver->imu->gyroZ());
        // printf("ang_vel : %f\n", driver->imu->gyroZ() * (M_PI / 180.0));
        //rad += driver->imu->gyroZ() * (M_PI / 180.0) / 1000.0 *100;// 1tick が100ms周期になっているため *100
        //printf("rad : %f\n", rad);

        /*
        h = driver->encL->readAngle();
        h1 = driver->encR->readAngle();

        float WheelAngle_L = 2.0 * M_PI * h / 16384.0;
        float WheelAngle_R = 2.0 * M_PI * h1 / 16384.0;

        float WeeelDegree_L = WheelAngle_L * 180.0 / M_PI;
        float WeeelDegree_R = WheelAngle_R * 180.0 / M_PI;

        printf(">L:%ld\n", h);
        //printf(">R:%ld\n", h1);
        //printf("L:%f    R:%f\n", WheelAngle_L, WheelAngle_R);
        // printf("L:%f    R:%f\n", WeeelDegree_L, WeeelDegree_R);
        */
        //  ESP_LOGI("MAIN", "MAIN LOOP");
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
