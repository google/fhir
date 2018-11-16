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

# Note: requires Cloud Bigquery SDK, see https://cloud.google.com/bigquery/quickstart-command-line
bq mk synthea

for i in $(basename -a -s.analytic.ndjson $1/*.analytic.ndjson); do
  echo "Uploading $i..."
  bq load --source_format=NEWLINE_DELIMITED_JSON --schema=$1/$i.schema.json synthea.$i $1/$i.analytic.ndjson
done
