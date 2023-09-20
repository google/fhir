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

SRC_ROOT="proto/r4/core"
PROTO_GENERATOR=bazel-bin/java/ProtoGenerator
OUTPUT_PATH="proto/google/fhir/proto/r4/core"
# Replace this line with a location of an R4 NPM
FHIR_PACKAGE="Set npm location here!"
bazel build //java/com/google/fhir/protogen:ProtoGeneratorV2


if [ $? -ne 0 ]
then
 echo "Build Failed"
 exit 1;
fi

# generate datatypes.proto
$PROTO_GENERATOR \
  --input_package $FHIR_PACKAGE \
  --proto_package "google.fhir.r4.core" \
  --java_proto_package "com.google.fhir.r4.core" \
  --license_date "2019" \
  --add_search_parameters \
  --legacy_datatype_generation \
  --output_directory "${OUTPUT_PATH}"

unzip -qo "${OUTPUT_PATH}/output.zip" -d "${OUTPUT_PATH}"
rm "${OUTPUT_PATH}/output.zip"

# Some datatypes are manually generated.
# These include:
# * Proto for Reference, which allows more structure than FHIR spec provides.
# * Extension, which has a field order discrepancy between spec and test data.
# TODO(b/244184211): generate Extension proto with custom ordering.

if [ $? -eq 0 ]
then
  echo -e "\n//End of auto-generated messages.\n" >> "${OUTPUT_PATH}/datatypes.proto"
  cat "$(dirname $0)/r4/datatypes_supplement.txt" >> "${OUTPUT_PATH}/datatypes.proto"
fi
