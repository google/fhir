#!/bin/bash
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

OUTPUT_PATH=.
MANUAL_ADDITIONS_ROOT=.

while getopts ":io:" opt; do
  case ${opt} in
    i )
      INPUT_PATH=$OPTARG
      ;;
    o )
      OUTPUT_PATH=$OPTARG
      ;;
    \? )
      echo "Invalid option: $OPTARG" 1>&2
      ;;
    : )
      echo "Invalid option: $OPTARG requires an argument" 1>&2
      ;;
  esac
done
shift $((OPTIND -1))

# Build the binary.
bazel build //java:ProtoGenerator

if [ $? -ne 0 ]
then
 echo "Build Failed"
 exit 1;
fi

source "common.sh"

# generate datatypes.proto
$PROTO_GENERATOR \
  $COMMON_FLAGS \
  --emit_proto \
  --output_directory $OUTPUT_PATH \
  --output_filename datatypes.proto \
  $(for i in $PRIMITIVES $DATATYPES; do echo "$INPUT_PATH/${i,,}.profile.json"; done)
# Some datatypes are manually generated.
# These include:
# * FHIR-defined valueset codes
# * Proto for Reference, which allows more structure than FHIR spec provides.
# * Extension, which has a field order discrepancy between spec and test data.
# TODO(nickgeorge): generate Extension proto with custom ordering.
# TODO(sundberg): generate codes.proto
if [ $? -eq 0 ]
then
  echo -e "\n//End of auto-generated messages.\n" >> $OUTPUT_PATH/datatypes.proto
  cat $MANUAL_ADDITIONS_ROOT/extension_proto.txt >> $OUTPUT_PATH/datatypes.proto
  cat $MANUAL_ADDITIONS_ROOT/reference_proto.txt >> $OUTPUT_PATH/datatypes.proto
  cat $MANUAL_ADDITIONS_ROOT/codes_proto.txt >> $OUTPUT_PATH/datatypes.proto
fi

# generate metadatatypes.proto
$PROTO_GENERATOR \
  $COMMON_FLAGS \
  --emit_proto \
  --output_directory $OUTPUT_PATH \
  --output_filename metadatatypes.proto \
  $(for i in $METADATATYPES; do echo "$INPUT_PATH/${i,,}.profile.json"; done)

# generate resources.proto
$PROTO_GENERATOR \
  $COMMON_FLAGS \
  --emit_proto \
  --output_directory $OUTPUT_PATH \
  --include_contained_resource \
  --include_metadatatypes \
  --output_filename resources.proto \
  $(for i in $RESOURCETYPES; do echo "$INPUT_PATH/${i,,}.profile.json"; done)

# generate profiles.proto
$PROTO_GENERATOR \
  $COMMON_FLAGS \
  --emit_proto \
  --output_directory $OUTPUT_PATH \
  --include_extensions\
  --include_resources \
  --include_metadatatypes \
  --output_filename profiles.proto \
  $(for i in $PROFILES; do echo "$INPUT_PATH/${i,,}.profile.json"; done)

# generate extensions
$PROTO_GENERATOR \
  $COMMON_FLAGS \
  --emit_proto \
  --output_directory $OUTPUT_PATH \
  --output_filename extensions.proto \
  $(for i in $EXTENSIONS; do echo "$i"; done)

# generate google-specific extensions
$PROTO_GENERATOR \
  $NO_PACKAGE_FLAGS \
  --emit_proto \
  --proto_package google.fhir.stu3.google \
  --java_proto_package com.google.fhir.stu3.google \
  --output_directory $OUTPUT_PATH \
  --output_filename google_extensions.proto \
  $(for i in $GOOGLE_EXTENSIONS; do echo "$i"; done)
