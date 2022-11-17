#!/bin/bash
# Copyright 2019 Google LLC
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

ROOT_PATH=../..
SRC_ROOT="proto/r4/core"
PROTO_GENERATOR=${ROOT_PATH}/bazel-bin/java/ProtoGenerator

OUTPUT_PATH="$(dirname $0)/../../proto/r4/core"

bazel build //spec/fhir_r4_definitions.zip
FHIR_PACKAGE="${ROOT_PATH}/bazel-genfiles/spec/fhir_r4_definitions.zip"

COMMON_FLAGS=" \
  --sort \
  --r4_core_dep $FHIR_PACKAGE \
  --input_package $FHIR_PACKAGE"

# Build the binary.
bazel build //java/com/google/fhir/protogen:ProtoGenerator

if [ $? -ne 0 ]
then
 echo "Build Failed"
 exit 1;
fi

# generate datatypes.proto
$PROTO_GENERATOR \
  $COMMON_FLAGS \
  --output_directory "${OUTPUT_PATH}" \
  --directory_in_source "${SRC_ROOT}" \
  --output_name datatypes \
  --filter datatype \
  --exclude elementdefinition-de \
  --exclude Reference \
  --exclude Extension \
  --exclude Element
unzip -qo "${OUTPUT_PATH}/datatypes.zip" -d "${OUTPUT_PATH}"
rm "${OUTPUT_PATH}/datatypes.zip"

# Some datatypes are manually generated.
# These include:
# * FHIR-defined valueset codes
# * Proto for Reference, which allows more structure than FHIR spec provides.
# * Extension, which has a field order discrepancy between spec and test data.
# TODO(b/244184211): generate Extension proto with custom ordering.
if [ $? -eq 0 ]
then
  echo -e "\n//End of auto-generated messages.\n" >> "${OUTPUT_PATH}/datatypes.proto"
  cat "$(dirname $0)/r4/datatypes_supplement.txt" >> "${OUTPUT_PATH}/datatypes.proto"
fi

# generate resource protos
$PROTO_GENERATOR \
  $COMMON_FLAGS \
  --output_directory "${OUTPUT_PATH}/resources" \
  --directory_in_source "${SRC_ROOT}/resources" \
  --filter resource \
  --output_name resources

unzip -qo "${OUTPUT_PATH}/resources/resources.zip" -d "${OUTPUT_PATH}/resources"
rm "${OUTPUT_PATH}/resources/resources.zip"

# generate profiles.proto
# exclude familymemberhistory-genetic due to
# https://gforge.hl7.org/gf/project/fhir/tracker/?action=TrackerItemEdit&tracker_id=677&tracker_item_id=19239
$PROTO_GENERATOR \
  $COMMON_FLAGS \
  --output_directory "${OUTPUT_PATH}/profiles" \
  --directory_in_source "${SRC_ROOT}/profiles" \
  --filter profile \
  --exclude familymemberhistory-genetic \
  --output_name profiles

unzip -qo "${OUTPUT_PATH}/profiles/profiles.zip" -d "${OUTPUT_PATH}/profiles"
rm "${OUTPUT_PATH}/profiles/profiles.zip"

# generate extensions
$PROTO_GENERATOR \
  $COMMON_FLAGS \
  --output_name extensions \
  --output_directory ${OUTPUT_PATH} \
  --directory_in_source "${SRC_ROOT}" \
  --filter extension

unzip -qo "${OUTPUT_PATH}/extensions.zip" -d "${OUTPUT_PATH}"
rm "${OUTPUT_PATH}/extensions.zip"
mv "${OUTPUT_PATH}/extensions_extensions.proto" \
   "${OUTPUT_PATH}/extensions.proto"
