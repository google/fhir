workspace(name = "com_google_fhir")

load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

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
    name = "rules_python",
    sha256 = "fa53cc0afe276d8f6675df1a424592e00e4f37b2a497e48399123233902e2e76",
    strip_prefix = "rules_python-0.0.1",
    urls = ["https://github.com/bazelbuild/rules_python/archive/0.0.1.tar.gz"],
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
    name = "org_tensorflow",
    sha256 = "49b5f0495cd681cbcb5296a4476853d4aea19a43bdd9f179c928a977308a0617",
    strip_prefix = "tensorflow-2.0.0",
    urls = [
        "https://github.com/tensorflow/tensorflow/archive/v2.0.0.tar.gz",
    ],
)

load("@rules_python//python:pip.bzl", "pip_import")

pip_import(
    name = "fhir_bazel_pip_dependencies",
    requirements = "//bazel:requirements.txt",
)


load("//bazel:dependencies.bzl", "fhir_proto_dependencies")
fhir_proto_dependencies()

