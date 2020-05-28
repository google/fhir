load("@google_bazel_common//tools/jarjar:jarjar.bzl", "jarjar_library")

package(
    # No Bazel target should depend on these
    default_visibility = ["//visibility:private"],
)

licenses(["notice"])

# Shading rules for creating Java library
SHADE_RULES = [
    "rule com.google.protobuf.** com.google.fhir.shaded.protobuf.@1",
    "rule com.google.common.** com.google.fhir.shaded.common.@1",
    "rule com.google.devtools.** com.google.fhir.shaded.devtools.@1",
    "rule com.google.errorprone.** com.google.fhir.shaded.errorprone.@1",
    "rule com.google.gson.** com.google.fhir.shaded.gson.@1",
    "rule com.google.j2objc.** com.google.fhir.shaded.j2objc.@1",
    "rule com.google.thirdparty.** com.google.fhir.shaded.thirdparty.@1",
    "rule javax.** com.google.fhir.shaded.thirdparty.@1",
    "rule org.** com.google.fhir.shaded.org.@1",
]

java_binary(
    name = "r4_bin",
    srcs = [
        "Release.java",
    ],
    main_class = "com.google.fhir.release.Release",
    runtime_deps = [
        "//java:json_format",
        "//proto/r4/core/resources:all_java_proto",
    ],
)

jarjar_library(
    name = "r4_lib",
    jars = [
        "//java/src/main/java/com/google/fhir/release:r4_bin_deploy.jar",
    ],
    rules = SHADE_RULES,
)
