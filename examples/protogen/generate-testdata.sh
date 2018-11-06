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

ROOT_PATH=../..
EXAMPLES=$(ls $ROOT_PATH/testdata/stu3/examples/*.prototxt | cut -d/ -f6 | sed s/.prototxt/.json/)
INPUT_PATH=$ROOT_PATH/spec/hl7.fhir.core/3.0.1/package
JSON_TO_PROTO=$ROOT_PATH/bazel-bin/java/JsonToProto
OUTPUT_PATH=.

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
bazel build //java:JsonToProto

# Parse all the examples, using base resource definitions (without profiles).
$JSON_TO_PROTO --output_directory $OUTPUT_PATH $(for in in $EXAMPLES; do echo $INPUT_PATH/$i; done)
