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

OUTPUT_PATH=$INPUT_PATH

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

# Generate descriptors for the main FHIR types.
$PROTO_GENERATOR \
  $COMMON_FLAGS \
  --emit_descriptors \
  --output_directory $OUTPUT_PATH \
  $(for i in $PRIMITIVES $DATATYPES $METADATATYPES $RESOURCETYPES $PROFILES; do echo "$INPUT_PATH/${i,,}.profile.json"; done)

# Generate descriptors for FHIR extensions.
$PROTO_GENERATOR \
  $COMMON_FLAGS \
  --emit_descriptors \
  --output_directory $EXTENSION_PATH \
  $(for i in $EXTENSIONS; do echo $i; done)

# Generate descriptors for Google extensions.
$PROTO_GENERATOR \
  $NO_PACKAGE_FLAGS \
  --emit_descriptors \
  --package_info $GOOGLE_PACKAGE_INFO \
  --output_directory $GOOGLE_EXTENSION_PATH \
  $(for i in $GOOGLE_EXTENSIONS; do echo $i; done)


