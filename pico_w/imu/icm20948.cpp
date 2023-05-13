#include "icm20948.h"

// I2C address
static const uint8_t ADXL343_ADDR = 0x53;

// Registers
static const uint8_t REG_DEVID = 0x00;
static const uint8_t REG_POWER_CTL = 0x2D;
static const uint8_t REG_DATAX0 = 0x32;

// Other constants
static const uint8_t DEVID = 0xE5;
static const float SENSITIVITY_2G = 1.0 / 256;  // (g/LSB)
static const float EARTH_GRAVITY = 9.80665;     // Earth's gravity in [m/s^2]

// Public function definition
// Constructor for SPI mode
icm20948::icm20948(spiObject_t &spi)
{
    _spi = &spi;
    _commMode = CommMode::SPI_MODE;
}

// Constructor for I2C mode
icm20948::icm20948(i2cObject_t &i2c)
{
    _i2c = &i2c;
    _commMode = CommMode::I2C_MODE;
}

bool icm20948::begin(void)
{
    bool err = initializeHardware();
    if(!err)
    {
        printf("Failed to intialize IMU sensor!\n");
        return false;
    }

    printf("Intialize IMU sensor successfully!\n");

    return true;
}

// Get all available data from IMU sensor and do the float conversion as well
void icm20948::updateImuData()
{
    // Update data for all accelerometer, gyroscope, mag, temperature
    const uint8_t numbytes = 14 + 9; //Read Accel, gyro, temp, and 9 bytes of mag
    uint8_t rawData[numbytes];

    // Read raw accel data
    reg_reads(ACCEL_XOUT_H, &rawData[0], numbytes);
    accelRaw.x = (int16_t)(rawData[0] << 8) | rawData[1];
    accelRaw.y = (int16_t)(rawData[2] << 8) | rawData[3];
    accelRaw.z = (int16_t)(rawData[4] << 8) | rawData[5];

    // Read raw gyro data
    gyroRaw.x = (int16_t)(rawData[6] << 8) | rawData[7];
    gyroRaw.y = (int16_t)(rawData[8] << 8) | rawData[9];
    gyroRaw.z = (int16_t)(rawData[10] << 8) | rawData[11];

    // Read raw temperature data
    temperatureRaw = (int16_t)(rawData[12] << 8) | rawData[13];

    // Read raw magnetometer data
    // magRaw["stat1"] = rawData[14];
    magRaw.x = ((rawData[16] << 8) | (rawData[15] & 0xFF)); //Mag data is read little endian
    magRaw.y = ((rawData[18] << 8) | (rawData[17] & 0xFF));
    magRaw.z = ((rawData[20] << 8) | (rawData[19] & 0xFF));
    // magRaw["stat2"] = rawData[22];

    /*-----------Convert to meaningful value here----------*/
    accel.x = accelRaw.x*aRes*9.81;
    accel.y = accelRaw.y*aRes*9.81;
    accel.z = accelRaw.z*aRes*9.81;

    gyro.x = gyroRaw.x*gRes;
    gyro.y = gyroRaw.y*gRes;
    gyro.z = gyroRaw.z*gRes;

    temperature = temperatureRaw;

    mag.x = magRaw.x * mRes;
    mag.y = magRaw.y * mRes;
    mag.z = magRaw.z * mRes;
}

// Check if there is available data
bool icm20948::dataReady(void)
{
    bool ret = false;
    uint8_t val;
  
    // Switch to user bank 0
    reg_write(REG_BANK_SEL, 0x00);

    val = reg_read(INT_STATUS_1);
    if (val == 0x01)
    {
      ret = true;
    }
    
    return ret;
}

