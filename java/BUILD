# Tools for working with FHIR protocol buffers in java.
package(default_visibility = ["//visibility:public"])

licenses(["notice"])

java_library(
    name = "stu3",
    srcs = glob(["src/main/java/com/google/fhir/stu3/*.java"]),
    deps = [
        "//external:gson",
        "//external:guava",
        "//proto/stu3:datatypes_java_proto",
        "//proto/stu3:extensions_java_proto",
        "//proto/stu3:resources_java_proto",
    ],
)

java_binary(
    name = "FhirToProto",
    srcs = [
        "src/main/java/com/google/fhir/examples/FhirToProtoMain.java",
    ],
    main_class = "com.google.fhir.examples.FhirToProtoMain",
    deps = [
        ":stu3",
        "//external:guava",
        "//proto/stu3:resources_java_proto",
        "@com_google_protobuf//:protobuf_java_util",
    ],
)

java_test(
    name = "AnnotationUtilsTest",
    size = "small",
    srcs = ["src/test/java/com/google/fhir/stu3/AnnotationUtilsTest.java"],
    test_class = "com.google.fhir.stu3.AnnotationUtilsTest",
    deps = [
        ":stu3",
        "//external:junit",
        "//external:truth",
        "//proto/stu3:datatypes_java_proto",
        "//proto/stu3:resources_java_proto",
    ],
)

java_test(
    name = "DateTimeWrapperTest",
    size = "small",
    srcs = ["src/test/java/com/google/fhir/stu3/DateTimeWrapperTest.java"],
    test_class = "com.google.fhir.stu3.DateTimeWrapperTest",
    deps = [
        ":stu3",
        "//external:junit",
        "//external:truth",
        "//proto/stu3:datatypes_java_proto",
    ],
)

java_test(
    name = "DateWrapperTest",
    size = "small",
    srcs = ["src/test/java/com/google/fhir/stu3/DateWrapperTest.java"],
    test_class = "com.google.fhir.stu3.DateWrapperTest",
    deps = [
        ":stu3",
        "//external:junit",
        "//external:truth",
        "//proto/stu3:datatypes_java_proto",
    ],
)

java_test(
    name = "DecimalWrapperTest",
    size = "small",
    srcs = ["src/test/java/com/google/fhir/stu3/DecimalWrapperTest.java"],
    test_class = "com.google.fhir.stu3.DecimalWrapperTest",
    deps = [
        ":stu3",
        "//external:junit",
        "//external:truth",
        "//proto/stu3:datatypes_java_proto",
    ],
)

java_test(
    name = "ExtensionWrapperTest",
    size = "medium",
    srcs = ["src/test/java/com/google/fhir/stu3/ExtensionWrapperTest.java"],
    data = [
        "//testdata/stu3:extensions",
    ],
    test_class = "com.google.fhir.stu3.ExtensionWrapperTest",
    deps = [
        ":stu3",
        "//external:guava",
        "//external:junit",
        "//external:truth",
        "//proto/stu3:datatypes_java_proto",
        "//proto/stu3:extensions_java_proto",
    ],
)

java_test(
    name = "InstantWrapperTest",
    size = "small",
    srcs = ["src/test/java/com/google/fhir/stu3/InstantWrapperTest.java"],
    test_class = "com.google.fhir.stu3.InstantWrapperTest",
    deps = [
        ":stu3",
        "//external:junit",
        "//external:truth",
        "//proto/stu3:datatypes_java_proto",
    ],
)

java_test(
    name = "JsonFormatTest",
    size = "medium",
    srcs = ["src/test/java/com/google/fhir/stu3/JsonFormatTest.java"],
    data = [
        "//testdata/stu3:examples",
    ],
    test_class = "com.google.fhir.stu3.JsonFormatTest",
    deps = [
        ":stu3",
        "//external:gson",
        "//external:guava",
        "//external:junit",
        "//external:truth",
        "//proto/stu3:resources_java_proto",
    ],
)

java_test(
    name = "TimeWrapperTest",
    size = "small",
    srcs = ["src/test/java/com/google/fhir/stu3/TimeWrapperTest.java"],
    test_class = "com.google.fhir.stu3.TimeWrapperTest",
    deps = [
        ":stu3",
        "//external:junit",
        "//external:truth",
        "//proto/stu3:datatypes_java_proto",
    ],
)

java_test(
    name = "ValidationTest",
    size = "medium",
    srcs = ["src/test/java/com/google/fhir/stu3/ValidationTest.java"],
    data = [
        "//testdata/stu3:validation",
    ],
    test_class = "com.google.fhir.stu3.ValidationTest",
    deps = [
        ":stu3",
        "//external:guava",
        "//external:junit",
        "//external:truth",
        "//proto/stu3:datatypes_java_proto",
    ],
)
