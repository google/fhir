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
    echo "If you do want to use an a hive metastore with this deployment please
    pass in argument 'hivemeta'"
fi

machinetype="n1-standard-16"
imageversion=1.3
inittimeout="150m"
tags="dataproc"

if [[ "$1" == "hivemeta" ]] ; then
  gcloud dataproc clusters create $clustername \
    --master-machine-type=$machinetype \
    --scopes cloud-platform \
    --image-version $imageversion \
    --initialization-actions gs://dataproc-initialization-actions/cloud-sql-proxy/cloud-sql-proxy.sh,gs://$bucketname/scripts/datalab_fhir.sh,gs://$bucketname/scripts/init-script.sh \
    --initialization-action-timeout=$inittimeout \
    --properties hive:hive.metastore.warehouse.dir=gs://$hivebucketname/hadoop,spark:spark.sql.warehouse.dir=gs://$hivebucketname/hadoop,core:fs.gs.path.encoding=uri-path \
    --metadata "hive-metastore-instance=$PROJECT:$REGION:$hivedbname" \
    --tags=$tags \
    --region=$REGION \
    --zone=$ZONE

else
  gcloud dataproc clusters create $clustername \
    --master-machine-type=$machinetype \
    --scopes cloud-platform \
    --image-version $imageversion \
    --initialization-actions gs://$bucketname/scripts/datalab_fhir.sh,gs://$bucketname/scripts/init-script.sh \
    --initialization-action-timeout=$inittimeout \
    --tags=$tags \
    --region=$REGION \
    --zone=$ZONE
fi
