# Generated Go Protos

This directory contains generated Go code for the protocol buffers in `proto/`
at the root of this repository. This code is only used when building the go
FHIR libraries using the native `go` build tool (and is _not_ used with when
building with bazel
). This allows for `go mod` and `go get` to work correctly.

If a protocol buffer is changed, the Go protocol buffers should be regenerated.

## Regenerate Go Protos

In order to generate the Go protos, you must have protoc and the protoc-gen-go
plugin installed in your PATH.
More details on protoc-gen-go install: https://protobuf.dev/reference/go/go-generated/
Install protoc on linux with `sudo apt install -y protobuf-compiler`.

Then run the following:

```sh
./go/generate_go_protos_default.sh
```
