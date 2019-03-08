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

mkdir -p $1
cd $1

# build synthea according to https://github.com/synthetichealth/synthea
echo Generating test data using Synthea. This script will build synthea only one time.
echo If you run this script second time and if synthea directory exist it will not build the synthea again.
echo If you want to rebuild synthea from scratch delete the synthea directory from $1
if [ ! -d "synthea" ]; then
    git clone https://github.com/synthetichealth/synthea.git
    cd synthea
    ./gradlew build check test
fi

cd $1/synthea

if [ -d "./output/fhir" ]; then
    rm -fr ./output/fhir
fi

# generate 200 valid STU3 bundles in output/fhir/
./run_synthea Massachusetts -p 200

#Copy generated data to a bucket in Google cloud storage
gsutil -m cp ${1}/synthea/output/fhir/*.json gs://$bucketname/synthea/fhir
