#include "ads7066.hpp"

ADS7066::ADS7066(spi_host_device_t bus, gpio_num_t cs)
{
    // デバイスの初期化
    memset(&dev_adc, 0, sizeof(dev_adc));
    dev_adc.clock_speed_hz = 60 * 1000 * 1000;
    dev_adc.mode = 0;
    dev_adc.spics_io_num = cs;
    dev_adc.queue_size = 4;

    err = spi_bus_add_device(bus, &dev_adc, &_spi);
    ESP_ERROR_CHECK(err);

    write_register(0x1, 0b1);
    while (read_register(0x1) & 0b1)
    {
        vTaskDelay(1);
    }

    write_register(0x2, 0b010000);
    write_register(0x3, 0b0);
    write_register(0x4, 0b0);

    ESP_LOGI("ADS7066", "%x", read_register(0x0));

    SEQ_MODE = read_register(0x10) & 0b11;
    ESP_LOGI("ADS7066", "%d", SEQ_MODE);
}

ADS7066::~ADS7066() {}

void ADS7066::write_register(uint8_t adrs, uint8_t data)
{
    spi_transaction_t cmd;

    memset(&cmd, 0, sizeof(cmd));
    cmd.flags = SPI_TRANS_USE_RXDATA | SPI_TRANS_USE_TXDATA;
    cmd.length = 24;
    cmd.tx_data[0] = 0b01 << 3;
    cmd.tx_data[1] = adrs;
    cmd.tx_data[2] = data;
    err = spi_device_polling_transmit(_spi, &cmd);
    ESP_ERROR_CHECK(err);
    vTaskDelay(1);
}

uint16_t ADS7066::read_one_shot(int8_t channelID)
{
    ESP_ERROR_CHECK(change_seq_mode(0b00));
    write_register(0x11, channelID);
    spi_transaction_t cmd;

    // 空読み
    memset(&cmd, 0, sizeof(cmd));
    cmd.flags = SPI_TRANS_USE_TXDATA;
    cmd.length = 24;
    err = spi_device_polling_transmit(_spi, &cmd);
    ESP_ERROR_CHECK(err);

    memset(&cmd, 0, sizeof(cmd));
    cmd.flags = SPI_TRANS_USE_RXDATA;
    cmd.length = 24;
    err = spi_device_polling_transmit(_spi, &cmd);
    ESP_ERROR_CHECK(err);

    if (channelID != cmd.rx_data[2] >> 4)
    {
        return 0;
    }
    return cmd.rx_data[0] << 8 | cmd.rx_data[1];
}

uint16_t ADS7066::read_on_the_fly(int8_t channelID)
{
    ESP_ERROR_CHECK(change_seq_mode(0b10));
    spi_transaction_t cmd;

    memset(&cmd, 0, sizeof(cmd));
    cmd.flags = SPI_TRANS_USE_RXDATA | SPI_TRANS_USE_TXDATA;
    cmd.length = 16;

    if (channelID >= 0)
    {
        uint8_t _select = (1 << 4) | (channelID & 0b1111);
        cmd.tx_data[0] = _select << 3;
    }

    err = spi_device_polling_transmit(_spi, &cmd);
    ESP_ERROR_CHECK(err);
    return cmd.rx_data[0] << 8 | cmd.rx_data[1];
}

esp_err_t ADS7066::change_seq_mode(uint8_t mode)
{
    if (SEQ_MODE != mode)
    {
        write_register(0x10, 0b10);
        vTaskDelay(1);
        SEQ_MODE = read_register(0x10);
        if (SEQ_MODE != mode)
        {
            return ESP_FAIL;
        }
    }
    return ESP_OK;
}

uint8_t ADS7066::read_register(uint8_t adrs)
{
    spi_transaction_t cmd;

    memset(&cmd, 0, sizeof(cmd));
    cmd.flags = SPI_TRANS_USE_RXDATA | SPI_TRANS_USE_TXDATA;
    cmd.length = 24;
    cmd.tx_data[0] = 0b10 << 3;
    cmd.tx_data[1] = adrs;
    cmd.tx_data[2] = 0;
    err = spi_device_polling_transmit(_spi, &cmd);
    ESP_ERROR_CHECK(err);

    memset(&cmd, 0, sizeof(cmd));
    cmd.flags = SPI_TRANS_USE_RXDATA | SPI_TRANS_USE_TXDATA;
    cmd.length = 24;
    err = spi_device_polling_transmit(_spi, &cmd);
    ESP_ERROR_CHECK(err);

    return cmd.rx_data[0];
}

float ADS7066::battery_voltage()
{
    return (float)read_on_the_fly(SENS[0]) / 65535.0 * 3.3 * 5.0;
}