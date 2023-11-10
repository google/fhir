// Copyright 2020 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <fstream>
#include <iostream>

#include "google/protobuf/descriptor.h"
#include "absl/strings/str_cat.h"
#include "google/fhir/footprint/proto_footprint.h"
#include "proto/google/fhir/proto/r4/core/datatypes.pb.h"
#include "proto/google/fhir/proto/r4/core/resources/bundle_and_contained_resource.pb.h"
#include "proto/google/fhir/proto/r4/uscore.pb.h"
#include "proto/google/fhir/proto/stu3/datatypes.pb.h"
#include "proto/google/fhir/proto/stu3/resources.pb.h"
#include "proto/google/fhir/proto/stu3/uscore.pb.h"

using ::std::string;

void WriteFootprintFile(const std::string& filename,
                        const std::string& footprint) {
  std::ofstream write_stream;
  std::cout << "Writing " << filename << std::endl;
  write_stream.open(filename);
  write_stream << footprint;
  write_stream.close();
}

int main(int argc, char** argv) {
  const string dir = "testdata/";

  WriteFootprintFile(
      absl::StrCat(dir, "stu3/footprint/stu3_contained_resource.fp"),
      google::fhir::ComputeResourceFootprint(
          google::fhir::stu3::proto::ContainedResource::descriptor()));
  WriteFootprintFile(
      absl::StrCat(dir, "r4/footprint/r4_contained_resource.fp"),
      google::fhir::ComputeResourceFootprint(
          google::fhir::r4::core::ContainedResource::descriptor()));

  WriteFootprintFile(
      absl::StrCat(dir, "stu3/footprint/stu3_datatypes.fp"),
      google::fhir::ComputeDatatypesFootprint(
          google::fhir::stu3::proto::String::descriptor()->file()));
  WriteFootprintFile(absl::StrCat(dir, "r4/footprint/r4_datatypes.fp"),
                     google::fhir::ComputeDatatypesFootprint(
                         google::fhir::r4::core::String::descriptor()->file()));

  WriteFootprintFile(
      absl::StrCat(dir, "stu3/footprint/stu3_uscore_resources.fp"),
      google::fhir::ComputeResourceFileFootprint(
          google::fhir::stu3::uscore::UsCorePatient::descriptor()->file()));

  WriteFootprintFile(
      absl::StrCat(dir, "r4/footprint/r4_uscore_resources.fp"),
      google::fhir::ComputeResourceFileFootprint(
          google::fhir::r4::uscore::USCorePatientProfile::descriptor()
              ->file()));

  return 0;
}
