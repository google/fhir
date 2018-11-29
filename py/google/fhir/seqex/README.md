This directory contains apache-beam code for generating tensorflow sequence
example from fhir bundle and labels. The input and output format
are in TFRecord.

An example run (we recommend you to run this from an virtual env) using
apache-beam's direct runner.

*  The input bundle is same as testdata/stu3/labels/bundle_1.pbtxt but in TFRecord format.
*  The input label is the output of the example in fhir/py/labels/README

```
% pip install -r bazel/requirements.txt
% bazel build //...
% ./bazel-bin/py/google/fhir/seqex/bundle_to_seqex_main  --fhir_version_config=proto/stu3/version_config.textproto --bundle_path=testdata/stu3/labels/test_bundle.tfrecord-00000-of-00001 --label_path=testdata/stu3/labels/test_label.tfrecord-00000-of-00001 --output_path=/tmp/seqex_output.tfrecord
```
