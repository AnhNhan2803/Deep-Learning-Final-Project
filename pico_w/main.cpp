/*******************************************************************************
* Title                 :   Main - Deep Learning
* Filename              :   main.cpp
* Author                :   Nhan
* Origin Date           :   04/29/2023
* Notes                 :   None
********************** ********************************************************/

/*******************************************************************************
* INCLUDE
*******************************************************************************/
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "btstack_run_loop.h"
#include "hardware/clocks.h"

#include "imu/icm20948.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "arm_math.h"

#include "tensorflow/lite/micro/micro_error_reporter.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/micro/micro_mutable_op_resolver.h"
#include "tensorflow/lite/schema/schema_generated.h"
#include "tensorflow/lite/version.h"

#include "model/tflite.h"
#include "model/model.h"

/******************************************************************************
* PREPROCESSOR CONSTANTS
*******************************************************************************/
#define SENSOR_TASK_PRIORITY				(tskIDLE_PRIORITY + 3UL)
#define MODEL_TASK_PRIORITY 				(tskIDLE_PRIORITY + 2UL)
#define TFLM_WINDOW_LEN                     (40)
#define SENSOR_QUEUE_LEN                    (100)
#define PLL_SYS_KHZ                         (133 * 1000)

#define MAX_ELEMENT_PER_PACKET  5
// Body size + 2 header size + 2 footer size
#define PACKET_CHUNK_SIZE     (23 + 4)
#define PACKET_TOTAL_SIZE     (PACKET_CHUNK_SIZE * MAX_ELEMENT_PER_PACKET)
#define RAW_SIZE_EACH_SENSOR  6

/******************************************************************************
* PREPROCESSOR MACROS
*******************************************************************************/
// #define ENABLE_BLUETOOTH_APP
#define ENABLE_WIFI_STACK

#ifdef ENABLE_BLUETOOTH_APP
#include "btstack/btapp.h"
#endif

#ifdef ENABLE_WIFI_STACK
#include "wifi/firebase.h"
#define WIFI_TASK_PRIORITY 				(tskIDLE_PRIORITY + 1UL)
#endif

/******************************************************************************
* TYPEDEFS
*******************************************************************************/


/******************************************************************************
* VARIABLE DEFINITIONS
*******************************************************************************/
static i2cObject_t I2C = {
    .sdaPin = 16,
    .sclPin = 17,
    .i2cAddress = 0x69,
    .clkHz = 400,
    .port = i2c0,
};

icm20948 imu = icm20948(I2C);
static QueueHandle_t sensorsQueue;
MLModel model(ml_model, 50*1024);
static accel_t inputBuffer[TFLM_WINDOW_LEN];

#ifdef ENABLE_BLUETOOTH_APP
static uint8_t sendbuff[PACKET_TOTAL_SIZE];
#endif

#ifdef ENABLE_WIFI_STACK
static SemaphoreHandle_t firebaseSem;
#endif

/******************************************************************************
* FUNCTION PROTOTYPES
*******************************************************************************/
static void sensorEntry(__unused void *params);
static void modelEntry(__unused void *params);

#ifdef ENABLE_BLUETOOTH_APP
static void structureRawPacket(accelRaw_t *accel, 
                                gyroRaw_t *gyro, 
                                magRaw_t *mag,
                                uint8_t *outData,
                                uint32_t sequenceNumber, 
                                uint32_t tick);
#endif

#ifdef ENABLE_WIFI_STACK
static void wifiEntry(__unused void *params);
#endif

