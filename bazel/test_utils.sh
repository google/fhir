#!/bin/bash -ex
# Copyright 2020 Google LLC
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

function go_modules_tests {
    mkdir -p /tmp/go/src/github.com/google/fhir
    export GOPATH=/tmp/go
    export GO111MODULE=on
    cp -r ./ /tmp/go/src/github.com/google/fhir
    pushd /tmp/go/src/github.com/google/fhir/go
    go mod download  # Download module dependencies
    go build ./...  # Build everything in go/
    go test ./...  # Test everything in go/
    popd
}

# This runs all builds and tests (e.g. all of Bazel tests, Go module tests,
# etc).
function run_all_tests {
  bazel test --test_output=errors //...
  go_modules_tests
}
