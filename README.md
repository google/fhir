# FHIR protocol buffers

This repository contains a Google implementation of protocol buffers for
FHIR. This is not an officially supported Google product.

To build this repository, install [bazel](https://bazel.build/), and necessary
dependencies (we recommend you to run this from an virtual env):

```
$ pip install -r bazel/requirements.txt
```

Then run:

```
$ bazel test //...
```

For some ideas of how to use this code, see the [examples](./examples/):

* [Querying with BigQuery](./examples/bigquery/README.md)
* [Converting FHIR json to proto](./java/src/main/java/com/google/fhir/examples/JsonToProtoMain.java)

This is still an early version and subject to change. For some background, please
see our [blog post](https://research.googleblog.com/2018/03/making-healthcare-data-work-better-with.html). Support for C++, Go, and Python is coming soon. You can post questions and feedback in this [web forum](https://groups.google.com/forum/#!forum/fhir-protobuf).
