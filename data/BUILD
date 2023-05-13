load("//tensorflow/lite/micro:build_def.bzl", "generate_cc_arrays")

package(
    default_visibility = ["//visibility:public"],
)

exports_files(
    srcs = [
        "training/SmartFall_Training.csv",
        "training/SmartFall_Testing.csv",
        "training/raw182_Training_Relabeled_Auto_25.csv",
        "training/raw91_Testing_Relabeled_Auto_25.csv",
        "training/Raw_Data_90ADL.csv",
    ],
    visibility = ["//tensorflow/lite/micro/examples/imu_fall_detection:__subpackages__"],
)