/******************************************************************************
* MAIN ENTRANCE FUNCTION
*******************************************************************************/
int main() 
{
    // Set the system frequency to 133 MHz. vco_calc.py from the SDK tells us
    // this is exactly attainable at the PLL from a 12 MHz crystal: FBDIV =
    // 133 (so VCO of 1596 MHz), PD1 = 6, PD2 = 2. This function will set the
    // system PLL to 133 MHz and set the clk_sys divisor to 1.
    set_sys_clock_khz(PLL_SYS_KHZ, true);

    // The previous line automatically detached clk_peri from clk_sys, and
    // attached it to pll_usb, so that clk_peri won't be disturbed by future
    // changes to system clock or system PLL. If we need higher clk_peri
    // frequencies, we can attach clk_peri directly back to system PLL (no
    // divider available) and then use the clk_sys divider to scale clk_sys
    // independently of clk_peri.
    clock_configure(
        clk_peri,
        0,                                                // No glitchless mux
        CLOCKS_CLK_PERI_CTRL_AUXSRC_VALUE_CLKSRC_PLL_SYS, // System PLL on AUX mux
        PLL_SYS_KHZ * 1000,                               // Input frequency
        PLL_SYS_KHZ * 1000                                // Output (must be same as no divider)
    );

    // Initialize chosen serial port
    stdio_init_all();

#if defined(ENABLE_BLUETOOTH_APP) || defined(ENABLE_WIFI_STACK)
    // initialize CYW43 driver
    if (cyw43_arch_init()) 
    {
        printf("cyw43_arch_init() failed.\n");
        return -1;
    }
#endif

    BaseType_t err;

    // Delay for all HW components initialization
    sleep_ms(1000);

    printf("Fall Detection System!\n");

    // Init IMU sensor
    imu.begin();

    // Init the ML model here
    if (!model.init()) 
    {
        printf("Infinity loop due to the failure of ML model initialization\n");
        while(1){}
    }

#ifdef ENABLE_BLUETOOTH_APP
    // Initialize the BT stack here
    btstack_main();
    // btstack_run_loop_execute();
#elif defined(ENABLE_WIFI_STACK)
    cyw43_arch_enable_sta_mode();

    printf("Connecting to Wi-Fi...\n");
    if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, 30000)) 
    {
        printf("failed to connect.\n");
        return 1;
    } 
    else 
    {
        printf("Connected.\n");
    }

    firebaseSem = xSemaphoreCreateBinary();

    // Create 1 tasks for wifi process
    TaskHandle_t wifiTask;
    err = xTaskCreate(wifiEntry, "WifiThread", 1024*8, NULL, WIFI_TASK_PRIORITY, &wifiTask);
    if(err != pdPASS)
    {
        printf("Failed to create WifiThread!\n");
    }

#endif

    // Create 2 tasks for model and sensor process
    TaskHandle_t sensorTask;
    err = xTaskCreate(sensorEntry, "SensorThread", 1024*2, NULL, SENSOR_TASK_PRIORITY, &sensorTask);
    if(err != pdPASS)
    {
        printf("Failed to create SensorThread!\n");
    }
    TaskHandle_t modelTask;
    err = xTaskCreate(modelEntry, "modelThread", 1024*4, NULL, MODEL_TASK_PRIORITY, &modelTask);
    if(err != pdPASS)
    {
        printf("Failed to create modelThread!\n");
    }
    // Create sensor queue
    sensorsQueue = xQueueCreate(SENSOR_QUEUE_LEN, sizeof(accel_t));

#if ( portSUPPORT_SMP == 1 ) && ( configNUM_CORES == 2 ) && (NO_SYS) && (configUSE_CORE_AFFINITY)
    // Launch model processing to core 1 to free the workload of core 0
    vTaskCoreAffinitySet(modelTask, 1 << 1);
#endif
    // /* Start the tasks and timer running. */
    vTaskStartScheduler();

    while(1)
    {
        printf("Should not reach here\n");
        sleep_ms(1000);
    }

    return 0;
}

/******************************************************************************
* STATIC FUNCTIONS
*******************************************************************************/
// Thread for sensor data collection
static void sensorEntry(__unused void *params) 
{
    accel_t sensorAccel;
    uint8_t streamIdx = 0;
    uint8_t sequenceCounter = 0;

    float32_t targetSampleRate = 31.25;
    uint32_t targetTimeInterval = uint8_t(1000 / targetSampleRate);
    uint32_t downSamplingTimeReference = 0;

    while (1) 
    {
        if(imu.dataReady())
        {
            imu.updateImuData();

            if (downSamplingTimeReference == 0)
            {
                // Add the start reference time here
                downSamplingTimeReference = to_ms_since_boot(get_absolute_time());
            }
            else if(to_ms_since_boot(get_absolute_time()) >= downSamplingTimeReference + targetTimeInterval)
            {
                downSamplingTimeReference += targetTimeInterval;

                // Officially push sensor data to queue here
                sensorAccel = imu.getAccelData();

                // Do not need to wait for the return error code
                // since it's ok if we miss 1 or 2 samples
                if(xQueueSend(sensorsQueue, (void *)&sensorAccel, 10) != pdTRUE)
                {
                    printf("Queue FULL!\n");
                }

                // Print results
                // printf("X: %.2f | Y: %.2f | Z: %.2f\r\n", accel["x"], accel["y"], accel["z"]);
#ifdef ENABLE_BLUETOOTH_APP
                if(isConnected())
                {
                    accelRaw_t accelRaw = imu.getAccelDataRaw();
                    gyroRaw_t gyroRaw = imu.getGyroDataRaw();
                    magRaw_t magRaw = imu.getMagDataRaw();

                    int idx = streamIdx * PACKET_CHUNK_SIZE;
                    structureRawPacket(&accelRaw, 
                                        &gyroRaw, 
                                        &magRaw, 
                                        &sendbuff[idx], 
                                        sequenceCounter++, 
                                        to_ms_since_boot(get_absolute_time()));

                    if(++streamIdx == MAX_ELEMENT_PER_PACKET)
                    {
                        streamIdx = 0;
                        uint8_t err = spp_send_packet(sendbuff, sizeof(sendbuff));
                        if(err != 0)
                        {
                            printf("Failed to send SPP packet - 0x%02x\n", err);
                        }
                    }
                }
#endif
            }
        }

        // Add minimum delay here so we can get more smooth accelerometers data
        vTaskDelay(5);
    }
}