/******************************************************************************
* PRIVATE FUNCTIONS
*******************************************************************************/
// Hardware initialization for sensor configuration
bool icm20948::initializeHardware(void)
{
    if (_commMode == CommMode::SPI_MODE)
    {
        // Initialize CS pin high
        gpio_init(_spi->csPin);
        gpio_set_dir(_spi->csPin, GPIO_OUT);
        gpio_put(_spi->csPin, 1);

        // Initialize SPI port at 1 MHz
        spi_init(_spi->port, _spi->clkHz*1000);

        // Set SPI format
        spi_set_format( spi0,   // SPI instance
                        8,      // Number of bits per transfer
                        SPI_CPOL_1,      // Polarity (CPOL)
                        SPI_CPHA_1,      // Phase (CPHA)
                        SPI_MSB_FIRST);

        // Initialize SPI pins
        gpio_set_function(_spi->clkHz, GPIO_FUNC_SPI);
        gpio_set_function(_spi->mosiPin, GPIO_FUNC_SPI);
        gpio_set_function(_spi->misoPin, GPIO_FUNC_SPI);
    }
    else
    {
        i2c_init(_i2c->port, _i2c->clkHz * 1000);
        // Initialize I2C pins
        gpio_set_function(_i2c->sdaPin, GPIO_FUNC_I2C);
        gpio_set_function(_i2c->sclPin, GPIO_FUNC_I2C);
    }

    bool err = true;
    // Get stable time source
    // Auto select clock source to be PLL gyroscope reference if ready else
    reg_write(PWR_MGMT_1, 0x01);
    sleep_ms(200);
    
    // Switch to user bank 2
    reg_write(REG_BANK_SEL, 0x20);
    
    // Configure Gyro and Thermometer
    // Disable FSYNC and set gyro bandwidth to 51.2 Hz,
    // respectively;
    // minimum sleep_ms time for this setting is 5.9 ms, which means sensor fusion
    // update rates cannot be higher than 1 / 0.0059 = 170 Hz
    // DLPF_CFG = bits 2:0 = 011; this limits the sample rate to 1000 Hz for both
    // With the ICM20948, it is possible to get gyro sample rates of 32 kHz (!),
    // 8 kHz, or 1 kHz
    // Set gyroscope full scale range to 250 dps
    reg_write(GYRO_CONFIG_1, 0x19);
    reg_write(TEMP_CONFIG, 0x03);

    // Set sample rate = gyroscope output rate/(1 + GYRO_SMPLRT_DIV)
    // Use a 220 Hz rate; a rate consistent with the filter update rate
    // determined inset in CONFIG above.
    reg_write(GYRO_SMPLRT_DIV, 0x04);

    // Set gyroscope full scale range
    // Range selects FS_SEL and AFS_SEL are 0 - 3, so 2-bit values are
    // left-shifted into positions 4:3

    // Set accelerometer full-scale range configuration
    // Get current ACCEL_CONFIG register value
    uint8_t c = reg_read(ACCEL_CONFIG);
    // c = c & ~0xE0; // Clear self-test bits [7:5]
    c = c & ~0x06;  // Clear AFS bits [4:3]
    c = c | Ascale << 1; // Set full scale range for the accelerometer
    c = c | 0x01; // Set enable accel DLPF for the accelerometer
    c = c | 0x18; // and set DLFPFCFG to 50.4 hz
    // Write new ACCEL_CONFIG register value
    reg_write(ACCEL_CONFIG, c);

    // Set accelerometer sample rate configuration
    // It is possible to get a 4 kHz sample rate from the accelerometer by
    // choosing 1 for accel_fchoice_b bit [3]; in this case the bandwidth is
    // 1.13 kHz
    reg_write(ACCEL_SMPLRT_DIV_2, 0x04);

    // The accelerometer, gyro, and thermometer are set to 1 kHz sample rates,
    // but all these rates are further reduced by a factor of 5 to 200 Hz because
    // of the GYRO_SMPLRT_DIV setting
    // Switch to user bank 0
    reg_write(REG_BANK_SEL, 0x00);

    // Configure Interrupts and Bypass Enable
    // Set interrupt pin active high, push-pull, hold interrupt pin level HIGH
    // until interrupt cleared, clear on read of INT_STATUS, and enable
    // I2C_BYPASS_EN so additional chips can join the I2C bus and all can be
    // controlled by the MCU as master.
    reg_write(INT_PIN_CFG, 0x22);
    // Enable data ready (bit 0) interrupt
    // reg_write(INT_ENABLE_1, 0x01);

    updateMagResolution();
    updateAccelResolution();
    updateGyroResolution();

    // Temporarily bypass the error check from digital communication (I2C, SPI)
    return err;
}

void icm20948::updateMagResolution()
{
    // TODO: define a way to read back the scale value from the sensor
    mRes = 10.0f * 4912.0f / 32760.0f; // Proper scale to return milliGauss
}

