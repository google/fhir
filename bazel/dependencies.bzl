""" Function for loading dependencies of the FhirProto library """

load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

def fhirproto_dependencies(core_lib = False):
    """ Sets up FhirProto dependencies

    Args:
      core_lib: Whether or not this is the core FHIR lib github.com/google/fhir
    """

    main_fhir_location = "" if core_lib else "@com_google_fhir"

    http_archive(
        name = "com_google_absl",
        sha256 = "987ce98f02eefbaf930d6e38ab16aa05737234d7afbab2d5c4ea7adbe50c28ed",
        strip_prefix = "abseil-cpp-20230802.1",
        urls = ["https://github.com/abseil/abseil-cpp/archive/refs/tags/20230802.1.tar.gz"],
    )

    http_archive(
        name = "rules_jvm_external",
        strip_prefix = "rules_jvm_external-6.0",
        sha256 = "85fd6bad58ac76cc3a27c8e051e4255ff9ccd8c92ba879670d195622e7c0a9b7",
        url = "https://github.com/bazelbuild/rules_jvm_external/releases/download/6.0/rules_jvm_external-6.0.tar.gz",
    )

    http_archive(
        name = "io_bazel_rules_closure",
        sha256 = "7d206c2383811f378a5ef03f4aacbcf5f47fd8650f6abbc3fa89f3a27dd8b176",
        strip_prefix = "rules_closure-0.12.0",
        urls = [
            "https://github.com/bazelbuild/rules_closure/archive/refs/tags/0.12.0.tar.gz",
        ],
    )

    http_archive(
        name = "rules_proto",
        sha256 = "dc3fb206a2cb3441b485eb1e423165b231235a1ea9b031b4433cf7bc1fa460dd",
        strip_prefix = "rules_proto-5.3.0-21.7",
        urls = [
            "https://github.com/bazelbuild/rules_proto/archive/refs/tags/5.3.0-21.7.tar.gz",
        ],
    )

    http_archive(
        name = "rules_python",
        url = "https://github.com/bazelbuild/rules_python/releases/download/0.1.0/rules_python-0.1.0.tar.gz",
        sha256 = "b6d46438523a3ec0f3cead544190ee13223a52f6a6765a29eae7b7cc24cc83a0",
    )

    http_archive(
        name = "rules_cc",
        urls = ["https://github.com/bazelbuild/rules_cc/releases/download/0.0.9/rules_cc-0.0.9.tar.gz"],
        strip_prefix = "rules_cc-0.0.9",
        sha256 = "2037875b9a4456dce4a79d112a8ae885bbc4aad968e6587dca6e64f3a0900cdf",
    )

    # Used for the FHIRPath parser runtime.
    http_archive(
        name = "antlr_cc_runtime",
        url = "https://www.antlr.org/download/antlr4-cpp-runtime-4.7.1-source.zip",
        sha256 = "23bebc0411052a260f43ae097aa1ab39869eb6b6aa558b046c367a4ea33d1ccc",
        strip_prefix = "runtime/src",
        build_file = main_fhir_location + "//bazel:antlr.BUILD",
    )

    http_archive(
        name = "bazel_skylib",
        sha256 = "d00f1389ee20b60018e92644e0948e16e350a7707219e7a390fb0a99b6ec9262",
        urls = [
            "https://mirror.bazel.build/github.com/bazelbuild/bazel-skylib/releases/download/1.7.0/bazel-skylib-1.7.0.tar.gz",
            "https://github.com/bazelbuild/bazel-skylib/releases/download/1.7.0/bazel-skylib-1.7.0.tar.gz",
        ],
    )

    http_archive(
        name = "io_bazel_rules_go",
        sha256 = "6734a719993b1ba4ebe9806e853864395a8d3968ad27f9dd759c196b3eb3abe8",
        urls = [
            "https://mirror.bazel.build/github.com/bazelbuild/rules_go/releases/download/v0.45.1/rules_go-v0.45.1.zip",
            "https://github.com/bazelbuild/rules_go/releases/download/v0.45.1/rules_go-v0.45.1.zip",
        ],
    )

    # Used for go_repository bazel rules for go dependencies.
    http_archive(
        name = "bazel_gazelle",
        sha256 = "32938bda16e6700063035479063d9d24c60eda8d79fd4739563f50d331cb3209",
        urls = [
            "https://mirror.bazel.build/github.com/bazelbuild/bazel-gazelle/releases/download/v0.35.0/bazel-gazelle-v0.35.0.tar.gz",
            "https://github.com/bazelbuild/bazel-gazelle/releases/download/v0.35.0/bazel-gazelle-v0.35.0.tar.gz",
        ],
    )

    http_archive(
        name = "com_google_protobuf",
        sha256 = "8ff511a64fc46ee792d3fe49a5a1bcad6f7dc50dfbba5a28b0e5b979c17f9871",
        url = "https://github.com/protocolbuffers/protobuf/archive/refs/tags/v25.2.tar.gz",
        strip_prefix = "protobuf-25.2",
    )

    http_archive(
        name = "nlohmann_json",
        url = "https://github.com/nlohmann/json/archive/v3.7.3.zip",
        strip_prefix = "json-3.7.3",
        sha256 = "e109cd4a9d1d463a62f0a81d7c6719ecd780a52fb80a22b901ed5b6fe43fb45b",
        build_file_content = """cc_library(name = "json",
                                           visibility = ["//visibility:public"],
                                           hdrs = glob([
                                               "include/nlohmann/*.hpp",
                                               "include/nlohmann/**/*.hpp",
                                               ]),
                                           includes = ["include"],
                                          )""",
    )

    http_archive(
        name = "libarchive",
        build_file = main_fhir_location + "//bazel/buildfiles:libarchive.BUILD",
        sha256 = "c676146577d989189940f1959d9e3980d28513d74eedfbc6b7f15ea45fe54ee2",
        strip_prefix = "libarchive-3.6.1",
        urls = [
            "https://github.com/libarchive/libarchive/releases/download/v3.6.1/libarchive-3.6.1.tar.gz",
        ],
    )

    http_archive(
        name = "icu",
        sha256 = "dfc62618aa4bd3ca14a3df548cd65fe393155edd213e49c39f3a30ccd618fc27",
        strip_prefix = "icu-release-64-2",
        build_file = main_fhir_location + "//bazel/buildfiles:icu.BUILD",
        urls = [
            "https://storage.googleapis.com/mirror.tensorflow.org/github.com/unicode-org/icu/archive/release-64-2.zip",
            "https://github.com/unicode-org/icu/archive/release-64-2.zip",
        ],
    )

    http_archive(
        name = "com_googlesource_code_re2",
        sha256 = "6f4c8514249cd65b9e85d3e6f4c35595809a63ad71c5d93083e4d1dcdf9e0cd6",
        strip_prefix = "re2-2020-08-01",
        urls = [
            "https://github.com/google/re2/archive/2020-08-01.tar.gz",
        ],
    )

    http_archive(
        name = "com_google_googletest",
        sha256 = "ad7fdba11ea011c1d925b3289cf4af2c66a352e18d4c7264392fead75e919363",
        strip_prefix = "googletest-1.13.0",
        urls = [
            "https://github.com/google/googletest/archive/refs/tags/v1.13.0.tar.gz",
        ],
    )

    http_archive(
        name = "com_github_gflags_gflags",
        sha256 = "34af2f15cf7367513b352bdcd2493ab14ce43692d2dcd9dfc499492966c64dcf",
        strip_prefix = "gflags-2.2.2",
        urls = [
            "https://mirror.bazel.build/github.com/gflags/gflags/archive/v2.2.2.tar.gz",
            "https://github.com/gflags/gflags/archive/v2.2.2.tar.gz",
        ],
    )

    http_archive(
        name = "com_github_glog_glog",
        strip_prefix = "glog-0a2e5931bd5ff22fd3bf8999eb8ce776f159cda6",
        sha256 = "58c9b3b6aaa4dd8b836c0fd8f65d0f941441fb95e27212c5eeb9979cfd3592ab",
        urls = [
            "https://github.com/google/glog/archive/0a2e5931bd5ff22fd3bf8999eb8ce776f159cda6.zip",
        ],
    )

    http_archive(
        name = "six_archive",
        urls = [
            "https://pypi.python.org/packages/source/s/six/six-1.16.0.tar.gz",
            "http://mirror.bazel.build/pypi.python.org/packages/source/s/six/six-1.16.0.tar.gz",
        ],
        sha256 = "1e61c37477a1626458e36f7b1d82aa5c9b094fa4802892072e49de9c60c4c926",
        strip_prefix = "six-1.16.0",
        build_file = main_fhir_location + "//bazel/buildfiles:six.BUILD",
    )
