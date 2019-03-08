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
    echo 'Usage: ./sshtunnel.sh {path to your env file}'
    exit 1
fi

source $1

export MASTERNODE=${clustername}"-m"
export PORT=1082
gcloud compute ssh ${MASTERNODE} \
    --project=${PROJECT} --zone=${ZONE}  -- \
    -D ${PORT} -N
