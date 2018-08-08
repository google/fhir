workspace(name = "com_google_fhir")

# defines py_proto_library.
git_repository(
    name = "protobuf_bzl",
    # v3.6.0
    commit = "ab8edf1dbe2237b4717869eaab11a2998541ad8d",
    remote = "https://github.com/google/protobuf.git",
)

# required by py_proto_library.
new_http_archive(
    name = "six_archive",
    build_file = "@protobuf_bzl//:six.BUILD",
    sha256 = "105f8d68616f8248e24bf0e9372ef04d3cc10104f1980f54d57b2ce73a5ad56a",
    url = "https://pypi.python.org/packages/source/s/six/six-1.10.0.tar.gz#md5=34eed507548117b2ab523ab14b2f8b55",
)

bind(
    name = "six",
    actual = "@six_archive//:six",
)

# proto_library, cc_proto_library, and java_proto_library rules implicitly
# depend on @com_google_protobuf for protoc and proto runtimes.
# This statement defines the @com_google_protobuf repo.
http_archive(
    name = "com_google_protobuf",
    sha256 = "cef7f1b5a7c5fba672bec2a319246e8feba471f04dcebfe362d55930ee7c1c30",
    strip_prefix = "protobuf-3.5.0",
    urls = ["https://github.com/google/protobuf/archive/v3.5.0.zip"],
)

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


