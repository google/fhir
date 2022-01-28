# Data files containing the base FHIR specification.

load("//bazel:protogen.bzl", "fhir_package")

package(default_visibility = ["//visibility:public"])

licenses(["notice"])

exports_files(["uscore_package_info.prototxt"])

# Current DSTU2 package
filegroup(
    name = "dstu2",
    srcs = glob([
        "hl7.fhir.core/1.0.2/package/*.json",
    ]),
)

# Current STU3 package
filegroup(
    name = "stu3",
    srcs = glob([
        "hl7.fhir.core/3.0.1/package/*.json",
    ]),
)

# STU3 core types.
fhir_package(
    name = "fhir_stu3",
    definitions = glob(
        ["hl7.fhir.core/3.0.1/package/StructureDefinition-*.json"],
        exclude = [
            "hl7.fhir.core/3.0.1/package/StructureDefinition-StructureDefinition.json",
            "hl7.fhir.core/3.0.1/package/StructureDefinition-cqif-condition.json",
            "hl7.fhir.core/3.0.1/package/StructureDefinition-cqif-cqlExpression.json",
            "hl7.fhir.core/3.0.1/package/StructureDefinition-cqif-fhirPathExpression.json",
            "hl7.fhir.core/3.0.1/package/StructureDefinition-cqif-library.json",
            "hl7.fhir.core/3.0.1/package/StructureDefinition-familymemberhistory-genetic.json",
        ],
    ) + glob([
        "hl7.fhir.core/3.0.1/modified/StructureDefinition-*.json",
    ]) + glob(
        [
            "hl7.fhir.core/3.0.1/package/ValueSet-*.json",
        ],
    ) + glob(
        [
            "hl7.fhir.core/3.0.1/package/CodeSystem-*.json",
        ],
    ),
    package_info = "fhir_stu3_package_info.prototxt",
)

# Current R4 package
filegroup(
    name = "r4",
    srcs = glob([
        "hl7.fhir.core/4.0.1/package/*.json",
    ]),
)

# Example testdata, e.g., Patient-example.json, was dropped in the 4.0.1 npm
filegroup(
    name = "r4_examples",
    srcs = glob([
        "hl7.fhir.r4.examples/4.0.1/package/*",
    ]),
)

fhir_package(
    name = "fhir_r4",
    definitions =
        glob(
            ["hl7.fhir.core/4.0.1/package/StructureDefinition-*.json"],
            exclude = [
                "hl7.fhir.core/4.0.1/package/StructureDefinition-example-*.json",
                "hl7.fhir.core/4.0.1/package/StructureDefinition-*.*.json",
                "hl7.fhir.core/4.0.1/package/StructureDefinition-structuredefinition-xml-type.json",
                "hl7.fhir.core/4.0.1/package/StructureDefinition-structuredefinition-rdf-type.json",
                "hl7.fhir.core/4.0.1/package/StructureDefinition-structuredefinition-json-type.json",
            ],
        ) + glob(
            [
                "hl7.fhir.core/4.0.1/package/ValueSet-*.json",
            ],
        ) + glob(
            ["hl7.fhir.core/4.0.1/package/CodeSystem-*.json"],
        ) + glob(
            ["hl7.fhir.core/4.0.1/package/SearchParameter-*.json"],
            exclude = [
                "hl7.fhir.core/4.0.1/package/SearchParameter-*-extensions-*.json",
            ],
        ),
    package_info = "fhir_r4_package_info.prototxt",
)

