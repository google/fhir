// Copyright 2018 Google LLC
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

#include "google/fhir/seqex/example_key.h"

#include <map>
#include <memory>
#include <string>

#include "absl/memory/memory.h"
#include "absl/strings/numbers.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_split.h"
#include "absl/time/time.h"
#include "tensorflow/core/platform/fingerprint.h"
#include "tensorflow/core/platform/logging.h"
#include "tensorflow/core/platform/types.h"
#include "re2/re2.h"

namespace google {
namespace fhir {
namespace seqex {

void ExampleKey::FromString(const std::string& key) {
  // e.g. "f26dd962a28daeb1-Patient/123:0-2@-957312000:Encounter/1"
  static LazyRE2 kStringFormatRegex = {
      "(?:[0-9a-f]{16}-)?"  // optional non-capturing group (shuffle prefix)
      "(Patient/[A-Za-z0-9\\-\\.]{1,64})"  // this->patient_id
      ":"
      "([0-9]+)"  // this->start
      "-"
      "([0-9]+)"  // this->end
      "@"
      "(-?[0-9]+)"  // this->trigger_timestamp (may be negative)
      "(?::"        // optional non-capturing group; require a leading colon if
                    // present
      "([A-Za-z]+/[A-Za-z0-9\\-\\.]{1,64})"  // this->source
      ")?"};
  tensorflow::int64 unix_seconds;
  CHECK(RE2::FullMatch(key, *kStringFormatRegex, &this->patient_id,
                       &this->start, &this->end, &unix_seconds, &this->source))
      << key;
  this->trigger_timestamp = absl::FromUnixSeconds(unix_seconds);
}

// We keep the offset and end for ease of debugging, although the label_time is
// what makes the key truly unique.
std::string ExampleKey::ToString() const {
  if (this->source.empty()) {
    return absl::StrCat(this->patient_id, ":", this->start, "-", this->end, "@",
                        absl::ToUnixSeconds(this->trigger_timestamp));
  }
  return absl::StrCat(this->patient_id, ":", this->start, "-", this->end, "@",
                      absl::ToUnixSeconds(this->trigger_timestamp), ":",
                      this->source);
}

// String form of key, with a uniformly distributed prefix for shuffling.
std::string ExampleKey::ToStringWithPrefix() const {
  return absl::StrCat(absl::Hex(::tensorflow::Fingerprint64(this->ToString()),
                                absl::kZeroPad16),
                      "-", this->ToString());
}

void ExampleKey::FromPatientIdTimestampString(const std::string& key) {
  static LazyRE2 kStringFormatRegex = {
      "(Patient/[A-Za-z0-9\\-\\.]{1,64})"  // this->patient_id
      "@"
      "(-?[0-9]+)"  // this->trigger_timestamp (may be negative)
  };
  tensorflow::int64 unix_seconds;
  CHECK(RE2::FullMatch(key, *kStringFormatRegex, &this->patient_id,
                       &unix_seconds))
      << key;
  this->trigger_timestamp = absl::FromUnixSeconds(unix_seconds);
  this->start = 0;
  this->end = 0;
}

std::string ExampleKey::ToPatientIdTimestampString() const {
  return absl::StrCat(this->patient_id, "@",
                      absl::ToUnixSeconds(this->trigger_timestamp));
}

std::string ExampleKey::ToPatientIdSourceString() const {
  return absl::StrCat(this->patient_id, ":", this->source);
}

}  // namespace seqex
}  // namespace fhir
}  // namespace google
