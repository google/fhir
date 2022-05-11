# Copyright 2022 Google LLC
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
        "@com_google_fhir//bazel/buildfiles:libzip_config.h",
    ],
    outs = [
        "config.h",
    ],
    cmd = "cp $< $@",
)

genrule(
    name = "copy_zipconf_h",
    srcs = [
        "@com_google_fhir//bazel/buildfiles:zipconf.h",
    ],
    outs = [
        "lib/zipconf.h",
    ],
    cmd = "cp $< $@",
)

genrule(
    name = "copy_zip_err_str_c",
    srcs = [
        "@com_google_fhir//bazel/buildfiles:zip_err_str.c",
    ],
    outs = [
        "zip_err_str.c",
    ],
    cmd = "cp $< $@",
)

cc_library(
    name = "libzip",
    srcs = glob(
        [
            "lib/*.c",
            "lib/*.h",
        ],
        exclude = [
            "lib/*win32*",
            "lib/zip_random_uwp.c",
            "lib/*crypto*",
            "lib/*aes*",
            "lib/*bzip2*",
            "lib/*xz*",
        ],
    ) + [
        "config.h",
        "zip_err_str.c",
        "lib/zipconf.h",
    ],
    hdrs = [
        "lib/zip.h",
        "lib/zipconf.h",
    ],
    copts = [
        "-DHAVE_CONFIG_H",
    ],
    includes = ["lib"],
    deps = [
        "@zlib",
    ],
)
