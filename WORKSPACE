workspace(name = "com_google_fhir")

load("//bazel:dependencies.bzl", "fhirproto_dependencies")

fhirproto_dependencies(core_lib = True)

load("@com_google_protobuf//:protobuf_deps.bzl", "protobuf_deps")

protobuf_deps()

load("//bazel:workspace.bzl", "fhirproto_workspace")

fhirproto_workspace(core_lib = True)

load("@rules_python//python:pip.bzl", "pip_install")

pip_install(
    name = "fhir_bazel_pip_dependencies",
    requirements = "//bazel:requirements.txt",
)

load("//bazel:go_dependencies.bzl", "fhir_go_dependencies")

# gazelle:repository_macro bazel/go_dependencies.bzl%fhir_go_dependencies
fhir_go_dependencies()

load("@io_bazel_rules_go//go:deps.bzl", "go_register_toolchains", "go_rules_dependencies")

go_rules_dependencies()

go_register_toolchains(version = "1.19.5")

load("@bazel_gazelle//:deps.bzl", "gazelle_dependencies")

gazelle_dependencies()

load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

http_archive(
    name = "google_bazel_common",
    sha256 = "e30e092e50c47a38994334dbe42386675cf519a5e86b973e45034323bbdb70a3",
    strip_prefix = "bazel-common-a9e1d8efd54cbf27249695b23775b75ca65bb59d",
    urls = ["https://github.com/google/bazel-common/archive/a9e1d8efd54cbf27249695b23775b75ca65bb59d.zip"],
)

# Needed for the jarjar_library rule.
load("@google_bazel_common//:workspace_defs.bzl", "google_common_workspace_rules")

google_common_workspace_rules()
