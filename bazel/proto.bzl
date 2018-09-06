"""Proto related build rules for fhir.
"""

load("@protobuf_archive//:protobuf.bzl", "cc_proto_library")
load("@protobuf_archive//:protobuf.bzl", "py_proto_library")

def fhir_proto_library(proto_library_prefix, srcs = [], proto_deps = []):
    """Generates proto_library target, as well as {py,cc,java}_proto_library targets.

    Args:
      proto_library_prefix: Name prefix to be added to various proto libraries.
      srcs: Srcs for the proto library.
      proto_deps: Deps by the proto_library.
    """
    py_deps = []
    cc_deps = []
    for x in proto_deps:
        if x.endswith(":descriptor_proto"):
            py_deps.append(x[:-17] + ":protobuf_python")
            cc_deps.append(x[:-17] + ":cc_wkt_protos")
        elif x.endswith("_proto"):
            py_deps.append(x[:-6] + "_py_pb2")
            cc_deps.append(x[:-6] + "_cc_proto")

    py_proto_library(
        name = proto_library_prefix + "_py_pb2",
        srcs = srcs,
        deps = py_deps,
        default_runtime = "@protobuf_archive//:protobuf_python",
        protoc = "@protobuf_archive//:protoc",
    )

    cc_proto_library(
        name = proto_library_prefix + "_cc_proto",
        srcs = srcs,
        deps = cc_deps,
        default_runtime = "@protobuf_archive//:protobuf",
        protoc = "@protobuf_archive//:protoc",
    )

    native.proto_library(
        name = proto_library_prefix + "_proto",
        srcs = srcs,
        deps = proto_deps,
    )

    native.java_proto_library(
        name = proto_library_prefix + "_java_proto",
        deps = [
            ":" + proto_library_prefix + "_proto",
        ],
    )
