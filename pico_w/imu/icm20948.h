#ifndef _ICM20948_CPP_H_
#define _ICM20948_CPP_H_

#include <iostream>
#include <cstdint>
#include <cstring>
#include <map>
#include <string>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/spi.h"

// See also ICM-20948 Datasheet, Register Map and Descriptions, Revision 1.3,
// https://www.invensense.com/wp-content/uploads/2016/06/DS-000189-ICM-20948-v1.3.pdf
// and AK09916 Datasheet and Register Map
// https://www.akm.com/akm/en/file/datasheet/AK09916C.pdf

//Magnetometer Registers
#define AK09916_ADDRESS  0x0C 
#define WHO_AM_I_AK09916 0x01 // (AKA WIA2) should return 0x09
#define AK09916_ST1      0x10  // data ready status bit 0
#define AK09916_XOUT_L   0x11  // data
#define AK09916_XOUT_H   0x12
#define AK09916_YOUT_L   0x13
#define AK09916_YOUT_H   0x14
#define AK09916_ZOUT_L   0x15
#define AK09916_ZOUT_H   0x16
#define AK09916_ST2      0x18  // Data overflow bit 3 and data read error status bit 2
#define AK09916_CNTL     0x30  // Power down (0000), single-measurement (0001), self-test (1000) and Fuse ROM (1111) modes on bits 3:0
#define AK09916_CNTL2    0x31  // Normal (0), Reset (1)

// ICM-20948

// USER BANK 0 REGISTER MAP
#define WHO_AM_I_ICM20948  0x00 // Should return 0xEA
#define USER_CTRL          0x03  // Bit 7 enable DMP, bit 3 reset DMP
#define LP_CONFIG		   0x05 // Not found in MPU-9250
#define PWR_MGMT_1         0x06 // Device defaults to the SLEEP mode
#define PWR_MGMT_2         0x07
#define INT_PIN_CFG        0x0F
#define INT_ENABLE         0x10
#define INT_ENABLE_1	   0x11 // Not found in MPU-9250
#define INT_ENABLE_2	   0x12 // Not found in MPU-9250
#define INT_ENABLE_3	   0x13 // Not found in MPU-9250
#define I2C_MST_STATUS     0x17
#define INT_STATUS         0x19
#define INT_STATUS_1	   0x1A // Not found in MPU-9250
#define INT_STATUS_2	   0x1B // Not found in MPU-9250
#define INT_STATUS_3	   0x1C // Not found in MPU-9250
#define DELAY_TIMEH		   0x28	// Not found in MPU-9250
#define DELAY_TIMEL		   0x29	// Not found in MPU-9250
#define ACCEL_XOUT_H       0x2D
#define ACCEL_XOUT_L       0x2E
#define ACCEL_YOUT_H       0x2F
#define ACCEL_YOUT_L       0x30
#define ACCEL_ZOUT_H       0x31
#define ACCEL_ZOUT_L       0x32
#define GYRO_XOUT_H        0x33
#define GYRO_XOUT_L        0x34
#define GYRO_YOUT_H        0x35
#define GYRO_YOUT_L        0x36
#define GYRO_ZOUT_H        0x37
#define GYRO_ZOUT_L        0x38
#define TEMP_OUT_H         0x39
#define TEMP_OUT_L         0x3A
#define EXT_SENS_DATA_00   0x3B
#define EXT_SENS_DATA_01   0x3C
#define EXT_SENS_DATA_02   0x3D
#define EXT_SENS_DATA_03   0x3E
#define EXT_SENS_DATA_04   0x3F
#define EXT_SENS_DATA_05   0x40
#define EXT_SENS_DATA_06   0x41
#define EXT_SENS_DATA_07   0x42
#define EXT_SENS_DATA_08   0x43
#define EXT_SENS_DATA_09   0x44
#define EXT_SENS_DATA_10   0x45
#define EXT_SENS_DATA_11   0x46
#define EXT_SENS_DATA_12   0x47
#define EXT_SENS_DATA_13   0x48
#define EXT_SENS_DATA_14   0x49
#define EXT_SENS_DATA_15   0x4A
#define EXT_SENS_DATA_16   0x4B
#define EXT_SENS_DATA_17   0x4C
#define EXT_SENS_DATA_18   0x4D
#define EXT_SENS_DATA_19   0x4E
#define EXT_SENS_DATA_20   0x4F
#define EXT_SENS_DATA_21   0x50
#define EXT_SENS_DATA_22   0x51
#define EXT_SENS_DATA_23   0x52
#define FIFO_EN_1          0x66
#define FIFO_EN_2          0x67 // Not found in MPU-9250
#define FIFO_RST		   0x68 // Not found in MPU-9250
#define FIFO_MODE		   0x69 // Not found in MPU-9250
#define FIFO_COUNTH        0x70
#define FIFO_COUNTL        0x71
#define FIFO_R_W           0x72
#define DATA_RDY_STATUS	   0x74 // Not found in MPU-9250
#define FIFO_CFG		   0x76 // Not found in MPU-9250
#define REG_BANK_SEL	   0x7F // Not found in MPU-9250