void icm20948::updateAccelResolution()
{
    // TODO: define a way to read back the scale value from the sensor
    switch (Gscale)
    {
        // Possible gyro scales (and their register bit settings) are:
        // 250 DPS (00), 500 DPS (01), 1000 DPS (10), and 2000 DPS (11).
        // Here's a bit of an algorith to calculate DPS/(ADC tick) based on that
        // 2-bit value:
        case GFS_250DPS:
          gRes = 250.0f / 32768.0f;
          break;
        case GFS_500DPS:
          gRes = 500.0f / 32768.0f;
          break;
        case GFS_1000DPS:
          gRes = 1000.0f / 32768.0f;
          break;
        case GFS_2000DPS:
          gRes = 2000.0f / 32768.0f;
          break;
    }
}

void icm20948::updateGyroResolution()
{
    // TODO: define a way to read back the scale value from the sensor
    switch (Ascale)
    {
      // Possible accelerometer scales (and their register bit settings) are:
      // 2 Gs (00), 4 Gs (01), 8 Gs (10), and 16 Gs  (11).
      // Here's a bit of an algorith to calculate DPS/(ADC tick) based on that
      // 2-bit value:
      case AFS_2G:
        aRes = 2.0f / 32768.0f;
        break;
      case AFS_4G:
        aRes = 4.0f / 32768.0f;
        break;
      case AFS_8G:
        aRes = 8.0f / 32768.0f;
        break;
      case AFS_16G:
        aRes = 16.0f / 32768.0f;
        break;
    }
}


