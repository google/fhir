//    Copyright 2018 Google Inc.
//
//    Licensed under the Apache License, Version 2.0 (the "License");
//    you may not use this file except in compliance with the License.
//    You may obtain a copy of the License at
//
//        https://www.apache.org/licenses/LICENSE-2.0
//
//    Unless required by applicable law or agreed to in writing, software
//    distributed under the License is distributed on an "AS IS" BASIS,
//    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//    See the License for the specific language governing permissions and
//    limitations under the License.

#include <fstream>
#include <iostream>
#include <string>

#include "absl/strings/str_cat.h"
#include "absl/time/time.h"
#include "google/fhir/stu3/json_format.h"
#include "google/fhir/stu3/profiles.h"
#include "examples/profiles/demo.pb.h"
#include "proto/stu3/datatypes.pb.h"
#include "proto/stu3/resources.pb.h"
#include "proto/stu3/uscore.pb.h"

using std::string;

using ::company::fhir::stu3::demo::DemoPatient;
using ::google::fhir::stu3::ConvertToProfileLenient;
using ::google::fhir::stu3::JsonFhirStringToProto;
using ::google::fhir::stu3::PrintFhirToJsonStringForAnalytics;
using ::google::fhir::stu3::proto::Patient;

template <typename R, typename P>
void ConvertToProfile(const absl::TimeZone& time_zone, std::string dir) {
  std::cout << "Converting Synthea " << R::descriptor()->name() << " to "
            << P::descriptor()->name() << std::endl;

  std::ifstream read_stream;
  std::cout << dir << std::endl;
  read_stream.open(
      absl::StrCat(dir, "/", R::descriptor()->name(), ".fhir.ndjson"));

  std::ofstream write_stream;
  write_stream.open(absl::StrCat(dir, "/", P::descriptor()->name(), ".ndjson"));

  string line;
  while (!read_stream.eof()) {
    std::getline(read_stream, line);
    if (!line.length()) continue;
    R raw = JsonFhirStringToProto<Patient>(line, time_zone).ValueOrDie();
    P profiled;
    CHECK(ConvertToProfileLenient(raw, &profiled).ok());
    write_stream
        << PrintFhirToJsonStringForAnalytics(profiled, time_zone).ValueOrDie();
    write_stream << "\n";
  }
}

int main(int argc, char** argv) {
  absl::TimeZone time_zone;
  CHECK(absl::LoadTimeZone("America/Los_Angeles", &time_zone));

  ConvertToProfile<Patient, DemoPatient>(time_zone, absl::StrCat(argv[1]));
  std::cout << "Done." << std::endl;
}
