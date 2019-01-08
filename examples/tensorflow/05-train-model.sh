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
bazel build -c opt //py/google/fhir/models:all
TRAIN_MODEL=$(pwd)/../../bazel-bin/py/google/fhir/models/run_locally

$TRAIN_MODEL \
  --num_train_steps=100 \
  --num_eval_steps=100 \
  --input_dir="$1/seqex" \
  --hparams_override="sequence_bucket_sizes=[128,128,128,128],batch_size=4" \
  --output_dir="$1/linear_model.$(date +%Y%m%d)/train" \
  --alsologtostderr
