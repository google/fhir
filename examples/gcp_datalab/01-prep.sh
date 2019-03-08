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
    echo "This deployment will not use an Hive metastore"
    echo "If you do want to use an a hive metastore with this deployment please pass in argument 'hivemeta'"
fi

#Create persistent hive metadata store

#Reference link
gsutil mb gs://$bucketname

#Update the init script with the bucketname
sed -i -e "s/<REPLACEME_BUCKETNAME>/$bucketname/g" ./init-scripts/datalab_fhir.sh

gsutil cp ./init-scripts/* gs://$bucketname/scripts/
gsutil ls gs://$bucketname/scripts/

#Create the sql database if you want to leverage an sql hive metasstore
if [[ "$1" == "hivemeta" ]] ; then
    gsutil ls gs://$hivebucketname
    if [ $? -eq 0 ]; then
      echo "Bucket $hivebucketname exists and the dataproc cluster will leverage this existing bucket"
    else
      gsutil mb gs://$hivebucketname
    fi
    gcloud sql instances create $hivedbname --database-version="MYSQL_5_7" --activation-policy=ALWAYS --gce-zone $ZONE
fi

