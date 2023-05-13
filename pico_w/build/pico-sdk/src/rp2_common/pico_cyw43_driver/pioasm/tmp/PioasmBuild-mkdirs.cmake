# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "/home/nhan/gitwork/pico/pico-sdk/tools/pioasm"
  "/home/nhan/gitwork/tflite-micro/tensorflow/lite/micro/examples/imu_fall_detection/pico_w/build/pioasm"
  "/home/nhan/gitwork/tflite-micro/tensorflow/lite/micro/examples/imu_fall_detection/pico_w/build/pico-sdk/src/rp2_common/pico_cyw43_driver/pioasm"
  "/home/nhan/gitwork/tflite-micro/tensorflow/lite/micro/examples/imu_fall_detection/pico_w/build/pico-sdk/src/rp2_common/pico_cyw43_driver/pioasm/tmp"
  "/home/nhan/gitwork/tflite-micro/tensorflow/lite/micro/examples/imu_fall_detection/pico_w/build/pico-sdk/src/rp2_common/pico_cyw43_driver/pioasm/src/PioasmBuild-stamp"
  "/home/nhan/gitwork/tflite-micro/tensorflow/lite/micro/examples/imu_fall_detection/pico_w/build/pico-sdk/src/rp2_common/pico_cyw43_driver/pioasm/src"
  "/home/nhan/gitwork/tflite-micro/tensorflow/lite/micro/examples/imu_fall_detection/pico_w/build/pico-sdk/src/rp2_common/pico_cyw43_driver/pioasm/src/PioasmBuild-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/home/nhan/gitwork/tflite-micro/tensorflow/lite/micro/examples/imu_fall_detection/pico_w/build/pico-sdk/src/rp2_common/pico_cyw43_driver/pioasm/src/PioasmBuild-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/home/nhan/gitwork/tflite-micro/tensorflow/lite/micro/examples/imu_fall_detection/pico_w/build/pico-sdk/src/rp2_common/pico_cyw43_driver/pioasm/src/PioasmBuild-stamp${cfgdir}") # cfgdir has leading slash
endif()
