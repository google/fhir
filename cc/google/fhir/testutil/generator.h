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

#ifndef GOOGLE_FHIR_TESTUTIL_GENERATOR_H_
#define GOOGLE_FHIR_TESTUTIL_GENERATOR_H_

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "absl/container/flat_hash_map.h"
#include "absl/random/distributions.h"
#include "absl/random/random.h"
#include "absl/strings/str_format.h"
#include "google/fhir/error_reporter.h"
#include "google/fhir/primitive_handler.h"
#include "google/fhir/status/status.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/message.h"

namespace google {
namespace fhir {
namespace testutil {

// Provides values for a generated FHIR resource. Users may implement their
// own or use the random provider below, which simply produces syntatically
// valid but random values for each field.
class ValueProvider {
 public:
  virtual ~ValueProvider() {}
  virtual bool ShouldFill(const ::google::protobuf::FieldDescriptor*,
                          int recursion_depth) = 0;
  virtual int GetNumRepeated(const ::google::protobuf::FieldDescriptor*,
                             int recursion_depth) = 0;

  // Primitive field value providers. These functions are named
  // Get<FhirDataType> where the FHIR data types are defined
  // at https://www.hl7.org/fhir/datatypes.html
  virtual bool GetBoolean(const ::google::protobuf::FieldDescriptor* field,
                          int recursion_depth) = 0;
  virtual std::string GetBase64Binary(const ::google::protobuf::FieldDescriptor* field,
                                      int recursion_depth) = 0;
  virtual std::string GetString(const ::google::protobuf::FieldDescriptor* field,
                                int recursion_depth) = 0;
  virtual int GetInteger(const ::google::protobuf::FieldDescriptor* field,
                         int recursion_depth) = 0;
  virtual int GetPositiveInt(const ::google::protobuf::FieldDescriptor* field,
                             int recursion_depth) = 0;
  virtual int GetUnsignedInt(const ::google::protobuf::FieldDescriptor* field,
                             int recursion_depth) = 0;
  virtual std::string GetDecimal(const ::google::protobuf::FieldDescriptor* field,
                                 int recursion_depth) = 0;
  virtual std::string GetDateTime(const ::google::protobuf::FieldDescriptor* field,
                                  int recursion_depth) = 0;
  virtual std::string GetDate(const ::google::protobuf::FieldDescriptor* field,
                              int recursion_depth) = 0;
  virtual std::string GetTime(const ::google::protobuf::FieldDescriptor* field,
                              int recursion_depth) = 0;
  virtual std::string GetInstant(const ::google::protobuf::FieldDescriptor* field,
                                 int recursion_depth) = 0;
  virtual std::string GetId(const ::google::protobuf::FieldDescriptor* field,
                            int recursion_depth) = 0;
  virtual std::string GetUuid(const ::google::protobuf::FieldDescriptor* field,
                              int recursion_depth) = 0;
  virtual std::string GetIdentifier(const ::google::protobuf::FieldDescriptor* field,
                                    int recursion_depth) = 0;
  virtual std::string GetUri(const ::google::protobuf::FieldDescriptor* field,
                             int recursion_depth) = 0;
  virtual std::string GetUrl(const ::google::protobuf::FieldDescriptor* field,
                             int recursion_depth) = 0;
  virtual std::string GetCanonical(const ::google::protobuf::FieldDescriptor* field,
                                   int recursion_depth) = 0;
  virtual std::string GetOid(const ::google::protobuf::FieldDescriptor* field,
                             int recursion_depth) = 0;
  virtual std::string GetCode(const ::google::protobuf::FieldDescriptor* field,
                              int recursion_depth) = 0;
  virtual std::string GetMarkdown(const ::google::protobuf::FieldDescriptor* field,
                                  int recursion_depth) = 0;
  virtual std::string GetXhtml(const ::google::protobuf::FieldDescriptor* field,
                               int recursion_depth) = 0;

  // Returns the identifier to be used in reference field.
  virtual std::string GetReferenceId(const ::google::protobuf::FieldDescriptor* field,
                                     int recursion_depth) = 0;
  // Returns a FHIR reference type (e.g., Patient, Organization, Encounter)
  // to be used in a reference field.
  virtual std::string GetReferenceType(const ::google::protobuf::FieldDescriptor* field,
                                       int recursion_depth) = 0;
  virtual const ::google::protobuf::EnumValueDescriptor* GetCodeEnum(
      const ::google::protobuf::FieldDescriptor* primitive_field,
      const ::google::protobuf::FieldDescriptor* value_field, int recursion_depth) = 0;

