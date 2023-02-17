package(
    default_applicable_licenses = [""],
    default_visibility = ["//visibility:public"],
)

licenses(["notice"])

cc_binary(
    name = "LocalProfiler",
    srcs = ["local_profiler.cc"],
    deps = [
        "//cc/google/fhir/r4:json_format",
        "//cc/google/fhir/r4:profiles",
        "//cc/google/fhir/status",
        "//proto/google/fhir/proto/r4/core/resources:patient_cc_proto",
        "//examples/profiles:demo_cc_proto",
        "@com_github_glog_glog//:glog",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/time",
    ],
)