// USER BANK 1 REGISTER MAP
#define SELF_TEST_X_GYRO  			0x02
#define SELF_TEST_Y_GYRO  			0x03
#define SELF_TEST_Z_GYRO  			0x04
#define SELF_TEST_X_ACCEL 			0x0E
#define SELF_TEST_Y_ACCEL 			0x0F
#define SELF_TEST_Z_ACCEL 			0x10
#define XA_OFFSET_H       			0x14
#define XA_OFFSET_L       			0x15
#define YA_OFFSET_H       			0x17
#define YA_OFFSET_L       			0x18
#define ZA_OFFSET_H       			0x1A
#define ZA_OFFSET_L       			0x1B
#define TIMEBASE_CORRECTION_PLL		0x28

// USER BANK 2 REGISTER MAP
#define GYRO_SMPLRT_DIV        	0x00 // Not found in MPU-9250
#define GYRO_CONFIG_1      		0x01 // Not found in MPU-9250
#define GYRO_CONFIG_2      		0x02 // Not found in MPU-9250
#define XG_OFFSET_H       		0x03  // User-defined trim values for gyroscope
#define XG_OFFSET_L       		0x04
#define YG_OFFSET_H       		0x05
#define YG_OFFSET_L       		0x06
#define ZG_OFFSET_H       		0x07
#define ZG_OFFSET_L       		0x08
#define ODR_ALIGN_EN			0x09 // Not found in MPU-9250
#define ACCEL_SMPLRT_DIV_1     	0x10 // Not found in MPU-9250
#define ACCEL_SMPLRT_DIV_2     	0x11 // Not found in MPU-9250
#define ACCEL_INTEL_CTRL		0x12 // Not found in MPU-9250
#define ACCEL_WOM_THR			0x13 // Not found in MPU-9250 (could be WOM_THR)
#define ACCEL_CONFIG      		0x14
#define ACCEL_CONFIG_2     		0x15 // Not found in MPU-9250 (could be ACCEL_CONFIG2)
#define FSYNC_CONFIG			0x52 // Not found in MPU-9250
#define TEMP_CONFIG				0x53 // Not found in MPU-9250
#define MOD_CTRL_USR			0x54 // Not found in MPU-9250

// USER BANK 3 REGISTER MAP
#define I2C_MST_ODR_CONFIG		0x00 // Not found in MPU-9250
#define I2C_MST_CTRL       		0x01
#define I2C_MST_DELAY_CTRL 		0x02
#define I2C_SLV0_ADDR      		0x03
#define I2C_SLV0_REG       		0x04
#define I2C_SLV0_CTRL      		0x05
#define I2C_SLV0_DO        		0x06
#define I2C_SLV1_ADDR      		0x07
#define I2C_SLV1_REG       		0x08
#define I2C_SLV1_CTRL      		0x09
#define I2C_SLV1_DO        		0x0A
#define I2C_SLV2_ADDR      		0x0B
#define I2C_SLV2_REG       		0x0C
#define I2C_SLV2_CTRL      		0x0D
#define I2C_SLV2_DO        		0x0E
#define I2C_SLV3_ADDR      		0x0F
#define I2C_SLV3_REG       		0x10
#define I2C_SLV3_CTRL      		0x11
#define I2C_SLV3_DO        		0x12
#define I2C_SLV4_ADDR      		0x13
#define I2C_SLV4_REG       		0x14
#define I2C_SLV4_CTRL      		0x15
#define I2C_SLV4_DO        		0x16
#define I2C_SLV4_DI        		0x17


// Using the ICM-20948 breakout board, ADO is set to 1
// Seven-bit device address is 1000100 for ADO = 0 and 1000101 for ADO = 1
#define ADO 1
#if ADO
#define ICM20948_ADDRESS 0x69  // Device address when ADO = 1
#else
#define ICM20948_ADDRESS 0x68  // Device address when ADO = 0
#define AK09916_ADDRESS  0x0C   // Address of magnetometer
#endif // AD0

#define READ_FLAG 0x80

enum class CommMode {
    SPI_MODE,
    I2C_MODE,
};

typedef struct {
    uint csPin;
    uint sckPin;
    uint mosiPin;
    uint misoPin;
    uint16_t clkHz;
    spi_inst_t *port;
} spiObject_t;