// Function which accumulates gyro and accelerometer data after device
// initialization. It calculates the average of the at-rest readings and then
// loads the resulting offsets into accelerometer and gyro bias registers.
void icm20948::calibrateICM20948(float * gyroBias, float * accelBias)
{
    uint8_t data[12]; // data array to hold accelerometer and gyro x, y, z, data
    uint16_t ii, packet_count, fifo_count;
    int32_t gyro_bias[3]  = {0, 0, 0}, accel_bias[3] = {0, 0, 0};

    // reset device
    // Write a one to bit 7 reset bit; toggle reset device
    reg_write(PWR_MGMT_1, READ_FLAG);
    sleep_ms(200);

    // get stable time source; Auto select clock source to be PLL gyroscope
    // reference if ready else use the internal oscillator, bits 2:0 = 001
    reg_write(PWR_MGMT_1, 0x01);
    sleep_ms(200);

    // Configure device for bias calculation
    // Disable all interrupts
    reg_write(INT_ENABLE, 0x00);
    // Disable FIFO
    reg_write(FIFO_EN_1, 0x00);
    reg_write(FIFO_EN_2, 0x00);
    // Turn on internal clock source
    reg_write(PWR_MGMT_1, 0x00);
    // Disable I2C master
    //reg_write(I2C_MST_CTRL, 0x00); Already disabled
    // Disable FIFO and I2C master modes
    reg_write(USER_CTRL, 0x00);
    // Reset FIFO and DMP
    reg_write(USER_CTRL, 0x08);
    reg_write(FIFO_RST, 0x1F);
    sleep_ms(10);
    reg_write(FIFO_RST, 0x00);
    sleep_ms(15);

    // Set FIFO mode to snapshot
    reg_write(FIFO_MODE, 0x1F);
    // Switch to user bank 2
    reg_write(REG_BANK_SEL, 0x20);
    // Configure ICM20948 gyro and accelerometer for bias calculation
    // Set low-pass filter to 188 Hz
    reg_write(GYRO_CONFIG_1, 0x01);
    // Set sample rate to 1 kHz
    reg_write(GYRO_SMPLRT_DIV, 0x00);
    // Set gyro full-scale to 250 degrees per second, maximum sensitivity
    reg_write(GYRO_CONFIG_1, 0x00);
    // Set accelerometer full-scale to 2 g, maximum sensitivity
    reg_write(ACCEL_CONFIG, 0x00);

    uint16_t  gyrosensitivity  = 131;   // = 131 LSB/degrees/sec
    uint16_t  accelsensitivity = 16384; // = 16384 LSB/g

    // Switch to user bank 0
    reg_write(REG_BANK_SEL, 0x00);
    // Configure FIFO to capture accelerometer and gyro data for bias calculation
    reg_write(USER_CTRL, 0x40);  // Enable FIFO
    // Enable gyro and accelerometer sensors for FIFO  (max size 512 bytes in
    // ICM20948)
    reg_write(FIFO_EN_2, 0x1E);
    sleep_ms(40);  // accumulate 40 samples in 40 milliseconds = 480 bytes

    // At end of sample accumulation, turn off FIFO sensor read
    // Disable gyro and accelerometer sensors for FIFO
    reg_write(FIFO_EN_2, 0x00);
    // Read FIFO sample count
    reg_reads(FIFO_COUNTH, &data[0], 2);
    fifo_count = ((uint16_t)data[0] << 8) | data[1];
    // How many sets of full gyro and accelerometer data for averaging
    packet_count = fifo_count/12;

    for (ii = 0; ii < packet_count; ii++)
    {
        int16_t accel_temp[3] = {0, 0, 0}, gyro_temp[3] = {0, 0, 0};
        // Read data for averaging
        reg_reads(FIFO_R_W, &data[0], 12);
        // Form signed 16-bit integer for each sample in FIFO
        accel_temp[0] = (int16_t) (((int16_t)data[0] << 8) | data[1]  );
        accel_temp[1] = (int16_t) (((int16_t)data[2] << 8) | data[3]  );
        accel_temp[2] = (int16_t) (((int16_t)data[4] << 8) | data[5]  );
        gyro_temp[0]  = (int16_t) (((int16_t)data[6] << 8) | data[7]  );
        gyro_temp[1]  = (int16_t) (((int16_t)data[8] << 8) | data[9]  );
        gyro_temp[2]  = (int16_t) (((int16_t)data[10] << 8) | data[11]);

        // Sum individual signed 16-bit biases to get accumulated signed 32-bit
        // biases.
        accel_bias[0] += (int32_t) accel_temp[0];
        accel_bias[1] += (int32_t) accel_temp[1];
        accel_bias[2] += (int32_t) accel_temp[2];
        gyro_bias[0]  += (int32_t) gyro_temp[0];
        gyro_bias[1]  += (int32_t) gyro_temp[1];
        gyro_bias[2]  += (int32_t) gyro_temp[2];
    }
    // Sum individual signed 16-bit biases to get accumulated signed 32-bit biases
    accel_bias[0] /= (int32_t) packet_count;
    accel_bias[1] /= (int32_t) packet_count;
    accel_bias[2] /= (int32_t) packet_count;
    gyro_bias[0]  /= (int32_t) packet_count;
    gyro_bias[1]  /= (int32_t) packet_count;
    gyro_bias[2]  /= (int32_t) packet_count;

    // Sum individual signed 16-bit biases to get accumulated signed 32-bit biases
    if (accel_bias[2] > 0L)
    {
        accel_bias[2] -= (int32_t) accelsensitivity;
    }
    else
    {
        accel_bias[2] += (int32_t) accelsensitivity;
    }

    // Construct the gyro biases for push to the hardware gyro bias registers,
    // which are reset to zero upon device startup.
    // Divide by 4 to get 32.9 LSB per deg/s to conform to expected bias input
    // format.
    data[0] = (-gyro_bias[0]/4  >> 8) & 0xFF;
    // Biases are additive, so change sign on calculated average gyro biases
    data[1] = (-gyro_bias[0]/4)       & 0xFF;
    data[2] = (-gyro_bias[1]/4  >> 8) & 0xFF;
    data[3] = (-gyro_bias[1]/4)       & 0xFF;
    data[4] = (-gyro_bias[2]/4  >> 8) & 0xFF;
    data[5] = (-gyro_bias[2]/4)       & 0xFF;
    
    // Switch to user bank 2
    reg_write(REG_BANK_SEL, 0x20);

    // Push gyro biases to hardware registers
    reg_write(XG_OFFSET_H, data[0]);
    reg_write(XG_OFFSET_L, data[1]);
    reg_write(YG_OFFSET_H, data[2]);
    reg_write(YG_OFFSET_L, data[3]);
    reg_write(ZG_OFFSET_H, data[4]);
    reg_write(ZG_OFFSET_L, data[5]);

    // Output scaled gyro biases for display in the main program
    gyroBias[0] = (float) gyro_bias[0]/(float) gyrosensitivity;
    gyroBias[1] = (float) gyro_bias[1]/(float) gyrosensitivity;
    gyroBias[2] = (float) gyro_bias[2]/(float) gyrosensitivity;

    // Construct the accelerometer biases for push to the hardware accelerometer
    // bias registers. These registers contain factory trim values which must be
    // added to the calculated accelerometer biases; on boot up these registers
    // will hold non-zero values. In addition, bit 0 of the lower byte must be
    // preserved since it is used for temperature compensation calculations.
    // Accelerometer bias registers expect bias input as 2048 LSB per g, so that
    // the accelerometer biases calculated above must be divided by 8.
    
    // Switch to user bank 1
    reg_write(REG_BANK_SEL, 0x10);
    // A place to hold the factory accelerometer trim biases
    int32_t accel_bias_reg[3] = {0, 0, 0};
    // Read factory accelerometer trim values
    reg_reads(XA_OFFSET_H, &data[0], 2);
    accel_bias_reg[0] = (int32_t) (((int16_t)data[0] << 8) | data[1]);
    reg_reads(YA_OFFSET_H, &data[0], 2);
    accel_bias_reg[1] = (int32_t) (((int16_t)data[0] << 8) | data[1]);
    reg_reads(ZA_OFFSET_H, &data[0], 2);
    accel_bias_reg[2] = (int32_t) (((int16_t)data[0] << 8) | data[1]);

    // Define mask for temperature compensation bit 0 of lower byte of
    // accelerometer bias registers
    uint32_t mask = 1uL;
    // Define array to hold mask bit for each accelerometer bias axis
    uint8_t mask_bit[3] = {0, 0, 0};

    for (ii = 0; ii < 3; ii++)
    {
        // If temperature compensation bit is set, record that fact in mask_bit
        if ((accel_bias_reg[ii] & mask))
        {
            mask_bit[ii] = 0x01;
        }
    }

    // Construct total accelerometer bias, including calculated average
    // accelerometer bias from above
    // Subtract calculated averaged accelerometer bias scaled to 2048 LSB/g
    // (16 g full scale)
    accel_bias_reg[0] -= (accel_bias[0]/8);
    accel_bias_reg[1] -= (accel_bias[1]/8);
    accel_bias_reg[2] -= (accel_bias[2]/8);

    data[0] = (accel_bias_reg[0] >> 8) & 0xFF;
    data[1] = (accel_bias_reg[0])      & 0xFF;
    // preserve temperature compensation bit when writing back to accelerometer
    // bias registers
    data[1] = data[1] | mask_bit[0];
    data[2] = (accel_bias_reg[1] >> 8) & 0xFF;
    data[3] = (accel_bias_reg[1])      & 0xFF;
    // Preserve temperature compensation bit when writing back to accelerometer
    // bias registers
    data[3] = data[3] | mask_bit[1];
    data[4] = (accel_bias_reg[2] >> 8) & 0xFF;
    data[5] = (accel_bias_reg[2])      & 0xFF;
    // Preserve temperature compensation bit when writing back to accelerometer
    // bias registers
    data[5] = data[5] | mask_bit[2];

    // Apparently this is not working for the acceleration biases in the ICM-20948
    // Are we handling the temperature correction bit properly?
    // Push accelerometer biases to hardware registers
    reg_write(XA_OFFSET_H, data[0]);
    reg_write(XA_OFFSET_L, data[1]);
    reg_write(YA_OFFSET_H, data[2]);
    reg_write(YA_OFFSET_L, data[3]);
    reg_write(ZA_OFFSET_H, data[4]);
    reg_write(ZA_OFFSET_L, data[5]);

    // Output scaled accelerometer biases for display in the main program
    accelBias[0] = (float)accel_bias[0]/(float)accelsensitivity;
    accelBias[1] = (float)accel_bias[1]/(float)accelsensitivity;
    accelBias[2] = (float)accel_bias[2]/(float)accelsensitivity;
    // Switch to user bank 0
    reg_write(REG_BANK_SEL, 0x00);
}

