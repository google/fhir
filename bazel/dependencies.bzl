""" Function for loading dependencies of the FhirProto library """

load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")
load("@rules_python//python:pip.bzl", "pip_repositories")
load("@fhir_bazel_pip_dependencies//:requirements.bzl", "pip_install")
load("@org_tensorflow//tensorflow:workspace.bzl", "tf_workspace")
load("@rules_jvm_external//:defs.bzl", "maven_install")
load("@rules_proto//proto:repositories.bzl", "rules_proto_dependencies", "rules_proto_toolchains")

def fhir_proto_dependencies():
    """ Loads dependencies of FhirProto """

    rules_proto_dependencies()
    rules_proto_toolchains()

    http_archive(
        name = "rules_cc",
        urls = ["https://github.com/bazelbuild/rules_cc/archive/bf6a32cff59d22305c37361ca6fea752df8fdd59.zip"],
        strip_prefix = "rules_cc-bf6a32cff59d22305c37361ca6fea752df8fdd59",
        sha256 = "3bb877a515252877080d68d919f39c54e18c74b421ec10831a1d17059cae86bf",
    )

    # Used for the FHIRPath parser runtime.
    http_archive(
        name = "antlr_cc_runtime",
        url = "https://www.antlr.org/download/antlr4-cpp-runtime-4.7.1-source.zip",
        sha256 = "23bebc0411052a260f43ae097aa1ab39869eb6b6aa558b046c367a4ea33d1ccc",
        strip_prefix = "runtime/src",
        build_file = "//bazel:antlr.BUILD",
    )

    http_archive(
        name = "bazel_skylib",
        sha256 = "bbccf674aa441c266df9894182d80de104cabd19be98be002f6d478aaa31574d",
        strip_prefix = "bazel-skylib-2169ae1c374aab4a09aa90e65efe1a3aad4e279b",
        urls = ["https://github.com/bazelbuild/bazel-skylib/archive/2169ae1c374aab4a09aa90e65efe1a3aad4e279b.tar.gz"],
    )

    pip_repositories()

    pip_install()

    tf_workspace("", "@org_tensorflow")

    maven_install(
        artifacts = [
            "org.antlr:antlr4:jar:4.7.1",
            "com.beust:jcommander:1.72",
            "com.fasterxml.jackson.core:jackson-core:2.9.5",
            "com.fasterxml.jackson.core:jackson-databind:2.9.5",
            "com.fasterxml.jackson.core:jackson-annotations:2.9.5",
            "com.google.cloud:google-cloud-bigquery:1.38.0",
            "com.google.code.gson:gson:2.8.5",
            "com.google.errorprone:error_prone_annotations:2.3.3",
            "com.google.guava:guava:26.0-jre",
            "com.google.http-client:google-http-client-gson:1.24.1",
            "com.google.truth:truth:0.42",
            "junit:junit:4.12",
            "org.apache.beam:beam-runners-direct-java:2.9.0",
            "org.apache.beam:beam-runners-google-cloud-dataflow-java:2.9.0",
            "org.apache.beam:beam-sdks-java-core:2.9.0",
            "org.slf4j:slf4j-simple:1.7.25",
        ],
        repositories = [
            "https://maven.google.com",
            "https://repo1.maven.org/maven2",
        ],
    )

    native.bind(
        name = "gson",
        actual = "@maven//:com_google_code_gson_gson",
    )

    native.bind(
        name = "guava",
        actual = "@maven//:com_google_guava_guava",
    )

    native.bind(
        name = "error_prone_annotations",
        actual = "@maven//:com_google_errorprone_error_prone_annotations",
    )