# Common FHIR R4 elements for lightweight use without the full specification.
# TODO: Remove this and update users when the larger fhir_r4 package loads efficiently.
fhir_package(
    name = "fhir_r4_core_structures",
    definitions =
        [
            "hl7.fhir.core/4.0.1/package/StructureDefinition-Address.json",
            "hl7.fhir.core/4.0.1/package/StructureDefinition-Age.json",
            "hl7.fhir.core/4.0.1/package/StructureDefinition-Annotation.json",
            "hl7.fhir.core/4.0.1/package/StructureDefinition-Attachment.json",
            "hl7.fhir.core/4.0.1/package/StructureDefinition-BackboneElement.json",
            "hl7.fhir.core/4.0.1/package/StructureDefinition-Count.json",
            "hl7.fhir.core/4.0.1/package/StructureDefinition-boolean.json",
            "hl7.fhir.core/4.0.1/package/StructureDefinition-code.json",
            "hl7.fhir.core/4.0.1/package/StructureDefinition-date.json",
            "hl7.fhir.core/4.0.1/package/StructureDefinition-dateTime.json",
            "hl7.fhir.core/4.0.1/package/StructureDefinition-DataRequirement.json",
            "hl7.fhir.core/4.0.1/package/StructureDefinition-Dosage.json",
            "hl7.fhir.core/4.0.1/package/StructureDefinition-Distance.json",
            "hl7.fhir.core/4.0.1/package/StructureDefinition-CodeableConcept.json",
            "hl7.fhir.core/4.0.1/package/StructureDefinition-Coding.json",
            "hl7.fhir.core/4.0.1/package/StructureDefinition-ContactDetail.json",
            "hl7.fhir.core/4.0.1/package/StructureDefinition-ContactPoint.json",
            "hl7.fhir.core/4.0.1/package/StructureDefinition-Contributor.json",
            "hl7.fhir.core/4.0.1/package/StructureDefinition-Duration.json",
            "hl7.fhir.core/4.0.1/package/StructureDefinition-Element.json",
            "hl7.fhir.core/4.0.1/package/StructureDefinition-Expression.json",
            "hl7.fhir.core/4.0.1/package/StructureDefinition-Extension.json",
            "hl7.fhir.core/4.0.1/package/StructureDefinition-HumanName.json",
            "hl7.fhir.core/4.0.1/package/StructureDefinition-Identifier.json",
            "hl7.fhir.core/4.0.1/package/StructureDefinition-instant.json",
            "hl7.fhir.core/4.0.1/package/StructureDefinition-integer.json",
            "hl7.fhir.core/4.0.1/package/StructureDefinition-time.json",
            "hl7.fhir.core/4.0.1/package/StructureDefinition-Meta.json",
            "hl7.fhir.core/4.0.1/package/StructureDefinition-Money.json",
            "hl7.fhir.core/4.0.1/package/StructureDefinition-Narrative.json",
            "hl7.fhir.core/4.0.1/package/StructureDefinition-Patient.json",
            "hl7.fhir.core/4.0.1/package/StructureDefinition-Observation.json",
            "hl7.fhir.core/4.0.1/package/StructureDefinition-ParameterDefinition.json",
            "hl7.fhir.core/4.0.1/package/StructureDefinition-Period.json",
            "hl7.fhir.core/4.0.1/package/StructureDefinition-Quantity.json",
            "hl7.fhir.core/4.0.1/package/StructureDefinition-Range.json",
            "hl7.fhir.core/4.0.1/package/StructureDefinition-Ratio.json",
            "hl7.fhir.core/4.0.1/package/StructureDefinition-Reference.json",
            "hl7.fhir.core/4.0.1/package/StructureDefinition-RelatedArtifact.json",
            "hl7.fhir.core/4.0.1/package/StructureDefinition-Resource.json",
            "hl7.fhir.core/4.0.1/package/StructureDefinition-SampledData.json",
            "hl7.fhir.core/4.0.1/package/StructureDefinition-Signature.json",
            "hl7.fhir.core/4.0.1/package/StructureDefinition-Timing.json",
            "hl7.fhir.core/4.0.1/package/StructureDefinition-TriggerDefinition.json",
            "hl7.fhir.core/4.0.1/package/StructureDefinition-UsageContext.json",
            "hl7.fhir.core/4.0.1/package/StructureDefinition-string.json",
            "hl7.fhir.core/4.0.1/package/StructureDefinition-uri.json",
            "hl7.fhir.core/4.0.1/package/StructureDefinition-url.json",
        ],
    package_info = "fhir_r4_package_info.prototxt",
)

# US-Core profiles for FHIR STU3.
fhir_package(
    name = "uscore_stu3_package",
    definitions = glob([
        "hl7.fhir.us.core/2.0.0/package/StructureDefinition-*.json",
    ]),
    package_info = "uscore_stu3_package_info.prototxt",
)

# US-Core Profiles for FHIR R4

fhir_package(
    name = "uscore_r4_package",
    definitions = glob([
        "hl7.fhir.us.core/3.1.0/package/StructureDefinition-*.json",
    ]) + glob(["hl7.fhir.us.core/3.1.0/package/ValueSet-*.json"]) + glob([
        "hl7.fhir.us.core/3.1.0/package/CodeSystem-*.json",
    ]),
    package_info = "uscore_r4_package_info.prototxt",
)

# US QI core profiles.
fhir_package(
    name = "qicore_3_2_0_package",
    definitions = glob([
        "hl7.fhir.us.qicore/3.2.0/*.json",
    ]),
    package_info = "qicore_stu3_package_info.prototxt",
)

# US QI core profiles.
fhir_package(
    name = "qicore_3_3_0_package",
    definitions = glob([
        "hl7.fhir.us.qicore/3.3.0/*.json",
    ]),
    package_info = "qicore_r4_package_info.prototxt",
)
