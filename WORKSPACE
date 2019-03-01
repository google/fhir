workspace(name = "com_google_fhir")

load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

# Needed by TensorFlow. com_google_protobuf and com_google_googletest
# are also imported here.
http_archive(
    name = "io_bazel_rules_closure",
    sha256 = "43c9b882fa921923bcba764453f4058d102bece35a37c9f6383c713004aacff1",
    strip_prefix = "rules_closure-9889e2348259a5aad7e805547c1a0cf311cfcd91",
    urls = [
        "https://mirror.bazel.build/github.com/bazelbuild/rules_closure/archive/9889e2348259a5aad7e805547c1a0cf311cfcd91.tar.gz",
        "https://github.com/bazelbuild/rules_closure/archive/9889e2348259a5aad7e805547c1a0cf311cfcd91.tar.gz",  # 2018-12-21
    ],
)

http_archive(
    name = "org_tensorflow",
    sha256 = "7cd19978e6bc7edc2c847bce19f95515a742b34ea5e28e4389dade35348f58ed",
    strip_prefix = "tensorflow-1.13.1",
    urls = [
        "https://github.com/tensorflow/tensorflow/archive/v1.13.1.tar.gz",
    ],
)

http_archive(
    name = "io_bazel_rules_python",
    sha256 = "8b32d2dbb0b0dca02e0410da81499eef8ff051dad167d6931a92579e3b2a1d48",
    strip_prefix = "rules_python-8b5d0683a7d878b28fffe464779c8a53659fc645",
    urls = ["https://github.com/bazelbuild/rules_python/archive/8b5d0683a7d878b28fffe464779c8a53659fc645.tar.gz"],
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
        "com.fasterxml.jackson.core:jackson-core:2.9.5",
        "com.fasterxml.jackson.core:jackson-databind:2.9.5",
        "com.fasterxml.jackson.core:jackson-annotations:2.9.5",
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


