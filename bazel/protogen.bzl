# Copyright 2018 Google LLC
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     https://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
"""Rules for generating Protos from FHIR definitions"""

load("@rules_java//java:defs.bzl", "java_binary", "java_test")

R4_PACKAGE_DEP = "@com_google_fhir//spec:fhir_r4"
PROTO_GENERATOR = "@com_google_fhir//java/com/google/fhir/protogen:ProtoGenerator"
PROFILE_GENERATOR = "@com_google_fhir//java/com/google/fhir/protogen:ProfileGenerator"

MANUAL_TAGS = ["manual"]

def fhir_package(
        name,
        definitions,
        package_info = None):
    """Generates a FHIR package in a way that can be referenced by other packages

    Given a zip of resources, and a package info proto, this generates a zip file and a filegroup for the package.

    Args:
        name: The name for the package, which other targets will use to refer to this
        definitions: the definitions to export with this package
        package_info: if set, the package_info to export with this package
    """
    filegroup_name = name + "_filegroup"
    file_srcs = definitions
    if package_info:
        file_srcs.append(package_info)
    native.filegroup(
        name = filegroup_name,
        srcs = file_srcs,
    )
    native.genrule(
        name = name + "_package_zip",
        srcs = [filegroup_name],
        outs = [_get_zip_for_pkg(name)],
        cmd = "zip --quiet -j $@ $(locations %s)" % filegroup_name,
        tags = MANUAL_TAGS,
    )

    # Create a package in NPM format, which is just a tar.gz file containing its
    # files under a "package" directory. This is the standard way to distribute
    # FHIR packages. See for example the artifacts published for IGs at
    # https://hl7.org/fhir/downloads.html.
    native.genrule(
        name = name + "_package_tar",
        srcs = [filegroup_name],
        outs = [_get_tar_for_pkg(name)],
        # --dereference to follow symlinks
        # --transform 's,.*\\/,package/,' to remove directory names (.*\/ all
        # characters up to the (final) slash) and replace them with just a
        # 'package' directory.
        cmd = "tar czf $@ $(locations %s) --dereference --transform 's,.*\\/,package/,'" % filegroup_name,
        tags = MANUAL_TAGS,
    )

def _src_dir(label):
    return "\"$$(dirname $(rootpath %s))\"" % label

def gen_fhir_protos(
        name,
        package,
        package_deps = [],
        additional_proto_imports = [],
        disable_test = False,
        golden_java_proto_rules = []):
    """Generates a proto file from a fhir_package

    These rules should be run by the generate_protos.sh script, which will generate the
    protos and move them into the source directory.
    e.g., for a gen_fhir_protos rule in foo/bar with name = quux,
    bazel/generate_protos.sh foo/bar:quux

    Args:
      name: The name for the generated proto files (without .proto)
      package: The fhir_package to generate protos for.
      package_deps: Any fhir_packages these definitions depend on.
          Core fhir definitions are automatically included.
      additional_proto_imports: Additional proto files the generated protos should import.
          FHIR datatypes, annotations, and codes are included automatically.
      disable_test: If true, will not declare a GeneratedProtoTest
      golden_java_proto_rules: If non-empty, will add these java proto rules as runtime deps to the
          generator. Any generated proto with a corresponding proto available to the runtime will
          preserve the tag numbers from that proto.  This should be left empty when generating the
          first version of protos.
    """

    # Get the directory that the rule is being run out of by using the directory of a known output.
    src_dir = _src_dir(":" + name + ".zip")

    flags = [
        "--output_directory $(@D)",
        "--output_name " + name,
        "--r4_core_dep $(location %s)" % _get_zip_for_pkg(R4_PACKAGE_DEP),
        "--input_package $(location %s)" % _get_zip_for_pkg(package),
    ]

    flags.extend([
        "--fhir_definition_dep $(location %s)" % _get_zip_for_pkg(dep)
        for dep in (package_deps + [package])
    ])

    flags.extend([
        "--additional_import %s" % proto_import
        for proto_import in additional_proto_imports
    ])

    flags.append("--directory_in_source " + src_dir)

    all_fhir_pkgs = package_deps + [
        package,
        R4_PACKAGE_DEP,
    ]
    src_pkgs = [_get_zip_for_pkg(pkg) for pkg in all_fhir_pkgs]

    proto_generator_tool = PROTO_GENERATOR
    if len(golden_java_proto_rules) > 0:
        proto_generator_tool = "Generator_" + name
        _proto_generator_with_runtime_deps_on_existing_protos(proto_generator_tool, golden_java_proto_rules)

    native.genrule(
        name = name + "_proto_zip",
        outs = ["%s.zip" % name],
        srcs = src_pkgs,
        tools = [proto_generator_tool],
        cmd = "$(location %s)" % proto_generator_tool + " " + " ".join(flags),
        tags = MANUAL_TAGS,
    )

    native.filegroup(
        name = name + "_proto_golden_files",
        srcs = native.glob(["*.proto"]),
        testonly = 1,
    )
    if not disable_test:
        additional_import_test_flag = ",".join(additional_proto_imports)
        deps_test_flag = ",".join(["$(location %s)" % _get_zip_for_pkg(dep) for dep in package_deps])
        test_flags = [
            "-Dgenerated_zip=$(location :%s.zip)" % name,
            "-Dgolden_dir=" + src_dir,
            "-Xmx4096M",
        ]
        java_test(
            name = "GeneratedProtoTest_" + name,
            size = "medium",
            srcs = ["//external:GeneratedProtoTest.java"],
            jvm_flags = test_flags,
            data = src_pkgs + [":%s.zip" % name, "%s_proto_golden_files" % name],
            test_class = "com.google.fhir.protogen.GeneratedProtoTest",
            deps = [
                "//external:protogen",
                "@maven//:com_google_guava_guava",
                "@maven//:com_google_truth_truth",
                "@maven//:junit_junit",
                "@com_google_protobuf//:protobuf_java",
            ],
        )

