# !/bin/bash
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

if [[ $# -eq 0 ]] ; then
    echo 'Missing argument: scratch directory'
    exit 1
fi
# Remove any trailing "/", which confuses beam's file matcher.
DIR=$(echo $1 | sed "s/\/$//")
bazel build -c opt //py/google/fhir/labels:all
BUNDLE_TO_LABELS=$(pwd)/../../bazel-bin/py/google/fhir/labels/bundle_to_label

# We generate labels for train, validation, and test separately, using an ad-hoc
# split method for now.
$BUNDLE_TO_LABELS  --for_synthea \
  --input_path=${DIR}/bundles-00000-of-00010.tfrecords \
  --output_path=${DIR}/labels/test &
$BUNDLE_TO_LABELS  --for_synthea \
  --input_path=${DIR}/bundles-00001-of-00010.tfrecords \
  --output_path=${DIR}/labels/validation &
$BUNDLE_TO_LABELS  --for_synthea \
  --input_path=${DIR}/bundles-0000[23456789]-of-00010.tfrecords \
  --output_path=${DIR}/labels/train &
wait
