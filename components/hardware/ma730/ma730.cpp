#include "ma730.hpp"

MA730::MA730(spi_host_device_t bus, gpio_num_t cs, uint8_t ccw)
{
    // SPIデバイスの設定
    memset(&dev_enc, 0, sizeof(dev_enc));
    dev_enc.clock_speed_hz = 25 * 1000 * 1000; // Max 25MHz
    dev_enc.mode = 3;
    dev_enc.spics_io_num = cs;
    dev_enc.queue_size = 1;

    err = spi_bus_add_device(bus, &dev_enc, &_spi);
    ESP_ERROR_CHECK(err); 

    if((read_register(ADRS_Rotation_direction, 0x00) & 0x80) >> 7 != ccw)
    {
        write_register(ADRS_Rotation_direction, ccw << 7);
    }

    set_filter(FCUTOFF_HZ_6000);

    ESP_LOGI("MA730", "CS:%d Initialized", cs);
}

MA730::~MA730() {}

uint16_t MA730::read()
{
    memset(&cmd, 0, sizeof(cmd));
    cmd.flags = SPI_TRANS_USE_TXDATA | SPI_TRANS_USE_RXDATA;
    cmd.length = 16;
    cmd.tx_data[0] = 0;
    cmd.tx_data[1] = 0;
    err = spi_device_polling_transmit(_spi, &cmd);
    assert(err == ESP_OK);

    return cmd.rx_data[0] << 8 | cmd.rx_data[1];
}

uint16_t MA730::read_angle()
{
    return read() >> 2;
}

uint16_t MA730::operate_registers(const uint8_t command, const uint8_t address, const uint8_t data)
{
    memset(&cmd, 0, sizeof(cmd));
    cmd.flags = SPI_TRANS_USE_TXDATA | SPI_TRANS_USE_RXDATA;
    cmd.length = 16;
    cmd.tx_data[0] = (command << 5) | address;
    cmd.tx_data[1] = data;
    err = spi_device_polling_transmit(_spi, &cmd);
    assert(err == ESP_OK);

    return cmd.rx_data[0] << 8 | cmd.rx_data[1];
}

uint8_t MA730::read_register(uint8_t address, uint8_t data)
{
    operate_registers(READ_COMMAND, address, data);
    return read() >> 8;
}

uint8_t MA730::write_register(uint8_t address, uint8_t data)
{
    operate_registers(WRITE_COMMAND, address, data);
    vTaskDelay(pdMS_TO_TICKS(20));
    return read() >> 8;
}

// フィルタ時定数の設定 (FW レジスタ 0xE に書き込む)
void MA730::set_filter(uint8_t fw_value)
{
    write_register(0xE, fw_value);
    vTaskDelay(pdMS_TO_TICKS(12)); // 安定化のための待機
    ESP_LOGI("MA730", "Filter set to FW: %d", fw_value);
}

void MA730::share_sensor_data(SensorData *_sens)
{
    sens = _sens;
}