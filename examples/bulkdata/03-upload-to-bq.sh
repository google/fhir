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

# Note: requires Cloud Bigquery SDK, see https://cloud.google.com/bigquery/quickstart-command-line
if [[ $# -eq 0 ]] ; then
    echo 'Missing argument: directory with protobuf input files'
    exit 1
fi

bq mk bulkdata

for i in $1/*.ndjson.prototxt.gz; do
  echo "uploading $i"
  echo "bq load --source_format=NEWLINE_DELIMITED_JSON --schema=$1/$(basename $i .gz).schema.json bulkdata.$(basename $i .ndjson.prototxt.gz) $i"1
done
