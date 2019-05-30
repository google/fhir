package(default_visibility = ["//visibility:public"])

cc_library(
    name = "antlr4_runtime",
    srcs = glob(["**/*.cpp"]),
    hdrs = glob(["**/*.h"]),
    copts = [
        "-fexceptions",
        "-Wno-implicit-fallthrough",
    ],
    features = ["-use_header_modules"],  # Incompatible with -fexceptions.
    includes = [
        ".",
        "./atn",
        "./dfa",
        "./misc",
        "./support",
        "./tree",
        "./tree/pattern",
        "./tree/xpath",
    ],
)
