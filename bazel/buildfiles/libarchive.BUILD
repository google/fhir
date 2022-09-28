# Copyright 2021 Google LLC
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     https://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY

licenses(["notice"])  # Apache v2.0

package(
    default_visibility = ["//visibility:public"],
)

genrule(
    name = "copy_config_h",
    srcs = [
        "@com_google_fhir//bazel/buildfiles:libarchive_config.h",
    ],
    outs = [
        "config.h",
    ],
    cmd = "cp $< $(@D)/config.h",
)

cc_library(
    name = "libarchive",
    srcs = ["config.h"] + glob(
        ["libarchive/*.c"],
    ),
    hdrs = ["config.h"] + glob(
        ["libarchive/*.h"],
    ),
    copts = [
        "-DHAVE_CONFIG_H=1",
        "-D_GNU_SOURCE=1",
        "-Wno-unused",
    ],
    deps = [
        "@zlib",
    ],
)
