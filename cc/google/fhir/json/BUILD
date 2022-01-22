licenses(["notice"])

package(default_visibility = ["//cc/google/fhir:__pkg__"])

cc_library(
    name = "json_sax_handler",
    srcs = ["json_sax_handler.cc"],
    hdrs = ["json_sax_handler.h"],
    strip_include_prefix = "//cc/",
    deps = [
        "@com_google_absl//absl/cleanup",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/status:statusor",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/strings:str_format",
        "@com_google_absl//absl/types:optional",
        "@nlohmann_json//:json",
        # TODO: To be removed once we move away from Json::Value
        "@jsoncpp_git//:jsoncpp",
    ],
)

cc_test(
    name = "json_sax_handler_test",
    srcs = ["json_sax_handler_test.cc"],
    deps = [
        ":json_sax_handler",
        "@com_google_absl//absl/strings",
        "@com_google_googletest//:gtest_main",
        "@jsoncpp_git//:jsoncpp",
    ],
)
