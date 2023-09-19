#!/bin/bash
# Copyright 2023 Google LLC
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

set -eu

SRC_ROOT="proto/r5/core"
PROTO_GENERATOR=bazel-bin/java/ProtoGenerator
OUTPUT_PATH="proto/google/fhir/proto/r5/core"
# Replace this line with a location of an R5 NPM
FHIR_PACKAGE="Set npm location here!"
bazel build //java/com/google/fhir/protogen:ProtoGeneratorV2


if [ $? -ne 0 ]
then
 echo "Build Failed"
 exit 1;
fi

$PROTO_GENERATOR \
  --input_package $FHIR_PACKAGE \
  --proto_package "google.fhir.r5.core" \
  --java_proto_package "com.google.fhir.r5.core" \
  --license_date "2023" \
  --fhir_version "r5" \
  --contained_resource_offset 5000 \
  --output_directory "${OUTPUT_PATH}"

unzip -qo "${OUTPUT_PATH}/output.zip" -d "${OUTPUT_PATH}"
rm "${OUTPUT_PATH}/output.zip"
