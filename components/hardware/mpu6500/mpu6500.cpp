#include "mpu6500.hpp"

MPU6500::MPU6500(spi_host_device_t bus, gpio_num_t cs){
    // デバイスの初期化
    memset(&dev_imu,0,sizeof(dev_imu));
    dev_imu.clock_speed_hz = 7*1000*1000;
    dev_imu.mode = 3;
    dev_imu.spics_io_num = cs;
    dev_imu.queue_size = 1;

    err = spi_bus_add_device(bus,&dev_imu,&_spi);
    ESP_ERROR_CHECK(err);

    write(0x6B,0b10000000);
    write(0x6B,0b00001001); //温度センサ無効

    uint8_t who = whoami();
    if(who != MPU6500_WHO_AM_I){ //MPU6500 : read(0x75)  ICM20648 : read(0x00)
        ESP_LOGW("MPU6500", "WHOAMI is not match %x:%x", MPU6500_WHO_AM_I, who);
        while (1) {} // Add a placeholder statement
        return;
    }
    
    if(changesens(3,1) == -1){
        while (1) {} // Add a placeholder statement
    }

    ESP_LOGI("MPU6500", "Initialized");
}

MPU6500::~MPU6500(){}

uint8_t MPU6500::read(uint8_t reg){
    spi_transaction_t cmd;

    memset(&cmd,0,sizeof(cmd));
    cmd.flags = SPI_TRANS_USE_TXDATA | SPI_TRANS_USE_RXDATA;
    cmd.length = 16;
    cmd.tx_data[0] = (reg | MPU6500_READ_FLAG);
    err = spi_device_polling_transmit(_spi,&cmd);
    ESP_ERROR_CHECK(err);

    return cmd.rx_data[1];
}

uint16_t MPU6500::read16(uint8_t reg){
    spi_transaction_t cmd;

    memset(&cmd,0,sizeof(cmd));
    cmd.flags = SPI_TRANS_USE_TXDATA | SPI_TRANS_USE_RXDATA;
    cmd.length = 24;
    cmd.tx_data[0] = (reg | MPU6500_READ_FLAG);
    err = spi_device_polling_transmit(_spi,&cmd);
    ESP_ERROR_CHECK(err);

    return cmd.rx_data[1] << 8 | cmd.rx_data[2];
}


void MPU6500::write(uint8_t reg,uint8_t data){
    esp_err_t err;
    spi_transaction_t cmd;

    memset(&cmd,0,sizeof(cmd));
    cmd.flags = SPI_TRANS_USE_TXDATA | SPI_TRANS_USE_RXDATA;
    cmd.length = 16;
    cmd.tx_data[0] = reg;
    cmd.tx_data[1] = data;
    err = spi_device_polling_transmit(_spi,&cmd);
    assert(err == ESP_OK);
    //Delay挟まないとうまくいかないことがある
    vTaskDelay(10/portTICK_PERIOD_MS);
    return;
}



int MPU6500::changesens(uint8_t _gyro,uint8_t _accel){
    if(_gyro > 0b11 || _accel > 0b11) return -1;

    write(GYRO_CONFIG,_gyro << GYRO_FS_SEL);
    write(ACCEL_CONFIG,_accel << ACCEL_FS_SEL);

    bool _verify = ((read(GYRO_CONFIG) >> GYRO_FS_SEL) & 0b11) == _gyro 
    && ((read(ACCEL_CONFIG) >> ACCEL_FS_SEL) & 0b11) == _accel;


    if(!_verify) return -1;
    
    switch (_gyro){
    case 0:
        gyro_sensitivity = 250.0 /32768.;
        break;
    case 1:
        gyro_sensitivity = 500.0 /32768.;
        break;
    case 2:
        gyro_sensitivity = 1000.0/32768.;
        break;
    case 3:
        gyro_sensitivity = 2000.0 /32768.;
        break;
    default:
        break;
    }
    switch (_accel){
    case 0:
        accel_sensitivity = 2.0 / 32768.;
        break;
    case 1:
        accel_sensitivity = 4.0 / 32768.;
        break;
    case 2:
        accel_sensitivity = 8.0 / 32768.;
        break;
    case 3:
        accel_sensitivity = 16.0 / 32768.;
        break;
    
    default:
        break;
    }

    return 0;
}

float MPU6500::surveybias(int reftime){
    in_survaeybias = true; //  バイアスサーベイ中フラグを立てる
    vTaskDelay(1000/portTICK_PERIOD_MS); //  100ms待つ
    float r_yaw_ref_tmp = 0;
    for(uint16_t i = 0; i < reftime; i++){
        r_yaw_ref_tmp += gyro_z();
        vTaskDelay(1/portTICK_PERIOD_MS);
    }
    in_survaeybias = false; 
    return (float)(r_yaw_ref_tmp / reftime);
}

float MPU6500::surveybias_accel_y(int reftime){
    in_survaeybias = true; //  バイアスサーベイ中フラグを立てる
    vTaskDelay(1000/portTICK_PERIOD_MS); //  1000ms待つ（静止を確実にする）
    float accel_y_ref_tmp = 0;
    for(uint16_t i = 0; i < reftime; i++){
        // バイアス補正前の生の値を使用（accel_y()ではなくaccelY_raw()を使用）
        accel_y_ref_tmp += (float)accel_y_raw() * accel_sensitivity;
        vTaskDelay(1/portTICK_PERIOD_MS);
    }
    in_survaeybias = false; 
    return (float)(accel_y_ref_tmp / reftime);
}

uint8_t MPU6500::whoami(){
    return read(0x75);  //  0x00
}
int16_t MPU6500::accel_x_raw(){
    return read16(0x3B);    //  0x3D
}
int16_t MPU6500::accel_y_raw(){
    return read16(0x3D);    //  0x3F
}
int16_t MPU6500::accel_z_raw(){
    return read16(0x3F);    //  0x41
}
int16_t MPU6500::gyro_x_raw(){
    return read16(0x43);    //  0x43
}
int16_t MPU6500::gyro_y_raw(){
    return read16(0x45);    //  0x45
}
int16_t MPU6500::gyro_z_raw(){
    return read16(0x47);    //  0x47
}
float MPU6500::accel_x(){
    return (float)accel_x_raw() * accel_sensitivity;
}
float MPU6500::accel_y(){
    return (float)accel_y_raw() * accel_sensitivity;
}
float MPU6500::accel_z(){
    return (float)accel_z_raw() * accel_sensitivity;
}
float MPU6500::gyro_x(){
    return (float)gyro_x_raw() * gyro_sensitivity;
}
float MPU6500::gyro_y(){
    return (float)gyro_y_raw() * gyro_sensitivity;
}
float MPU6500::gyro_z(){
    return (float)gyro_z_raw() * gyro_sensitivity;
}

void MPU6500::share_sensor_data(SensorData *_sens){
    sens = _sens;
}