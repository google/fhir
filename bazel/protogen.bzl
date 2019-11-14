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

STU3_PACKAGE_DEP = "@com_google_fhir//spec:fhir_stu3_package"
R4_PACKAGE_DEP = "@com_google_fhir//spec:fhir_r4_package"
PROTO_GENERATOR = "@com_google_fhir//java:ProtoGenerator"
PROFILE_GENERATOR = "@com_google_fhir//java:ProfileGenerator"

MANUAL_TAGS = ["manual"]

def fhir_package(
        package_name,
        definitions,
        package_info):
    """Generates a FHIR package in a way that can be referenced by other packages

    Given a zip of resources, and a package info proto, this generates a zip file and a filegroup for the package.
    This zip file will have the name $(package_name).zip, which can be used to construct a java FhirPackage object.

    Args:
        package_name: The name for the package, which other targets will use to refer to this
        definitions: the definitions to export with this package
        package_info: the package_info to export with this package
    """
    filegroup_name = package_name + "_filegroup"
    native.filegroup(
        name = filegroup_name,
        srcs = definitions + [package_info],
    )
    native.genrule(
        name = "_genrule_" + package_name,
        srcs = [filegroup_name],
        outs = [_get_zip_for_pkg(package_name)],
        cmd = "zip --quiet -j $@ $(locations %s)" % filegroup_name,
        tags = MANUAL_TAGS,
    )

def gen_fhir_protos(
        name,
        package,
        package_deps = [],
        additional_proto_imports = None,
        separate_extensions = False):
    """Generates a proto file from a fhir_package

    These rules should be run by the generate_protos.sh script, which will generate the
    protos and move them into the source directory.
    e.g., for a gen_fhir_protos rule in foo/bar with name = quux,
    bazel/generate_protos.sh foo/bar:quux

    Args:
      name: The name for the generated proto file (without .proto)
      package: The fhir_package to generate protos for.
      package_deps: Any fhir_packages these definitions depend on.
                    Core fhir definitions are automatically included.
      additional_proto_imports: Additional proto files the generated protos should import.
                                FHIR datatypes, annotations, and codes are included automatically.
      separate_extensions: If true, will produce two proto files, one for extensions
                           and one for profiles.
    """

    struct_def_dep_flags = " ".join([
        "--fhir_definition_dep $(location %s)" % _get_zip_for_pkg(dep)
        for dep in (package_deps + [package])
    ])
    if not additional_proto_imports:
        additional_proto_imports = []
    if separate_extensions:
        # Also add the extensions proto files as an import to the main file.
        # Unfortunately we don't have an easy way to get the directory that
        # a genrule runs out of, but it's pretty easy to deduce from the output
        # directory - we just cut off everything up to and including "genfiles/"
        src_dir = "$$(GENDIR=$(@D) && echo $${GENDIR##*genfiles/})"
        additional_proto_imports += [src_dir + "/" + name + "_extensions.proto"]

    additional_proto_imports_flags = " ".join([
        "--additional_import %s" % proto_import
        for proto_import in additional_proto_imports
    ])
    cmd = """
        $(location %s) \
            --emit_proto \
            --emit_codes \
            --output_directory $(@D) \
            --stu3_core_dep $(location %s) \
            --r4_core_dep $(location %s) \
            --output_name _genfiles_%s \
            --input_package $(location %s) \
            """ % (
        PROTO_GENERATOR,
        _get_zip_for_pkg(STU3_PACKAGE_DEP),
        _get_zip_for_pkg(R4_PACKAGE_DEP),
        name,
        _get_zip_for_pkg(package),
    )

    cmd += additional_proto_imports_flags + " " + struct_def_dep_flags

    outs = ["_genfiles_" + name + ".proto", "_genfiles_" + name + "_codes.proto"]

    if separate_extensions:
        outs += ["_genfiles_" + name + "_extensions.proto"]

    all_fhir_pkgs = package_deps + [
        package,
        STU3_PACKAGE_DEP,
        R4_PACKAGE_DEP,
    ]
    srcs = [_get_zip_for_pkg(pkg) for pkg in all_fhir_pkgs]

    native.genrule(
        name = name + "_proto_files",
        outs = outs,
        srcs = srcs,
        tools = [PROTO_GENERATOR],
        cmd = cmd,
        tags = MANUAL_TAGS,
    )

def gen_fhir_definitions_and_protos(
        name,
        package_info,
        extensions = [],
        profiles = [],
        package_deps = [],
        additional_proto_imports = [],
        separate_extensions = False):
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
      package_deps: Any package_deps these definitions depend on.
                    Core fhir definitions are automatically included.
      additional_proto_imports: Additional proto files the generated profiles should import
                                FHIR datatypes, annotations, and codes are included automatically.
      separate_extensions: If true, will produce two proto files, one for extensions
                                          and one for profiles.
    """

    extension_flags = " ".join([("--extensions $(location %s) " % extension) for extension in extensions])
    profile_flags = " ".join([("--profiles $(location %s) " % profile) for profile in profiles])

    struct_def_dep_zip_flags = " ".join([
        ("--struct_def_dep_zip $(location %s) " % _get_zip_for_pkg(dep))
        for dep in package_deps
    ])

    fhir_definition_srcs = ([
                                package_info,
                                _get_zip_for_pkg(STU3_PACKAGE_DEP),
                                _get_zip_for_pkg(R4_PACKAGE_DEP),
                            ] +
                            extensions +
                            profiles +
                            [_get_zip_for_pkg(dep) for dep in package_deps])

    json_outs = ["_genfiles_" + name + "_extensions.json", "_genfiles_" + name + ".json"]

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
                --name _genfiles_%s \
                --package_info $(location %s) \
                --stu3_struct_def_zip $(location %s) \
                --r4_struct_def_zip $(location %s) \
                %s %s %s""" % (
            PROFILE_GENERATOR,
            name,
            package_info,
            _get_zip_for_pkg(STU3_PACKAGE_DEP),
            _get_zip_for_pkg(R4_PACKAGE_DEP),
            struct_def_dep_zip_flags,
            extension_flags,
            profile_flags,
        )),
        tags = MANUAL_TAGS,
    )

    fhir_package(
        package_name = name,
        definitions = json_outs,
        package_info = package_info,
    )

    gen_fhir_protos(
        name = name,
        package = name,
        package_deps = package_deps,
        additional_proto_imports = additional_proto_imports,
        separate_extensions = separate_extensions,
    )

def _get_zip_for_pkg(pkg):
    return pkg + ".zip"