  // For one-of fields, selects which one to populate.
  virtual const ::google::protobuf::FieldDescriptor* SelectOneOf(
      const ::google::protobuf::Message* message,
      const std::vector<const ::google::protobuf::FieldDescriptor*>& one_of_fields) = 0;
};

// Provides random FHIR values for fuzzing or load testing needs.
class RandomValueProvider : public ValueProvider {
 public:
  // Parameters object to pass to the constructor to govern for field value
  // generation. the `DefaultParams` function can be used to get reasonable
  // values for most use cases.
  //
  // - optional_set_probability is the probability an optional field will
  //     be set. If the field is in a recursive structure, that probability
  //.    is multiplied by optional_set_ratio_per_level for each recursion
  //.    so eventually the recursion will be terminated.
  //
  // - optional_set_ratio_per_level reduces the probability that a field
  //.    is set in a recursive structure as described above.
  //
  // - min_repeated is the minimum number of elements in a repeated field that
  //     has passed the optional_set_proability check.
  //
  // - max_repeated is the maximum number of elements in a repeated field that
  //     has passed the optional_set_proability check.
  //
  // - high_value is the high range for randomly generated nunmbers.
  //
  // - low_value is the low range for randomly generated numbers.
  //
  // - max_string_length defines the maximum length of generated strings
  //
  // - max_recursion_depth is the max depth that recursive fields will be
  //   populated.  This should be a positive integer, with 1 meaning
  //   "root only, no recursion".  Note optional_set_ratio_per_level causes a
  //   natural decay, so this can be set relatively high.
  struct Params {
    double optional_set_probability;
    double optional_set_ratio_per_level;
    int min_repeated;
    int max_repeated;
    int high_value;
    int low_value;
    int max_string_length;
    int max_recursion_depth;
    bool fill_extensions;
  };

  static Params DefaultParams() {
    return {.optional_set_probability = 0.75,
            .optional_set_ratio_per_level = 0.1,
            .min_repeated = 1,
            .max_repeated = 2,
            .high_value = 10000000,
            .low_value = -10000000,
            .max_string_length = 20,
            .max_recursion_depth = 10,
            .fill_extensions = true};
  }

  explicit RandomValueProvider(const Params& params) : params_(params) {}

  RandomValueProvider() : RandomValueProvider(DefaultParams()) {}

  bool ShouldFill(const ::google::protobuf::FieldDescriptor*,
                  int recursion_depth) override;
  int GetNumRepeated(const ::google::protobuf::FieldDescriptor*,
                     int recursion_depth) override;
  const ::google::protobuf::FieldDescriptor* SelectOneOf(
      const ::google::protobuf::Message* message,
      const std::vector<const ::google::protobuf::FieldDescriptor*>& one_of_fields)
      override;

