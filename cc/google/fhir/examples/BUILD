package(
    default_visibility = ["//visibility:public"],
)

licenses(["notice"])

cc_binary(
    name = "LocalProfiler",
    srcs = ["local_profiler.cc"],
    deps = [
        "//cc/google/fhir:json_format",
        "//cc/google/fhir/r4:profiles",
        "//examples/profiles:demo_cc_proto",
        "//proto/r4/core/resources:patient_cc_proto",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/time",
    ],
)
