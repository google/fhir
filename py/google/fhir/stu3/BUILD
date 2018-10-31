licenses(["notice"])

package(default_visibility = ["//visibility:public"])

py_library(
    name = "util",
    srcs = [
        "util.py",
    ],
    deps = [
        "//proto/stu3:resources_py_pb2",
    ],
)

py_test(
    name = "util_test",
    srcs = [
        "util_test.py",
    ],
    deps = [
        ":util",
        "//proto/stu3:resources_py_pb2",
        "@absl_py//absl/testing:absltest",
        "@protobuf_archive//:protobuf_python",
    ],
)