// Thread for Tflite model execution
static void modelEntry(__unused void *params) 
{
    bool fall = false;
    uint8_t sampleIdx = 0;

    while (true) 
    {
        // Wait for available sensor data here via Queue
        if (xQueueReceive(sensorsQueue, (void *)&inputBuffer[sampleIdx], portMAX_DELAY) == pdTRUE)
        {
            // Wait for a full window of 40 samples before feeding the model
            if(++sampleIdx == TFLM_WINDOW_LEN)
            {
                // Do the lowpass filtering with cutoff frequency = 15Hz using CMSIS DSP

                // Fill the input buffer with data from inputBuffer
                for (int i = 0; i < TFLM_WINDOW_LEN; i++)
                {
                    model._input_tensor->data.f[i * 3 + 0] = inputBuffer[i].x;
                    model._input_tensor->data.f[i * 3 + 1] = inputBuffer[i].y;
                    model._input_tensor->data.f[i * 3 + 2] = inputBuffer[i].z;
                }

                uint32_t tRef = to_ms_since_boot(get_absolute_time());
                // Process the model here
                float32_t prediction = model.predict();
                printf("Predicted result: %f\n", prediction);
                printf("Model performance: %d\n",  to_ms_since_boot(get_absolute_time()) - tRef);

                fall = (prediction >= 0.7) ? true : false;

                // Send fall notification to server here
                if (fall)
                {
                    printf("=========> Fall detected\n");
                    // Notify the wifi thread so we can send th fall notification to firebase
                    xSemaphoreGive(firebaseSem);
                }

                // Might do the nottfication to remote here (via Wifi)

                // We have a straide of 5 samples per window
                sampleIdx = TFLM_WINDOW_LEN - 5;
                // Remove the first 4 samples here
                memmove(inputBuffer, &inputBuffer[5], sampleIdx * sizeof(accel_t));
            }
        }
    }
}

#ifdef ENABLE_BLUETOOTH_APP
// Structure the SPP streaming packet if required
static void structureRawPacket(accelRaw_t *accel, 
                                gyroRaw_t *gyro, 
                                magRaw_t *mag,
                                uint8_t *outData,
                                uint32_t sequenceNumber, 
                                uint32_t tick)
{
    int idx = 0;
    outData[idx++] = 0xAB;
    outData[idx++] = 0xBA;
    // Sequence number to make sure we do not miss any packet
    outData[idx++] = sequenceNumber;

    // Add timestamp here to sync data later in PC
    int tickSize = sizeof(tick);
    
    memcpy(&outData[idx], &tick, tickSize);
    idx += tickSize;

    // Place raw data from here, accel first
    memcpy(&outData[idx], accel, RAW_SIZE_EACH_SENSOR);
    idx += RAW_SIZE_EACH_SENSOR;
    memcpy(&outData[idx], gyro, RAW_SIZE_EACH_SENSOR);
    idx += RAW_SIZE_EACH_SENSOR;
    memcpy(&outData[idx], mag, RAW_SIZE_EACH_SENSOR);
    idx += RAW_SIZE_EACH_SENSOR;
    outData[idx++] = 0xAD;
    outData[idx] = 0xDA;
}
#endif

#ifdef ENABLE_WIFI_STACK
static void wifiEntry(__unused void *params)
{
    while(true)
    {
        // Wait for the semaphore here, since the job of sending data to firebase
        // takes a lot of time, that is why we need to define an individual thread
        // for this
        xSemaphoreTake(firebaseSem, portMAX_DELAY);
        sendDataToFireBase(1);
        sleep_ms(1000);
    }
}
#endif