  // Primitive field value providers. These functions are named
  // Get<FhirDataType> where the FHIR data types are defined
  // at https://www.hl7.org/fhir/datatypes.html
  bool GetBoolean(const ::google::protobuf::FieldDescriptor* field,
                  int recursion_depth) override;
  std::string GetBase64Binary(const ::google::protobuf::FieldDescriptor* field,
                              int recursion_depth) override;
  std::string GetString(const ::google::protobuf::FieldDescriptor* field,
                        int recursion_depth) override;
  int GetInteger(const ::google::protobuf::FieldDescriptor* field,
                 int recursion_depth) override;
  int GetPositiveInt(const ::google::protobuf::FieldDescriptor* field,
                     int recursion_depth) override;
  int GetUnsignedInt(const ::google::protobuf::FieldDescriptor* field,
                     int recursion_depth) override;
  std::string GetDecimal(const ::google::protobuf::FieldDescriptor* field,
                         int recursion_depth) override;
  std::string GetDateTime(const ::google::protobuf::FieldDescriptor* field,
                          int recursion_depth) override;
  std::string GetDate(const ::google::protobuf::FieldDescriptor* field,
                      int recursion_depth) override;
  std::string GetTime(const ::google::protobuf::FieldDescriptor* field,
                      int recursion_depth) override;
  std::string GetInstant(const ::google::protobuf::FieldDescriptor* field,
                         int recursion_depth) override;
  std::string GetId(const ::google::protobuf::FieldDescriptor* field,
                    int recursion_depth) override;
  std::string GetUuid(const ::google::protobuf::FieldDescriptor* field,
                      int recursion_depth) override;
  std::string GetIdentifier(const ::google::protobuf::FieldDescriptor* field,
                            int recursion_depth) override;
  std::string GetUri(const ::google::protobuf::FieldDescriptor* field,
                     int recursion_depth) override;
  std::string GetUrl(const ::google::protobuf::FieldDescriptor* field,
                     int recursion_depth) override;
  std::string GetCanonical(const ::google::protobuf::FieldDescriptor* field,
                           int recursion_depth) override;
  std::string GetOid(const ::google::protobuf::FieldDescriptor* field,
                     int recursion_depth) override;
  std::string GetCode(const ::google::protobuf::FieldDescriptor* field,
                      int recursion_depth) override;
  std::string GetMarkdown(const ::google::protobuf::FieldDescriptor* field,
                          int recursion_depth) override;
  std::string GetXhtml(const ::google::protobuf::FieldDescriptor* field,
                       int recursion_depth) override;
  std::string GetReferenceId(const ::google::protobuf::FieldDescriptor* field,
                             int recursion_depth) override;
  std::string GetReferenceType(const ::google::protobuf::FieldDescriptor* field,
                               int recursion_depth) override;
  const ::google::protobuf::EnumValueDescriptor* GetCodeEnum(
      const ::google::protobuf::FieldDescriptor* primitive_field,
      const ::google::protobuf::FieldDescriptor* value_field,
      int recursion_depth) override;

 private:
  // Helper methods to get FHIR partial dates that
  // may contain only the year or the year and month.
  std::string GetYear();
  std::string GetYearMonth();
  std::string GetFullDate();

  absl::BitGen bitgen_;
  const Params params_;
};

// Generates FHIR data. This is generally used for random values for
// fuzzing interfaces or load testing infrastructure, and will not produce
// anything that looks like real clinical data.
//
// Note that randomly generated data is not guaranteed to satisfy any
// data constraints (for instance, a period could end before it starts).
class FhirGenerator {
 public:
  explicit FhirGenerator(
      std::unique_ptr<ValueProvider>&& value_provider,
      const ::google::fhir::PrimitiveHandler* primitive_handler)
      : value_provider_(std::move(value_provider)),
        primitive_handler_(primitive_handler) {}

  // Fills the given FHIR resource with values provided by the
  // value provider.
  absl::Status Fill(::google::protobuf::Message* message) {
    absl::flat_hash_map<const ::google::protobuf::Descriptor*, int> recursion_count;
    return Fill(message, &recursion_count);
  }

 private:
  // The methods below fill FHIR message and primitive types.
  // The recursion_count parameter to these methods is a map that counts the
  // number of times the given descriptor has been encountered recursively,
  // allowing value providers to use that in their logic.
  absl::Status Fill(
      ::google::protobuf::Message* message,
      absl::flat_hash_map<const ::google::protobuf::Descriptor*, int>* recursion_count);

  absl::Status FillPrimitive(
      const ::google::protobuf::FieldDescriptor* field, ::google::protobuf::Message* message,
      absl::flat_hash_map<const ::google::protobuf::Descriptor*, int>* recursion_count);

  absl::Status FillReference(
      const ::google::protobuf::FieldDescriptor* field, ::google::protobuf::Message* message,
      absl::flat_hash_map<const ::google::protobuf::Descriptor*, int>* recursion_count);

  bool ShouldFill(
      const ::google::protobuf::FieldDescriptor* field, ::google::protobuf::Message* message,
      absl::flat_hash_map<const ::google::protobuf::Descriptor*, int>* recursion_count);

  std::unique_ptr<ValueProvider> value_provider_;
  const google::fhir::PrimitiveHandler* primitive_handler_;
};

}  // namespace testutil
}  // namespace fhir
}  // namespace google

#endif  // GOOGLE_FHIR_TESTUTIL_GENERATOR_H_
