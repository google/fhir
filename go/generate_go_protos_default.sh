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

# In order to run this script, you must have Go installed on your machine. This
# script will attempt to setup a GOPATH and GOBIN in your $HOME directory, if
# those environment variables are not already set. It will then attempt to
# install the dependencies needed to run (protoc-gen-go and protoc, needed to
# compile the protos).

# This should be run from the root FHIR repository, like so:
# ./go/generate_go_protos_default.sh

if [ "${GOPATH}" = "" ]; then
  echo "Setting temp GOPATH environment variable, creating GOPATH directory \$HOME/go"
  GOPATH="${HOME}/go"
  mkdir -p $GOPATH
fi

if [ "${GOBIN}" = "" ]; then
  echo "Setting temp GOBIN environment variable, creating GOBIN directory in \$GOPATH/bin"
  GOBIN="${GOPATH}/bin"
  mkdir -p $GOBIN
fi

if ! [ -x "$(command -v protoc)" ]; then
  echo 'protoc missing, please install the protobuf compiler. On linux: sudo apt install -y protobuf-compiler'
fi

if ! [ -x "$(command -v protoc-gen-go)" ]; then
  go install google.golang.org/protobuf/cmd/protoc-gen-go@v1.29
  PATH=$PATH:$GOBIN
fi

bazel run //go:generate_go_protos -- --repo-path=.