// Private function definition
// Write 1 byte to the specified register
int icm20948::reg_write_i2c(i2c_inst_t *i2c, 
                    const uint addr, 
                    const uint8_t reg, 
                    uint8_t *buf,
                    const uint8_t nbytes)
{
    int num_bytes_read = 0;
    uint8_t msg[nbytes + 1];

    // Check to make sure caller is sending 1 or more bytes
    if (nbytes < 1) 
    {
        return 0;
    }

    // Append register address to front of data packet
    msg[0] = reg;
    for (int i = 0; i < nbytes; i++) 
    {
        msg[i + 1] = buf[i];
    }

    // Write data to register(s) over I2C
    i2c_write_blocking(i2c, addr, msg, (nbytes + 1), false);

    return num_bytes_read;
}


int icm20948::reg_read_i2c(i2c_inst_t *i2c,
                    const uint addr,
                    const uint8_t reg,
                    uint8_t *buf,
                    const uint8_t nbytes)
{
    int num_bytes_read = 0;

    // Check to make sure caller is asking for 1 or more bytes
    if (nbytes < 1)
    {
        return 0;
    }

    // Read data from register(s) over I2C
    i2c_write_blocking(i2c, addr, &reg, 1, true);
    num_bytes_read = i2c_read_blocking(i2c, addr, buf, nbytes, false);

    return num_bytes_read;
}


