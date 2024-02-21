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

#include "google/fhir/testutil/archive.h"

#include <cstdio>
#include <functional>
#include <string>
#include <vector>

#include "absl/cleanup/cleanup.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/str_format.h"
#include "absl/strings/string_view.h"
#include "libarchive/archive.h"
#include "libarchive/archive_entry.h"

namespace google::fhir::testutil {

namespace {
// Writes an archive to a temporary file containing the given {name,
// data} structs. The `set_archive_format` argument determines the type
// of archive written. Pass one of libarchive's functions such as
// `archive_write_set_format_ustar` or `archive_write_set_format_zip` to set the
// type of the archive.
absl::StatusOr<std::string> CreateArchiveContaining(
    const std::vector<ArchiveContents>& contents,
    const std::function<int(archive*)>& set_archive_format,
    absl::string_view filename = "") {
  archive* archive = archive_write_new();
  absl::Cleanup archive_closer = [&archive] { archive_write_free(archive); };

  int errorp = set_archive_format(archive);
  if (errorp != ARCHIVE_OK) {
    return absl::UnavailableError(absl::StrFormat(
        "Failed to create archive due to error code: %d.", errorp));
  }

  std::string temp_name =
      filename.empty() ? std::tmpnam(nullptr) : std::string(filename);
  errorp = archive_write_open_filename(archive, temp_name.c_str());
  if (errorp != ARCHIVE_OK) {
    return absl::UnavailableError(
        absl::StrFormat("Failed to create archive due to error code: %d %s.",
                        errorp, archive_error_string(archive)));
  }

  for (const ArchiveContents& file : contents) {
    archive_entry* entry = archive_entry_new();
    archive_entry_set_pathname(entry, file.name.c_str());
    archive_entry_set_size(entry, file.data.length());
    archive_entry_set_filetype(entry, AE_IFREG);
    absl::Cleanup entry_closer = [&entry] { archive_entry_free(entry); };

    errorp = archive_write_header(archive, entry);
    if (errorp != ARCHIVE_OK) {
      return absl::UnavailableError(
          absl::StrFormat("Unable to add file %s to archive, error code: %d %s",
                          file.name, errorp, archive_error_string(archive)));
    }

    la_ssize_t written =
        archive_write_data(archive, file.data.c_str(), file.data.length());
    if (written < file.data.length()) {
      return absl::UnavailableError(
          absl::StrFormat("Unable to add file %s to archive, error: %s",
                          file.name, archive_error_string(archive)));
    }
  }

  return temp_name;
}
}  // namespace

absl::StatusOr<std::string> CreateZipFileContaining(
    const std::vector<ArchiveContents>& contents) {
  return CreateArchiveContaining(contents, [](archive* archive) {
    return archive_write_set_format_zip(archive);
  });
}

absl::StatusOr<std::string> CreateTarFileContaining(
    const std::vector<ArchiveContents>& contents, absl::string_view filename) {
  return CreateArchiveContaining(
      contents,
      [](archive* archive) { return archive_write_set_format_ustar(archive); },
      filename);
}

}  // namespace google::fhir::testutil
