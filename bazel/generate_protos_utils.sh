#!/bin/bash
# Copyright 2018 Google LLC
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

function try_build() {
  bazel build "$1"
  if [[ $? -ne 0 ]]
  then
   echo Build Failed: "$1"
   exit 1;
  fi
}

function copy_to_src_if_present() {
  src=bazel-genfiles/$dir/_genfiles_$1

  dst=$dir/$1
  if [ -e $src ] && [ $(wc -l < $src) -gt 20 ]
  then
    cp $src $dst
  fi
}
