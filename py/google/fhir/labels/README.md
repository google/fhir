This directory contains example of labels generation code (length of stay
range at 24 hours label) from fhir bundle. The input and output format
are in TFRecord.

An example run (we recommend you to run this from an virtual env) using
apache-beam's direct runner.
The input is same as testdata/stu3/labels/bundle_1.pbtxt but in TFRecord format.

```shell
pip install -r bazel/requirements.txt
bazel build //...
./bazel-bin/py/google/fhir/labels/bundle_to_label --input_path=testdata/stu3/labels/test_bundle.tfrecord-00000-of-00001 --output_path=/tmp/output.tfrecord
```

The same example run using Google Cloud Dataflow.

```shell
# Set up a Google Cloud project and enable related APIs as in https://beam.apache.org/documentation/runners/dataflow.
# Install and set up gcloud cli tool as in https://cloud.google.com/sdk/gcloud/.
# Login gcloud tool.
gcloud auth login
# Set up default project.
gcloud config set project ${YOUR_PROJECT_ID?}
# Set up application default credentails.
gcloud auth application-default login
# Set up dependencies; assuming at the project root directory.
bazel build //...
# Make sure the needed packages can be recognized.
touch bazel-genfiles/proto/__init__.py bazel-genfiles/proto/stu3/__init__.py
touch py/__init__.py py/google/__init__.py py/google/fhir/__init__.py
# Copy the setup script to the top level for dependency management.
cp py/google/fhir/labels/setup_py.txt setup.py
# Launch beam pipeline on Google Cloud Dataflow.
./bazel-bin/py/google/fhir/labels/bundle_to_label \
    --input_path=gs://${YOUR_BUCKET?}/input/test_bundle.tfrecord-00000-of-00001 \
    --output_path=gs://${YOUR_BUCKET?}/output \
    --runner=DataflowRunner \
    --project_id=${YOUR_PROJECT_ID?} \
    --gcs_temp_location=gs://${YOUR_BUCKET?}/tmp \
    --setup_file=./setup.py
```
