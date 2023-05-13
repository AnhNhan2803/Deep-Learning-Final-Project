# Description:
#   TensorFlow Lite for Microcontrollers "hello world" example.
load("@tflm_pip_deps//:requirements.bzl", "requirement")
load(
    "//tensorflow/lite/micro:build_def.bzl",
    "micro_copts",
)

package(
    features = ["-layering_check"],
    licenses = ["notice"],
)

py_binary(
    name = "train",
    srcs = ["train.py"],
    data = [
        "//tensorflow/lite/micro/examples/imu_fall_detection/model:fall_detection_model.tflite",
        "//tensorflow/lite/micro/examples/imu_fall_detection/data:training/SmartFall_Training.csv",
        "//tensorflow/lite/micro/examples/imu_fall_detection/data:training/SmartFall_Testing.csv",
        "//tensorflow/lite/micro/examples/imu_fall_detection/data:training/raw182_Training_Relabeled_Auto_25.csv",
        "//tensorflow/lite/micro/examples/imu_fall_detection/data:training/raw91_Testing_Relabeled_Auto_25.csv",
        "//tensorflow/lite/micro/examples/imu_fall_detection/data:training/Raw_Data_90ADL.csv",
        ],
    python_version = "PY3",
    srcs_version = "PY3",
    deps = [
        "@absl_py//absl:app",
        "@absl_py//absl/flags",
        "@absl_py//absl/logging",
        requirement("numpy"),
        requirement("tensorflow-cpu"),
        "//tensorflow/lite/micro/python/interpreter/src:tflm_runtime",
        "//tensorflow/lite/micro/examples/imu_fall_detection/utils:fall_detection_algorithms",
    ],
)

py_binary(
    name = "tuning",
    srcs = ["tuning.py"],
    data = [
        "//tensorflow/lite/micro/examples/imu_fall_detection/model:fall_detection_model.tflite",
        "//tensorflow/lite/micro/examples/imu_fall_detection/data:training/SmartFall_Training.csv",
        "//tensorflow/lite/micro/examples/imu_fall_detection/data:training/raw182_Training_Relabeled_Auto_25.csv",
        "//tensorflow/lite/micro/examples/imu_fall_detection/data:training/Raw_Data_90ADL.csv",
        ],
    python_version = "PY3",
    srcs_version = "PY3",
    deps = [
        "@absl_py//absl:app",
        "@absl_py//absl/flags",
        "@absl_py//absl/logging",
        requirement("numpy"),
        requirement("tensorflow-cpu"),
        "//tensorflow/lite/micro/python/interpreter/src:tflm_runtime",
        "//tensorflow/lite/micro/examples/imu_fall_detection/utils:fall_detection_algorithms",
    ],
)

py_binary(
    name = "main",
    srcs = ["main.py"],
    python_version = "PY3",
    srcs_version = "PY3",
    deps = [
        "@absl_py//absl:app",
        "@absl_py//absl/flags",
        "@absl_py//absl/logging",
        requirement("numpy"),
        requirement("tensorflow-cpu"),
        "//tensorflow/lite/micro/python/interpreter/src:tflm_runtime",
        "//tensorflow/lite/micro/examples/imu_fall_detection/utils:fall_detection_algorithms",
        "//tensorflow/lite/micro/examples/imu_fall_detection/utils:dsp",
        "//tensorflow/lite/micro/examples/imu_fall_detection/utils:Serial",
        "//tensorflow/lite/micro/examples/imu_fall_detection/utils:visualize_data",
    ],
)

py_binary(
    name = "analyzeIMUData",
    srcs = ["analyzeIMUData.py"],
    python_version = "PY3",
    srcs_version = "PY3",
    deps = [
        requirement("numpy"),
        requirement("tensorflow-cpu"),
        "//tensorflow/lite/micro/examples/imu_fall_detection/utils:dsp",
        "//tensorflow/lite/micro/examples/imu_fall_detection/utils:Serial",
        "//tensorflow/lite/micro/examples/imu_fall_detection/utils:visualize_data",
    ],
)
