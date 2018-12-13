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

"""Rules for generating Protos from Profiles and StructureDefinitions"""

STU3_STRUCTURE_DEFINITION_DEP = "//spec:fhir_stu3_package"
PROTO_GENERATOR = "//java:ProtoGenerator"
PROFILE_GENERATOR = "//java:ProfileGenerator"
FHIR_PROTO_ROOT = "proto/stu3"

def zip_file(name, srcs = []):
    native.genrule(
        name = name,
        srcs = srcs,
        outs = [name],
        cmd = "zip --quiet -j $@ $(SRCS)",
    )

def structure_definition_package(package_name, structure_definitions_zip, package_info):
    if _get_zip_for_pkg(package_name) != structure_definitions_zip:
        native.alias(
            name = _get_zip_for_pkg(package_name),
            actual = structure_definitions_zip,
        )
    native.alias(
        name = _get_package_info_for_pkg(package_name),
        actual = package_info,
    )

def gen_fhir_protos(
        name,
        package,
        package_deps = [],
        additional_proto_imports = None,
        separate_extensions = False,
        add_apache_license = False):
    """Generates a proto file from a structure_definition_package

    These rules should be run by the generate_protos.sh script, which will generate the
    protos and move them into the source directory.
    e.g., for a gen_fhir_protos rule in foo/bar with name = quux,
    bazel/generate_protos.sh foo/bar:quux

    Args:
      name: The name for the generated proto file (without .proto)
      package: The structure_definition_package to generate protos for.
      package_deps: Any package_deps these definitions depend on.
                    Core fhir structure definitions are automatically included.
      additional_proto_imports: Additional proto files the generated protos should import
                                FHIR datatypes, annotations, and codes are included automatically.
      separate_extensions: If true, will produce two proto files, one for extensions
                                          and one for profiles.
      add_apache_license: Whether or not to include the apache license
    """

    all_struct_def_pkgs = package_deps + [STU3_STRUCTURE_DEFINITION_DEP, package]
    struct_def_dep_flags = " ".join([
        "--struct_def_dep_pkg \"$(location %s)|$(location %s)\"" %
        (_get_zip_for_pkg(dep), _get_package_info_for_pkg(dep))
        for dep in all_struct_def_pkgs
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
            --output_directory $(@D) \
            --package_info $(location %s) \
            --fhir_proto_root %s \
            --output_name _genfiles_%s \
            --input_zip $(location %s) \
            """ % (
        PROTO_GENERATOR,
        _get_package_info_for_pkg(package),
        FHIR_PROTO_ROOT,
        name,
        _get_zip_for_pkg(package),
    )

    cmd += additional_proto_imports_flags + " " + struct_def_dep_flags

    if add_apache_license:
        cmd += " --add_apache_license "

    if separate_extensions:
        outs = ["_genfiles_" + name + ".proto", "_genfiles_" + name + "_extensions.proto"]
        cmd += " --separate_extensions "
    else:
        outs = ["_genfiles_" + name + ".proto"]

    srcs = ([_get_zip_for_pkg(pkg) for pkg in all_struct_def_pkgs] +
            [_get_package_info_for_pkg(pkg) for pkg in all_struct_def_pkgs])

    native.genrule(
        name = name + "_proto_files",
        outs = outs,
        srcs = srcs,
        tools = [PROTO_GENERATOR],
        cmd = cmd,
    )

def gen_fhir_definitions_and_protos(
        name,
        package_info,
        extensions = [],
        profiles = [],
        package_deps = [],
        additional_proto_imports = [],
        separate_extensions = False,
        add_apache_license = False):
    """Generates structure definitions and protos based on Extensions and Profiles protos.

    These rules should be run by bazel/generate_protos.sh, which will generate the
    profiles and protos and move them into the source directory.
    e.g., bazel/generate_protos.sh foo/bar:quux

    This also exports the package_info and a zip of structure definitions, so that this target
    can be used as a dependency of other gen_fhir_definitions_and_protos.

    Args:
      name: name prefix for all generated rules
      package_info: Metadata shared by all generated Structure Definitions.
      extensions: List of Extensions prototxt files
      profiles: List of Profiles prototxt files.
      package_deps: Any package_deps these definitions depend on.
                    Core fhir structure definitions are automatically included.
      additional_proto_imports: Additional proto files the generated profiles should import
                                FHIR datatypes, annotations, and codes are included automatically.
      separate_extensions: If true, will produce two proto files, one for extensions
                                          and one for profiles.
      add_apache_license: Whether or not to include the apache license
    """

    extension_flags = " ".join([("--extensions $(location %s) " % extension) for extension in extensions])
    profile_flags = " ".join([("--profiles $(location %s) " % profile) for profile in profiles])

    all_struct_def_deps = [STU3_STRUCTURE_DEFINITION_DEP] + package_deps
    struct_def_dep_zip_flags = " ".join([
        ("--struct_def_dep_zip $(location %s) " % _get_zip_for_pkg(dep))
        for dep in all_struct_def_deps
    ])

    structure_definition_srcs = ([package_info] +
                                 extensions +
                                 profiles +
                                 [_get_zip_for_pkg(dep) for dep in all_struct_def_deps])

    native.genrule(
        name = name + "_structure_definitions",
        outs = ["_genfiles_" + name + "_extensions.json", "_genfiles_" + name + ".json"],
        srcs = structure_definition_srcs,
        tools = [
            PROFILE_GENERATOR,
        ],
        cmd = ("""
            $(location %s) \
                --output_directory $(@D) \
                --name _genfiles_%s \
                --package_info $(location %s) \
                %s %s %s""" % (
            PROFILE_GENERATOR,
            name,
            package_info,
            struct_def_dep_zip_flags,
            extension_flags,
            profile_flags,
        )),
    )

    zip_file(
        name = name + "_structure_definitions.zip",
        srcs = [name + "_extensions.json", name + ".json"],
    )

    structure_definition_package(
        package_name = name,
        structure_definitions_zip = name + "_structure_definitions.zip",
        package_info = package_info,
    )

    gen_fhir_protos(
        name = name,
        package = name,
        package_deps = package_deps,
        additional_proto_imports = additional_proto_imports,
        separate_extensions = separate_extensions,
        add_apache_license = add_apache_license,
    )

def _get_zip_for_pkg(pkg):
    return pkg + "_structure_definitions_zip"

def _get_package_info_for_pkg(pkg):
    return pkg + "_package_info_prototxt"
