load("@tflm_pip_deps//:requirements.bzl", "requirement")

package(
    default_visibility = ["//visibility:public"],
)

py_binary(
    name = "fall_detection_algorithms",
    srcs = ["fall_detection_algorithms.py"],
    python_version = "PY3",
    srcs_version = "PY3",
    deps = [
        requirement("numpy"),
        requirement("tensorflow-cpu"),
        "//tensorflow/lite/micro/python/interpreter/src:tflm_runtime",
    ],
)

py_binary(
    name = "dsp",
    srcs = ["dsp.py"],
    python_version = "PY3",
    srcs_version = "PY3",
    deps = [
        requirement("numpy"),
    ],
)

py_binary(
    name = "Serial",
    srcs = ["Serial.py"],
    python_version = "PY3",
    srcs_version = "PY3",
)

py_binary(
    name = "visualize_data",
    srcs = ["visualize_data.py"],
    python_version = "PY3",
    srcs_version = "PY3",
    deps = [
        requirement("numpy"),
    ],
)