def gen_fhir_definitions_and_protos(
        name,
        package_info,
        extensions = [],
        profiles = [],
        terminologies = [],
        package_deps = [],
        additional_proto_imports = [],
        disable_test = False,
        golden_java_proto_rules = [],
        package_json = None):
    """Generates structure definitions and protos based on Extensions and Profiles protos.

    These rules should be run by bazel/generate_protos.sh, which will generate the
    profiles and protos and move them into the source directory.
    e.g., bazel/generate_protos.sh foo/bar:quux

    This also exports a fhir_package rule, so that this target
    can be used as a dependency of other gen_fhir_definitions_and_protos.

    Args:
      name: name prefix for all generated rules
      package_info: Metadata shared by all generated Structure Definitions.
      extensions: List of Extensions prototxt files
      profiles: List of Profiles prototxt files.
      terminologies: List of Terminologies prototxt files.
      package_deps: Any package_deps these definitions depend on.
                    Core fhir definitions are automatically included.
      additional_proto_imports: Additional proto files the generated profiles should import
                                FHIR datatypes, annotations, and codes are included automatically.
      disable_test: If true, will not declare a GeneratedProtoTest
      golden_java_proto_rules: If non-empty, will add these java proto rules as runtime deps to the
          generator. Any generated proto with a corresponding proto available to the runtime will
          preserve the tag numbers from that proto.  This should be left empty when generating the
          first version of protos.
      package_json: The path of the package.json metadata file for the package.
    """

    profile_flags = " ".join([("--profiles $(location %s) " % profile) for profile in profiles])
    extension_flags = " ".join([("--extensions $(location %s) " % extension) for extension in extensions])
    terminology_flags = " ".join([("--terminologies $(location %s) " % terminology) for terminology in terminologies])

    struct_def_dep_zip_flags = " ".join([
        ("--struct_def_dep_zip $(location %s) " % _get_zip_for_pkg(dep))
        for dep in package_deps
    ])

    fhir_definition_srcs = ([
                                package_info,
                                _get_zip_for_pkg(R4_PACKAGE_DEP),
                            ] +
                            profiles +
                            extensions +
                            terminologies +
                            [_get_zip_for_pkg(dep) for dep in package_deps])

    json_outs = [
        name + ".json",
        name + "_extensions.json",
        name + "_terminologies.json",
    ]

    native.genrule(
        name = name + "_definitions",
        outs = json_outs,
        srcs = fhir_definition_srcs,
        tools = [
            PROFILE_GENERATOR,
        ],
        cmd = ("""
            $(location %s) \
                --output_directory $(@D) \
                --name %s \
                --package_info $(location %s) \
                --r4_struct_def_zip $(location %s) \
                %s %s %s %s""" % (
            PROFILE_GENERATOR,
            name,
            package_info,
            _get_zip_for_pkg(R4_PACKAGE_DEP),
            struct_def_dep_zip_flags,
            profile_flags,
            extension_flags,
            terminology_flags,
        )),
        tags = MANUAL_TAGS,
    )

    if package_json and package_json.rpartition("/")[-1] != "package.json":
        # If the file is not named 'package.json', create a copy of the file
        # named package.json and add the renamed file into the archive.
        # Place the copy in a new directory to avoid clashes with other
        # files. When we create the archive, we strip the directory names.
        package_rename_rule = name + "_package_json_rename"
        package_rename_path = "%s/package.json" % package_rename_rule
        native.genrule(
            name = package_rename_rule,
            srcs = [package_json],
            outs = [package_rename_path],
            cmd = "cp $< $@",
        )

    fhir_package(
        name = name,
        definitions = native.glob([
            "**/*.json",
        ]),
        package_info = package_info,
    )

    gen_fhir_protos(
        name = name,
        package = name,
        package_deps = package_deps,
        additional_proto_imports = additional_proto_imports,
        disable_test = disable_test,
        golden_java_proto_rules = golden_java_proto_rules,
    )

def _get_zip_for_pkg(pkg):
    return pkg + "_package.zip"

def _get_tar_for_pkg(pkg):
    return pkg + "_package.tgz"

def _proto_generator_with_runtime_deps_on_existing_protos(name, golden_java_protos_rules):
    java_binary(
        name = name,
        main_class = "com.google.fhir.protogen.ProtoGeneratorMain",
        runtime_deps = golden_java_protos_rules +
                       ["//java/com/google/fhir/protogen:proto_generator_binary_lib"],
    )
