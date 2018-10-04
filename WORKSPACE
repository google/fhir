workspace(name = "com_google_fhir")

# Needed by TensorFlow. com_google_protobuf and com_google_googletest
# are also imported here.
http_archive(
    name = "io_bazel_rules_closure",
    sha256 = "a38539c5b5c358548e75b44141b4ab637bba7c4dc02b46b1f62a96d6433f56ae",
    strip_prefix = "rules_closure-dbb96841cc0a5fb2664c37822803b06dab20c7d1",
    urls = [
        "https://mirror.bazel.build/github.com/bazelbuild/rules_closure/archive/dbb96841cc0a5fb2664c37822803b06dab20c7d1.tar.gz",
        "https://github.com/bazelbuild/rules_closure/archive/dbb96841cc0a5fb2664c37822803b06dab20c7d1.tar.gz",  # 2018-04-13
    ],
)

# TensorFlow v1.11.0-rc1 (2018-09-17). com_google_absl is also provided by
# tensorflow.
http_archive(
    name = "org_tensorflow",
    sha256 = "fe1a59c8efffc4b6a8c55120bc8ccf8ebfb38617bb7af221729459fe18a5397f",
    strip_prefix = "tensorflow-1.11.0-rc1",
    urls = [
        "https://github.com/tensorflow/tensorflow/archive/v1.11.0-rc1.tar.gz",
    ],
)

git_repository(
    name="io_bazel_rules_python",
    remote="https://github.com/bazelbuild/rules_python.git",
    commit="b25495c47eb7446729a2ed6b1643f573afa47d99", # April 6, 2018
)

load("@io_bazel_rules_python//python:pip.bzl", "pip_repositories")

pip_repositories()

load("@io_bazel_rules_python//python:pip.bzl", "pip_import")

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

load("@transitive_maven_jar_http//transitive_maven_jar:transitive_maven_jar.bzl", "transitive_maven_jar")

transitive_maven_jar(
    name = "dependencies",
    artifacts = [
        "com.beust:jcommander:1.72",
        "com.google.cloud:google-cloud-bigquery:1.38.0",
        "com.google.code.gson:gson:2.8.5",
        "com.google.truth:truth:0.42",
        "com.google.http-client:google-http-client-gson:1.24.1",
        "junit:junit:4.12",
    ]
)

load("@dependencies//:generate_workspace.bzl", "generated_maven_jars")
generated_maven_jars()

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


