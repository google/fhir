package(
    default_visibility = ["//visibility:public"],
)

licenses(["notice"])

cc_binary(
    name = "LocalProfiler",
    srcs = ["local_profiler.cc"],
    deps = [
        "//cc/google/fhir/stu3:json_format",
        "//cc/google/fhir/stu3:profiles",
        "//examples/profiles:demo_cc_proto",
        "//proto/stu3:datatypes_cc_proto",
        "//proto/stu3:resources_cc_proto",
        "//proto/stu3:uscore_cc_proto",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/time",
    ],
)
