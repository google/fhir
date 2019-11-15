"""Proto related build rules for fhir.
"""

load("@rules_proto//proto:defs.bzl", "proto_library")
load("@rules_cc//cc:defs.bzl", "cc_proto_library")
load("@com_google_protobuf//:protobuf.bzl", "py_proto_library")

WELL_KNOWN_PROTOS = ["descriptor_proto", "any_proto"]

def fhir_proto_library(proto_library_prefix, srcs = [], proto_deps = [], **kwargs):
    """Generates proto_library target, as well as {py,cc,java}_proto_library targets.

    Args:
      proto_library_prefix: Name prefix to be added to various proto libraries.
      srcs: Srcs for the proto library.
      proto_deps: Deps by the proto_library.
      **kwargs: varargs. Passed through to proto rules.
    """
    py_deps = []
    cc_deps = []
    has_well_known_dep = False
    for x in proto_deps:
        tokens = x.split(":")
        if len(tokens) == 2 and tokens[1] in WELL_KNOWN_PROTOS:
            if not has_well_known_dep:
                py_deps.append(tokens[0] + ":protobuf_python")
                cc_deps.append(tokens[0] + ":cc_wkt_protos")
                has_well_known_dep = True
        elif x.endswith("_proto"):
            py_deps.append(x[:-6] + "_py_pb2")
            cc_deps.append(x[:-6] + "_cc_proto")

    proto_library(
        name = proto_library_prefix + "_proto",
        srcs = srcs,
        deps = proto_deps,
        **kwargs
    )

    py_proto_library(
        name = proto_library_prefix + "_py_pb2",
        srcs = srcs,
        deps = py_deps,
        default_runtime = "@com_google_protobuf//:protobuf_python",
        protoc = "@com_google_protobuf//:protoc",
        **kwargs
    )

    cc_proto_library(
        name = proto_library_prefix + "_cc_proto",
        deps = [proto_library_prefix + "_proto"],
    )

    native.java_proto_library(
        name = proto_library_prefix + "_java_proto",
        deps = [
            ":" + proto_library_prefix + "_proto",
        ],
        **kwargs
    )
