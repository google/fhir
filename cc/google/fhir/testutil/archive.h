// Copyright 2022 Google LLC
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

#ifndef GOOGLE_FHIR_TESTUTIL_ARCHIVE_H_
#define GOOGLE_FHIR_TESTUTIL_ARCHIVE_H_

#include <functional>
#include <string>
#include <vector>

#include "absl/status/statusor.h"
#include "absl/strings/string_view.h"
#include "libarchive/archive.h"
#include "libarchive/archive_entry.h"

namespace google::fhir::testutil {

// File contents provided in CreateArchiveContaining calls.
struct ArchiveContents {
  const std::string name;
  const std::string data;
};

// Writes a zip file to a temporary file containing the given {name, data}
// structs. Returns the path to the temporary file.
// This is only provided for legacy usage - users should prefer to use
// CreateTarFileContaining instead.
absl::StatusOr<std::string> CreateZipFileContaining(
    const std::vector<ArchiveContents>& contents);

// Writes a tar file to a temporary file containing the given {name, data}
// structs. Returns the path to the temporary file.
absl::StatusOr<std::string> CreateTarFileContaining(
    const std::vector<ArchiveContents>& contents,
    absl::string_view filename = "");

}  // namespace google::fhir::testutil

#endif  // GOOGLE_FHIR_TESTUTIL_ARCHIVE_H_
