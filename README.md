# FHIR protocol buffers

This repository contains a Google implementation of protocol buffers for
FHIR. This is not an officially supported Google product.

To build this repository, install [bazel](https://bazel.build/), and then run

```
$ bazel test //...
```

For some ideas of how to use this code, see the [examples](./examples/):

* [Querying with BigQuery](./examples/bigquery/README.md)
* [Converting FHIR json to proto](./java/src/main/java/com/google/fhir/examples/FhirToProtoMain.java)

This is still an early version, subject to change. For some background, please
see our blog post. Support for c++, go, and python is coming soon.
