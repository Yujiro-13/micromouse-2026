#ifndef GATTS_TABLE_CREATE_HPP
#define GATTS_TABLE_CREATE_HPP


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define GATTS_DEMO_CHAR_VAL_LEN_MAX 500         // Characteristic値の最大長


/* Attributes State Machine */
enum
{
    IDX_SVC,
    IDX_CHAR_A,
    IDX_CHAR_VAL_A,
    IDX_CHAR_CFG_A,

    IDX_CHAR_B,
    IDX_CHAR_VAL_B,
    IDX_CHAR_CFG_B,

    IDX_CHAR_C,
    IDX_CHAR_VAL_C,
    IDX_CHAR_CFG_C,

    IDX_CHAR_D,
    IDX_CHAR_VAL_D,
    IDX_CHAR_CFG_D,

    IDX_CHAR_E,
    IDX_CHAR_VAL_E,
    IDX_CHAR_CFG_E,

    IDX_CHAR_F,
    IDX_CHAR_VAL_F,
    IDX_CHAR_CFG_F,

    IDX_CHAR_G,
    IDX_CHAR_VAL_G,
    IDX_CHAR_CFG_G,

    IDX_CHAR_H,
    IDX_CHAR_VAL_H,
    IDX_CHAR_CFG_H,

    IDX_CHAR_I,
    IDX_CHAR_VAL_I,
    IDX_CHAR_CFG_I,

    IDX_CHAR_J,
    IDX_CHAR_VAL_J,
    IDX_CHAR_CFG_J,

    HRS_IDX_NB,
};

/* クライアント設定のデフォルト値とCharacteristicのデフォルト値 */
static const uint8_t heart_measurement_ccc[2] = {0x00, 0x00};  // 通知/インディケーションのデフォルト設定（無効）
static const uint8_t char_value[4] = {0x11, 0x22, 0x33, 0x44}; // Characteristicのデフォルト値



/* GATT Service, Characteristic, and Descriptor UUIDs */
static const uint16_t GATTS_SERVICE_UUID_TEST = 0x00FF; // サービスUUID
static const uint16_t GATTS_CHAR_UUID_TEST_A = 0xFF01;  // CharacteristicAのUUID
static const uint16_t GATTS_CHAR_UUID_TEST_B = 0xFF02;  // CharacteristicBのUUID
static const uint16_t GATTS_CHAR_UUID_TEST_C = 0xFF03;  // CharacteristicCのUUID
static const uint16_t GATTS_CHAR_UUID_TEST_D = 0xFF04;
static const uint16_t GATTS_CHAR_UUID_TEST_E = 0xFF05;
static const uint16_t GATTS_CHAR_UUID_TEST_F = 0xFF06;
static const uint16_t GATTS_CHAR_UUID_TEST_G = 0xFF07;
static const uint16_t GATTS_CHAR_UUID_TEST_H = 0xFF08;
static const uint16_t GATTS_CHAR_UUID_TEST_I = 0xFF09;
static const uint16_t GATTS_CHAR_UUID_TEST_J = 0xFF0A;

typedef struct {
    uint16_t char_uuid;    // CharacteristicのUUID
    uint16_t perm;         // アクセス権限（読み取り・書き込み可能）
    uint16_t max_len;      // Characteristicの値の最大長
    uint8_t *value;        // Characteristicのデフォルト値
    uint16_t cfg_perm;     // Client Configurationのアクセス権限
    uint8_t *cfg_val;      // Client Configuration値
} gatt_char_config_t;

#define CHAR_CONFIG(uuid) { \
    .char_uuid = (uuid), \
    .perm = ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE, \
    .max_len = GATTS_DEMO_CHAR_VAL_LEN_MAX, \
    .value = (uint8_t *)char_value, \
    .cfg_perm = ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE, \
    .cfg_val = (uint8_t *)heart_measurement_ccc \
}

static const gatt_char_config_t char_configs[] = {
    CHAR_CONFIG(GATTS_CHAR_UUID_TEST_A),
    CHAR_CONFIG(GATTS_CHAR_UUID_TEST_B),
    CHAR_CONFIG(GATTS_CHAR_UUID_TEST_C),
    CHAR_CONFIG(GATTS_CHAR_UUID_TEST_D),
    CHAR_CONFIG(GATTS_CHAR_UUID_TEST_E),
    CHAR_CONFIG(GATTS_CHAR_UUID_TEST_F),
    CHAR_CONFIG(GATTS_CHAR_UUID_TEST_G),
    CHAR_CONFIG(GATTS_CHAR_UUID_TEST_H),
    CHAR_CONFIG(GATTS_CHAR_UUID_TEST_I),
    CHAR_CONFIG(GATTS_CHAR_UUID_TEST_J),
};

#endif