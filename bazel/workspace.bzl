""" Provides a function for setting up the FhirProto workspace """

load("@rules_jvm_external//:defs.bzl", "maven_install")

def fhirproto_workspace(core_lib = False):
    """ Sets up FhirProto workspace """

    maven_install(
        artifacts = [
            "com.beust:jcommander:1.72",
            "com.fasterxml.jackson.core:jackson-annotations:2.9.5",
            "com.fasterxml.jackson.core:jackson-core:2.9.5",
            "com.fasterxml.jackson.core:jackson-databind:2.9.5",
            "com.google.cloud:google-cloud-bigquery:1.38.0",
            "com.google.code.gson:gson:2.8.6",
            "com.google.errorprone:error_prone_annotations:2.3.3",
            "com.google.guava:guava-testlib:31.1-jre",
            "com.google.guava:guava:31.1-jre",
            "com.google.http-client:google-http-client-gson:1.43.0",
            "com.google.testparameterinjector:test-parameter-injector:1.15",
            "com.google.truth:truth:0.42",
            "com.google.truth.extensions:truth-proto-extension:1.1",
            "junit:junit:4.13-rc-1",
            "org.antlr:antlr4:jar:4.7.1",
            "org.apache.beam:beam-runners-direct-java:2.9.0",
            "org.apache.beam:beam-runners-google-cloud-dataflow-java:2.9.0",
            "org.apache.beam:beam-sdks-java-core:2.9.0",
            "org.apache.commons:commons-compress:1.21",
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

    protogen_prefix = "//" if core_lib else "@com_google_fhir//"
    native.bind(
        name = "protogen",
        actual = protogen_prefix + "java/com/google/fhir/protogen:protogen",
    )
    native.bind(
        name = "GeneratedProtoTest.java",
        actual = protogen_prefix + "javatests/com/google/fhir/protogen:GeneratedProtoTest.java",
    )
