#!/bin/bash
# Copyright 2020 Google LLC
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# In order to run this command, you must have protoc and the protoc-gen-go
# plugin installed in your PATH.
# More details on protoc-gen-go install: https://protobuf.dev/reference/go/go-generated/
# Install protoc on linux with sudo apt install -y protobuf-compiler.

# This should be run from the root FHIR repository, like so:
# ./go/generate_go_protos_default.sh
bazel run //go:generate_go_protos -- --repo-path=.
