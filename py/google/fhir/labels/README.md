This directory contains example of labels generation code (length of stay
range at 24 hours label) from fhir bundle. The input and output format
are in TFRecord.

An example run (we recommend you to run this from an virtual env) using
apache-beam's direct runner.
The input is same as testdata/stu3/labels/bundle_1.pbtxt but in TFRecord format.

```
% pip install -r bazel/requirements.txt
% bazel build //...
% ./bazel-bin/py/google/fhir/labels/bundle_to_label --input_path=testdata/stu3/labels/test_bundle.tfrecord-00000-of-00001 --output_path=/tmp/output.tfrecord
```
