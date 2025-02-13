workspace(name = "com_google_fhir")

load("//bazel:dependencies.bzl", "fhirproto_dependencies")

fhirproto_dependencies(core_lib = True)

load("@com_google_protobuf//:protobuf_deps.bzl", "protobuf_deps")

protobuf_deps()

load("//bazel:workspace.bzl", "fhirproto_workspace")

fhirproto_workspace(core_lib = True)

load("//bazel:go_dependencies.bzl", "fhir_go_dependencies")

# gazelle:repository_macro bazel/go_dependencies.bzl%fhir_go_dependencies
fhir_go_dependencies()

load("@io_bazel_rules_go//go:deps.bzl", "go_register_toolchains", "go_rules_dependencies")

go_rules_dependencies()

go_register_toolchains(version = "1.19.5")

load("@bazel_gazelle//:deps.bzl", "gazelle_dependencies")

gazelle_dependencies()

load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive", "http_file")

http_archive(
    name = "google_bazel_common",
    sha256 = "e30e092e50c47a38994334dbe42386675cf519a5e86b973e45034323bbdb70a3",
    strip_prefix = "bazel-common-a9e1d8efd54cbf27249695b23775b75ca65bb59d",
    urls = ["https://github.com/google/bazel-common/archive/a9e1d8efd54cbf27249695b23775b75ca65bb59d.zip"],
)

# Needed for the jarjar_library rule.
load("@google_bazel_common//:workspace_defs.bzl", "google_common_workspace_rules")

google_common_workspace_rules()

# Core FHIR spec dependencies.
http_file(
    name = "hl7.fhir.r4.core_4.0.1",
    downloaded_file_path = "hl7.fhir.r4.core@4.0.1.tgz",
    sha256 = "b090bf929e1f665cf2c91583720849695bc38d2892a7c5037c56cb00817fb091",
    url = "https://hl7.org/fhir/R4/hl7.fhir.r4.core.tgz",
)

http_file(
    name = "hl7.fhir.r5.core_5.0.0",
    downloaded_file_path = "hl7.fhir.r5.core@5.0.0.tgz",
    sha256 = "74b27cd1bfce9e80eaceac431edf230b0945a443564fbf5512f82e5fa50a80d4",
    url = "https://hl7.org/fhir/R5/hl7.fhir.r5.core.tgz",
)
