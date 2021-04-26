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

readonly PIP_VERSION='~=21.0.0'
readonly SETUPTOOLS_VERSION='~=56.0.0'

function run_bazel_in_venv {
  python3 -m venv venv
  source venv/bin/activate
  pip install pip"${PIP_VERSION}"
  pip install setuptools"${SETUPTOOLS_VERSION}"
  pip install wheel

  # Run bazel test command. We run bazel test //... first so that the tests
  # can start before all targets have been built. In particular, it takes
  # around 2 minutes to build resources proto package for java - and it would
  # block all cc and py tests if we run bazel build //... first.
  bazel test --test_output=errors //...
  bazel build //...

  # Deactivate venv
  deactivate
}

function go_modules_tests {
    mkdir -p /tmp/go/src/github.com/google/fhir
    export GOPATH=/tmp/go
    export GO111MODULE=on
    cp -r ./ /tmp/go/src/github.com/google/fhir
    pushd /tmp/go/src/github.com/google/fhir/go
    go1.15.5 mod download  # Download module dependencies
    go1.15.5 build ./...  # Build everything in go/
    go1.15.5 test ./...  # Test everything in go/
    popd
}

# This runs all builds and tests (e.g. all of Bazel tests, Go module tests,
# etc).
function run_all_tests {
  run_bazel_in_venv
  go_modules_tests
}
