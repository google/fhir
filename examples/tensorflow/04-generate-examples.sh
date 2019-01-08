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
bazel build -c opt //py/google/fhir/seqex:all
BUNDLE_TO_SEQEX=$(pwd)/../../bazel-bin/py/google/fhir/seqex/bundle_to_seqex_main
VERSION_CONFIG=$(pwd)/../../proto/stu3/version_config.textproto

# Generate tf.SequenceExamples for train, test and validation.
for i in validation test train; do
  echo Generating examples for $i...
  $BUNDLE_TO_SEQEX --fhir_version_config=$VERSION_CONFIG \
    --bundle_path=${DIR}/bundles*.tfrecords \
    --label_path=${DIR}/labels/${i}-* \
    --output_path=${DIR}/seqex/${i} &
done
wait
