workspace(name = "com_google_fhir")

load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

# Needed by TensorFlow. com_google_protobuf and com_google_googletest
# are also imported here.
http_archive(
    name = "io_bazel_rules_closure",
    sha256 = "7d206c2383811f378a5ef03f4aacbcf5f47fd8650f6abbc3fa89f3a27dd8b176",
    strip_prefix = "rules_closure-0.10.0",
    urls = [
        "https://github.com/bazelbuild/rules_closure/archive/0.10.0.tar.gz",
    ],
)

http_archive(
    name = "bazel_skylib",
    sha256 = "bbccf674aa441c266df9894182d80de104cabd19be98be002f6d478aaa31574d",
    strip_prefix = "bazel-skylib-2169ae1c374aab4a09aa90e65efe1a3aad4e279b",
    urls = ["https://github.com/bazelbuild/bazel-skylib/archive/2169ae1c374aab4a09aa90e65efe1a3aad4e279b.tar.gz"],
)

http_archive(
    name = "org_tensorflow",
    sha256 = "49b5f0495cd681cbcb5296a4476853d4aea19a43bdd9f179c928a977308a0617",
    strip_prefix = "tensorflow-2.0.0",
    urls = [
        "https://github.com/tensorflow/tensorflow/archive/v2.0.0.tar.gz",
    ],
)

http_archive(
    name = "rules_python",
    sha256 = "fa53cc0afe276d8f6675df1a424592e00e4f37b2a497e48399123233902e2e76",
    strip_prefix = "rules_python-0.0.1",
    urls = ["https://github.com/bazelbuild/rules_python/archive/0.0.1.tar.gz"],
)

# Used for the FHIRPath parser runtime.
http_archive(
    name = "antlr_cc_runtime",
    url = "https://www.antlr.org/download/antlr4-cpp-runtime-4.7.1-source.zip",
    sha256 = "23bebc0411052a260f43ae097aa1ab39869eb6b6aa558b046c367a4ea33d1ccc",
    strip_prefix = "runtime/src",
    build_file = "//bazel:antlr.BUILD",
)

load("@rules_python//python:pip.bzl", "pip_repositories", "pip_import")

pip_repositories()

pip_import(
    name="fhir_bazel_pip_dependencies",
    requirements="//bazel:requirements.txt",
)

load("@fhir_bazel_pip_dependencies//:requirements.bzl", "pip_install")
pip_install()

load("@org_tensorflow//tensorflow:workspace.bzl", "tf_workspace")
tf_workspace("", "@org_tensorflow")

# When possible, we fetch java dependencies from maven central, including
# transitive dependencies.
http_archive(
    name = "transitive_maven_jar_http",
    sha256 = "05a1bb89c4027d8fa0dc5e5404cca200526b2d6e87cddfe4262d971780da0d91",
    url = "https://github.com/bazelbuild/migration-tooling/archive/0f25a7e83f2f4b776fad9c8cb929ec9fa7cac87f.zip",
    type = "zip",
    strip_prefix = "migration-tooling-0f25a7e83f2f4b776fad9c8cb929ec9fa7cac87f",
)

# TODO: Update this to the newer, maintained rules_jvm_external as done below.
load("@transitive_maven_jar_http//transitive_maven_jar:transitive_maven_jar.bzl", "transitive_maven_jar")

transitive_maven_jar(
    name = "dependencies",
    artifacts = [
        "com.beust:jcommander:1.72",
        "com.fasterxml.jackson.core:jackson-core:2.9.5",
        "com.fasterxml.jackson.core:jackson-databind:2.9.5",
        "com.fasterxml.jackson.core:jackson-annotations:2.9.5",
        "com.google.errorprone:error_prone_annotations:2.3.3",
        "com.google.cloud:google-cloud-bigquery:1.38.0",
        "com.google.code.gson:gson:2.8.5",
        "com.google.truth:truth:0.42",
        "com.google.http-client:google-http-client-gson:1.24.1",
        "junit:junit:4.12",
        "org.apache.beam:beam-runners-direct-java:2.9.0",
        "org.apache.beam:beam-runners-google-cloud-dataflow-java:2.9.0",
        "org.apache.beam:beam-sdks-java-core:2.9.0",
        "org.slf4j:slf4j-simple:1.7.25",
    ]
)

load("@dependencies//:generate_workspace.bzl", "generated_maven_jars")
generated_maven_jars()

http_archive(
    name = "rules_jvm_external",
    strip_prefix = "rules_jvm_external-2.1",
    sha256 = "515ee5265387b88e4547b34a57393d2bcb1101314bcc5360ec7a482792556f42",
    url = "https://github.com/bazelbuild/rules_jvm_external/archive/2.1.zip",
)

load("@rules_jvm_external//:defs.bzl", "maven_install")

maven_install(
    artifacts = ["org.antlr:antlr4:jar:4.7.1"],
    repositories = [
        "https://maven.google.com",
        "https://repo1.maven.org/maven2",
    ],
)

maven_jar(
    name = "guava_maven",
    artifact = "com.google.guava:guava:26.0-jre",
    sha1 = "6a806eff209f36f635f943e16d97491f00f6bfab",
)

bind(
    name = "gson",
    actual = "@com_google_code_gson_gson//jar",
)

bind(
    name = "guava",
    actual = "@guava_maven//jar",
)

bind(
    name = "error_prone_annotations",
    actual = "@com_google_errorprone_error_prone_annotations//jar",
)

