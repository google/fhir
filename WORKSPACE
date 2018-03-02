workspace(name = "com_google_fhir")

# proto_library, cc_proto_library, and java_proto_library rules implicitly
# depend on @com_google_protobuf for protoc and proto runtimes.
# This statement defines the @com_google_protobuf repo.
http_archive(
    name = "com_google_protobuf",
    sha256 = "cef7f1b5a7c5fba672bec2a319246e8feba471f04dcebfe362d55930ee7c1c30",
    strip_prefix = "protobuf-3.5.0",
    urls = ["https://github.com/google/protobuf/archive/v3.5.0.zip"],
)

maven_jar(
    name = "guava_maven",
    artifact = "com.google.guava:guava:24.0-jre",
)

bind(
    name = "guava",
    actual = "@guava_maven//jar",
)

maven_jar(
    name = "gson_maven",
    artifact = "com.google.code.gson:gson:2.7",
)

bind(
    name = "gson",
    actual = "@gson_maven//jar",
)

maven_jar(
    name = "junit_maven",
    artifact = "junit:junit:4.12",
)

bind(
    name = "junit",
    actual = "@junit_maven//jar",
)

maven_jar(
    name = "truth_maven",
    artifact = "com.google.truth:truth:0.39",
)

bind(
    name = "truth",
    actual = "@truth_maven//jar",
)

