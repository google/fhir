package(default_visibility = ["//:__subpackages__"])

licenses(["notice"])

filegroup(
    name = "jsonformat",
    srcs = glob(
        [
            "*.json",
            "*.prototxt",
        ],
        exclude = [
            "observation_invalid_unicode.json",
        ],
    ),
)
