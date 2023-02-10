licenses(["notice"])

package(
    default_applicable_licenses = ["//:license"],
    
    default_visibility = ["//visibility:public"],
)

cc_library(
    name = "status",
    hdrs = ["status.h"],
    strip_include_prefix = "//cc/",
    deps = [
        "@com_google_absl//absl/status",
    ],
)

cc_library(
    name = "statusor",
    hdrs = [
        "statusor.h",
    ],
    strip_include_prefix = "//cc/",
    deps = [
        ":status",
        "@com_google_absl//absl/status:statusor",
    ],
)
