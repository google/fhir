#!/bin/bash
# Copyright 2021 Google LLC
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

# Script intended to be used on gen_fhir_definitions_and_protos rules to
# generate only JSON FHIR IG resources, but *NOT* protos.
# The only argument to this should be a gen_fhir_definitions_and_protos.
#
# e.g., for rule in //foo/bar/BUILD,
# gen_fhir_definitions_and_protos(
#   name = "quux",
#   ...
# ),
#
# This will generate 3 files in the foo/bar/ source directory:
# * foo/bar/quux_extensions.json  (bundle of extension StructureDefinitions)
# * foo/bar/quux_profiles.json    (bundle of profile StructureDefinitions)
# * foo/bar/quux_terminologies.json    (bundle of CodeSystems and ValueSets)

target=$1
tokens=(${target/:/ })
target_dir=${tokens[0]}
if [[ ${target_dir:0:2} != "//" ]]; then
  echo "Taget must be absolute and begin with //"
  echo "e.g., //path/to/my:target"
  exit 1
fi
dir=${target_dir#"//"}
label=${tokens[1]}

source $(dirname "$BASH_SOURCE")/generate_protos_utils.sh

# Build structure definitions, and then copy them into source
try_build "${target}_definitions"
copy_to_src_if_present ${label}.json
copy_to_src_if_present ${label}_extensions.json
copy_to_src_if_present ${label}_terminologies.json
