/* Copyright 2018 Google Inc.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
==============================================================================*/

#ifndef GOOGLE_FHIR_STATUS_STATUSOR_H_
#define GOOGLE_FHIR_STATUS_STATUSOR_H_

#include "absl/status/statusor.h"
#include "google/fhir/status/status.h"

// Internal helper for concatenating macro values.
#define FHIR_STATUS_MACROS_CONCAT_NAME_INNER(x, y) x##y
#define FHIR_STATUS_MACROS_CONCAT_NAME(x, y) \
  FHIR_STATUS_MACROS_CONCAT_NAME_INNER(x, y)

#define FHIR_ASSIGN_OR_RETURN(lhs, rexpr)                                 \
  FHIR_ASSIGN_OR_RETURN_IMPL(                                             \
      FHIR_STATUS_MACROS_CONCAT_NAME(_status_or_value, __COUNTER__), lhs, \
      rexpr)

#define FHIR_ASSIGN_OR_RETURN_IMPL(statusor, lhs, rexpr) \
  auto statusor = (rexpr);                               \
  if (!statusor.ok()) {                                  \
    return statusor.status();                            \
  }                                                      \
  lhs = std::move(statusor.value());

#define FHIR_ASSERT_OK_AND_ASSIGN(lhs, rexpr)                             \
  FHIR_ASSERT_OK_AND_ASSIGN_IMPL(                                         \
      FHIR_STATUS_MACROS_CONCAT_NAME(_status_or_value, __COUNTER__), lhs, \
      rexpr)

#define FHIR_ASSERT_OK_AND_ASSIGN_IMPL(statusor, lhs, rexpr) \
  auto statusor = (rexpr);                                   \
  if (!statusor.ok()) {                                      \
    LOG(ERROR) << statusor.status().message();               \
    ASSERT_TRUE(statusor.ok());                              \
  }                                                          \
  lhs = std::move(statusor.value());

#define FHIR_ASSERT_OK_AND_CONTAINS(lhs, rexpr)  \
  {                                              \
    auto statusor = (rexpr);                     \
    if (!statusor.ok()) {                        \
      LOG(ERROR) << statusor.status().message(); \
      ASSERT_TRUE(statusor.ok());                \
    }                                            \
    ASSERT_EQ(lhs, statusor.value());       \
  }

namespace google {
namespace fhir {

using ::absl::Status;

template <typename T>
using StatusOr = ::absl::StatusOr<T>;

}  // namespace fhir
}  // namespace google

#endif  // GOOGLE_FHIR_STATUS_STATUSOR_H_
