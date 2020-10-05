workspace(name = "com_google_fhir")

load("//bazel:dependencies.bzl", "fhirproto_dependencies")
fhirproto_dependencies(core_lib = True)

load("@com_google_protobuf//:protobuf_deps.bzl", "protobuf_deps")
protobuf_deps()

load("//bazel:workspace.bzl", "fhirproto_workspace")
fhirproto_workspace(core_lib = True)

load("@rules_python//python:pip.bzl", "pip3_import", "pip_repositories")
pip3_import(
    name = "fhir_bazel_pip_dependencies",
    requirements = "//bazel:requirements.txt",
)
pip_repositories()

load("@fhir_bazel_pip_dependencies//:requirements.bzl", "pip_install")
pip_install()

load("//bazel:go_dependencies.bzl", "fhir_go_dependencies")
fhir_go_dependencies()

load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")
http_archive(
    name = "google_bazel_common",
    sha256 = "18f266d921db1daa2ee9837343938e37fa21e0a8b6a0e43a67eda4c30f62b812",
    strip_prefix = "bazel-common-eb5c7e5d6d2c724fe410792c8be9f59130437e4a",
    urls = ["https://github.com/google/bazel-common/archive/eb5c7e5d6d2c724fe410792c8be9f59130437e4a.zip"],
)

# Needed for the jarjar_library rule.
load("@google_bazel_common//:workspace_defs.bzl", "google_common_workspace_rules")
google_common_workspace_rules()
