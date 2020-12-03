# Generated Go Protos

This directory contains generated Go code for the protocol buffers in `proto/`
at the root of this repository. This code is only used when building the go
FHIR libraries using the native `go` build tool (and is _not_ used with when
building with bazel
). This allows for `go mod` and `go get` to work correctly.

If a protocol buffer is changed, the Go protocol buffers should be regenerated.

## Regenerate Go Protos

To regenerate the Go protocol buffers run the following:

```sh
./go/generate_go_protos_default.sh
```
