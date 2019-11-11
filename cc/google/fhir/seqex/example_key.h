/*
 * Copyright 2018 Google LLC
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef GOOGLE_FHIR_SEQEX_EXAMPLE_KEY_H_
#define GOOGLE_FHIR_SEQEX_EXAMPLE_KEY_H_

#include <string>
#include <tuple>
#include <utility>

#include "absl/time/time.h"

namespace google {
namespace fhir {
namespace seqex {


struct ExampleKey {
  std::string patient_id;
  absl::Time trigger_timestamp;
  std::string source;
  int start;
  int end;
  // Formats all data fields from ExampleKey:
  // <patient_id>:<start>-<end>@<timestamp>:<source>.
  std::string ToString() const;
  // Formats all data fields, and adds a prefix based on the hash
  // (to simplify shuffling for the caller):
  // <shuffle_prefix>-<patient_id>:<start>-<end>@<timestamp>:<source>.
  std::string ToStringWithPrefix() const;
  // Parses formatted string, with or without shuffle prefix and source:
  // <shuffle_prefix>-<patient_id>:<start>-<end>@<timestamp>:<source>.
  void FromString(const std::string& key);
  // Formats only patient id and timestamp: <patient_id>@<timestamp>.
  std::string ToPatientIdTimestampString() const;
  // Formats only patient id and source: <patient_id>:<source>.
  std::string ToPatientIdSourceString() const;
  // Parses formatted string with only patient id and timestamp:
  // <patient_id>@<timestamp>.
  void FromPatientIdTimestampString(const std::string& key);

  bool operator<(const ExampleKey& rhs) const {
    return std::tie(patient_id, trigger_timestamp, source, start, end) <
           std::tie(rhs.patient_id, rhs.trigger_timestamp, rhs.source,
                    rhs.start, rhs.end);
  }

  bool operator==(const ExampleKey& rhs) const {
    return std::tie(patient_id, trigger_timestamp, source, start, end) ==
           std::tie(rhs.patient_id, rhs.trigger_timestamp, rhs.source,
                    rhs.start, rhs.end);
  }

  std::ostream& operator<<(std::ostream& os) const {
    os << ToString();
    return os;
  }
};

}  // namespace seqex
}  // namespace fhir
}  // namespace google

#endif  // GOOGLE_FHIR_SEQEX_EXAMPLE_KEY_H_