typedef struct {
    uint sdaPin;
    uint sclPin;
    uint8_t i2cAddress;
    uint16_t clkHz;
    i2c_inst_t *port;
} i2cObject_t;

typedef struct {
    int16_t x;
    int16_t y;
    int16_t z;
} accelRaw_t;

typedef struct {
    int16_t x;
    int16_t y;
    int16_t z;
} gyroRaw_t;

typedef struct {
    int16_t x;
    int16_t y;
    int16_t z;
} magRaw_t;

typedef struct {
    float x;
    float y;
    float z;
} accel_t;

typedef struct {
    float x;
    float y;
    float z;
} gyro_t;

typedef struct {
    float x;
    float y;
    float z;
} mag_t;

class icm20948
{
  protected:
    // Set initial input parameters
    enum Ascale
    {
      AFS_2G = 0,
      AFS_4G,
      AFS_8G,
      AFS_16G
    };

    enum Gscale {
      GFS_250DPS = 0,
      GFS_500DPS,
      GFS_1000DPS,
      GFS_2000DPS
    };

    enum Mscale {
      MFS_14BITS = 0, // 0.6 mG per LSB
      MFS_16BITS      // 0.15 mG per LSB
    };

    enum M_MODE {
      M_8HZ = 0x02,  // 8 Hz update
      M_100HZ = 0x06 // 100 Hz continuous magnetometer
    };

    // TODO: Add setter methods for this hard coded stuff
    // Specify sensor full scale
    uint8_t Gscale = GFS_250DPS;
    uint8_t Ascale = AFS_2G;
    // 2 for 8 Hz, 6 for 100 Hz continuous magnetometer data read
    uint8_t Mmode = M_100HZ;


private:
    uint8_t default_i2c_address = 0x69;

    accel_t accel;
    gyro_t gyro;
    mag_t mag;
    float temperature;

    accelRaw_t accelRaw;
    gyroRaw_t gyroRaw;
    magRaw_t magRaw;
    int16_t temperatureRaw;

    uint8_t chip_id;

    // Communication mode
    CommMode _commMode;
    spiObject_t *_spi;
    i2cObject_t *_i2c;

    int16_t tempCount;   // Temperature raw count output
    uint32_t dt = 0;     // Delta time used to control display output rate

    float aRes, gRes, mRes;

    // I2C read/write function
    int reg_write_i2c(i2c_inst_t *i2c, 
                    const uint addr, 
                    const uint8_t reg, 
                    uint8_t *buf,
                    const uint8_t nbytes);

    int reg_read_i2c(i2c_inst_t *i2c,
                    const uint addr,
                    const uint8_t reg,
                    uint8_t *buf,
                    const uint8_t nbytes);

    // SPI read/write function
    int reg_write_spi(spi_inst_t *spi, 
                const uint cs, 
                const uint8_t reg, 
                const uint8_t *data,
                const uint8_t nbytes);

    int reg_read_spi(spi_inst_t *spi,
                    const uint cs,
                    const uint8_t reg,
                    uint8_t *buf,
                    uint8_t nbytes);

    void reg_write(uint8_t reg, uint8_t data);
    uint8_t reg_read(uint8_t reg);
    void reg_writes(uint8_t reg, uint8_t *data, uint16_t len);
    void reg_reads(uint8_t reg, uint8_t *data, uint16_t len);

    // Validate during startup phase, reading wrong chip ID can cause problem
    bool initializeHardware(void);
    // Get resolution factor for all accel, gyro, magneto
    void updateMagResolution();
    void updateAccelResolution();
    void updateGyroResolution();

public:
    // Constructor for SPI mode
    icm20948(spiObject_t &spi);
    // Constructor for I2C mode
    icm20948(i2cObject_t &i2c);

    // Factory mag calibration and mag bias
    float factoryMagCalibration[3] = {0, 0, 0}, factoryMagBias[3] = {0, 0, 0};
    // Bias corrections for gyro, accelerometer, and magnetometer
    float gyroBias[3]  = {0, 0, 0},
          accelBias[3] = {0, 0, 0},
          magBias[3]   = {0, 0, 0},
          magScale[3]  = {0, 0, 0};

    bool begin(void);

    uint8_t whoAmI(void) const {return chip_id;}
    
    accel_t getAccelData() const {return accel;}
    gyro_t getGyroData() const {return gyro;}
    mag_t getMagData() const {return mag;}
    float getTemperature() const {return temperature;}

    accelRaw_t getAccelDataRaw() const {return accelRaw;}
    gyroRaw_t getGyroDataRaw() const {return gyroRaw;}
    magRaw_t getMagDataRaw() const {return magRaw;}

    void updateImuData();
    void calibrateICM20948(float * gyroBias, float * accelBias);
    bool dataReady(void);
};// class ICM20948

#endif // _ICM20948_H_