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

echo "This script is meant for building fhir code dependencies, unless there is a reason to rebuild this code you could leverage what you have built"
echo "This script sets up the an init script in the init-scripts folder with the build code in the gcs bucket"
echo "Running this script will delete the contents of gs://$bucketname/fhir, the process to rebuild the code is long, are you sure you want to proceed (y/n)?"
read yesno
if [[ $yesno != "y" ]] ; then
  echo "Exiting"
  exit
fi

# Start wih a clean gcs folder destined for the fhir dependencies

gsutil -m rm -r gs://$bucketname/fhir

cd $(pwd)/../../
pip install -r bazel/requirements.txt
bazel build -c opt //...

if [[ $? -eq 0 ]] ; then
    echo 'Successful bazel build'
else
    echo 'Unsuccessful build, please check the requirements'
    exit
fi

# Post processing steps
mkdir out-genfiles
cp -fr bazel-genfiles/py ./out-genfiles/.
cp -fr bazel-genfiles/proto ./out-genfiles/.

mkdir out-bin
cp -fr bazel-bin/py ./out-bin/.
cp -fr bazel-bin/proto ./out-bin/.

#Importing google_extensions_pb2
touch ./out-genfiles/proto/__init__.py
touch ./out-genfiles/proto/stu3/__init__.py

#Importing py.google.fhir.seqex

touch ./py/__init__.py
touch ./py/google/__init__.py

cat <<EOT >> ./py/google/fhir/seqex/bundle_to_seqex_converter.py
def __bootstrap__():
  global __bootstrap__, __loader__, __file__
  import sys, pkg_resources, imp
  __file__ = pkg_resources.resource_filename(__name__,'bundle_to_seqex_converter.so')
  __loader__ = None; del __bootstrap__, __loader__
  imp.load_dynamic(__name__,__file__)
__bootstrap__()
EOT

cp ./out-bin/py/google/fhir/seqex/*.so ./py/google/fhir/seqex/.

# Finally copying over the files to the desginated gcs bucket
gsutil -m cp -r proto gs://$bucketname/fhir/
gsutil -m cp -r py gs://$bucketname/fhir/
gsutil -m cp -r out-genfiles gs://$bucketname/fhir/

# Change out of the directory
cd examples/gcp_datalab