// Write 1 byte to the specified register 
int icm20948::reg_write_spi(spi_inst_t *spi, 
                const uint cs, 
                const uint8_t reg, 
                const uint8_t *data,
                const uint8_t nbytes)
{
    // Just define maximum bytes to read here
    uint8_t msg[128];

    // Check if nbytes is within the limit
    if (nbytes > 126) {
        return -1; // Error: too many bytes
    }
                
    // Construct message (set ~W bit low, MB bit low)
    msg[0] = 0x00 | reg;

    // Copy data to the message buffer
    memcpy(msg + 1, data, nbytes);

    // Write to register
    gpio_put(cs, 0);
    spi_write_blocking(spi, msg, nbytes + 1);
    gpio_put(cs, 1);

    return 0;
}

// Read byte(s) from specified register. If nbytes > 1, read from consecutive
// registers.
int icm20948::reg_read_spi(spi_inst_t *spi,
                const uint cs,
                const uint8_t reg,
                uint8_t *buf,
                const uint8_t nbytes)
{
    int num_bytes_read = 0;
    uint8_t mb = 0;

    // Determine if multiple byte (MB) bit should be set
    if (nbytes < 1) 
    {
        return -1;
    } 
    else if (nbytes == 1) 
    {
        mb = 0;
    } 
    else 
    {
        mb = 1;
    }

    // Construct message (set ~W bit high)
    uint8_t msg = 0x80 | (mb << 6) | reg;

    // Read from register
    gpio_put(cs, 0);
    spi_write_blocking(spi, &msg, 1);
    num_bytes_read = spi_read_blocking(spi, 0, buf, nbytes);
    gpio_put(cs, 1);

    return num_bytes_read;
}


// Common function to write a byte to the corresponding register
void icm20948::reg_write(uint8_t reg, uint8_t data) 
{
    if(_commMode == CommMode::SPI_MODE) 
    {
        reg_write_spi(_spi->port, _spi->csPin, reg, &data, 1);
    } 
    else if(_commMode == CommMode::I2C_MODE) 
    {
        uint8_t buf[] = {data};
        reg_write_i2c(_i2c->port, _i2c->i2cAddress, reg, buf, 1);
    }
}

// Common function to write a byte to the corresponding register
void icm20948::reg_writes(uint8_t reg, uint8_t *data, uint16_t len) 
{
    if(_commMode == CommMode::SPI_MODE) 
    {
        reg_write_spi(_spi->port, _spi->csPin, reg, data, len);
    } 
    else if(_commMode == CommMode::I2C_MODE) 
    {
        reg_write_i2c(_i2c->port, _i2c->i2cAddress, reg, data, len);
    }
}

// Common function to read a byte from the corresponding register
uint8_t icm20948::reg_read(uint8_t reg) 
{
    uint8_t data = 0;

    if(_commMode == CommMode::SPI_MODE) 
    {
        reg_read_spi(_spi->port, _spi->csPin, reg, &data, 1);
    } 
    else if(_commMode == CommMode::I2C_MODE) 
    {
        reg_read_i2c(_i2c->port, _i2c->i2cAddress, reg, &data, 1);
    }

    return data;
}

void icm20948::reg_reads(uint8_t reg, uint8_t *data, uint16_t len) 
{
    if(_commMode == CommMode::SPI_MODE) 
    {
        reg_read_spi(_spi->port, _spi->csPin, reg, data, len);
    } 
    else if(_commMode == CommMode::I2C_MODE) 
    {
        reg_read_i2c(_i2c->port, _i2c->i2cAddress, reg, data, len);
    }
}
