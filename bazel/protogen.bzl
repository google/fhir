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

STU3_STRUCTURE_DEFINITION_DEP = "//testdata/stu3:fhir"
PROTO_GENERATOR = "//java:ProtoGenerator"
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
        additional_proto_imports = [],
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

def _get_zip_for_pkg(pkg):
    return pkg + "_structure_definitions_zip"

def _get_package_info_for_pkg(pkg):
    return pkg + "_package_info_prototxt"
