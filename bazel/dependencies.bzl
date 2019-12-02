""" Function for loading dependencies of the FhirProto library """

load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

def fhirproto_dependencies():
    """ Loads dependencies of FhirProto """

    http_archive(
        name = "rules_jvm_external",
        strip_prefix = "rules_jvm_external-2.1",
        sha256 = "515ee5265387b88e4547b34a57393d2bcb1101314bcc5360ec7a482792556f42",
        url = "https://github.com/bazelbuild/rules_jvm_external/archive/2.1.zip",
    )

    http_archive(
        name = "io_bazel_rules_closure",
        sha256 = "7d206c2383811f378a5ef03f4aacbcf5f47fd8650f6abbc3fa89f3a27dd8b176",
        strip_prefix = "rules_closure-0.10.0",
        urls = [
            "https://github.com/bazelbuild/rules_closure/archive/0.10.0.tar.gz",
        ],
    )

    http_archive(
        name = "rules_proto",
        sha256 = "602e7161d9195e50246177e7c55b2f39950a9cf7366f74ed5f22fd45750cd208",
        strip_prefix = "rules_proto-97d8af4dc474595af3900dd85cb3a29ad28cc313",
        urls = [
            "https://mirror.bazel.build/github.com/bazelbuild/rules_proto/archive/97d8af4dc474595af3900dd85cb3a29ad28cc313.tar.gz",
            "https://github.com/bazelbuild/rules_proto/archive/97d8af4dc474595af3900dd85cb3a29ad28cc313.tar.gz",
        ],
    )

    http_archive(
        name = "rules_python",
        sha256 = "fa53cc0afe276d8f6675df1a424592e00e4f37b2a497e48399123233902e2e76",
        strip_prefix = "rules_python-0.0.1",
        urls = ["https://github.com/bazelbuild/rules_python/archive/0.0.1.tar.gz"],
    )

    http_archive(
        name = "rules_cc",
        urls = ["https://github.com/bazelbuild/rules_cc/archive/bf6a32cff59d22305c37361ca6fea752df8fdd59.zip"],
        strip_prefix = "rules_cc-bf6a32cff59d22305c37361ca6fea752df8fdd59",
        sha256 = "3bb877a515252877080d68d919f39c54e18c74b421ec10831a1d17059cae86bf",
    )

    http_archive(
        name = "org_tensorflow",
        sha256 = "674cc90223f1d6b7fa2969e82636a630ce453e48a9dec39d73d6dba2fd3fd243",
        strip_prefix = "tensorflow-2.1.0-rc0",
        urls = [
            "https://github.com/tensorflow/tensorflow/archive/v2.1.0-rc0.tar.gz",
        ],
    )

    # Used for the FHIRPath parser runtime.
    http_archive(
        name = "antlr_cc_runtime",
        url = "https://www.antlr.org/download/antlr4-cpp-runtime-4.7.1-source.zip",
        sha256 = "23bebc0411052a260f43ae097aa1ab39869eb6b6aa558b046c367a4ea33d1ccc",
        strip_prefix = "runtime/src",
        build_file = "@com_google_fhir//bazel:antlr.BUILD",
    )

    http_archive(
        name = "bazel_skylib",
        sha256 = "1dde365491125a3db70731e25658dfdd3bc5dbdfd11b840b3e987ecf043c7ca0",
        urls = ["https://github.com/bazelbuild/bazel-skylib/releases/download/0.9.0/bazel_skylib-0.9.0.tar.gz"],
    )
