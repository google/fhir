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

#ifndef GOOGLE_FHIR_STATUS_STATUS_H_
#define GOOGLE_FHIR_STATUS_STATUS_H_

#include "absl/status/status.h"

namespace google {
namespace fhir {

#define FHIR_RETURN_IF_ERROR(expr) \
  {                                \
    auto _status = (expr);         \
    if (!_status.ok()) {           \
      return _status;              \
    }                              \
  }

#define FHIR_ASSERT_OK(rexpr)                     \
  {                                               \
    auto status = (rexpr);                        \
    ASSERT_TRUE(status.ok()) << status.message(); \
  }

#define FHIR_CHECK_OK(rexpr)                \
  {                                         \
    auto status = (rexpr);                  \
    CHECK(status.ok()) << status.message(); \
  }

#define FHIR_DCHECK_OK(rexpr)                \
  {                                          \
    auto status = (rexpr);                   \
    DCHECK(status.ok()) << status.message(); \
  }

#define FHIR_ASSERT_STATUS(rexpr, msg) \
  {                                    \
    auto status = (rexpr);             \
    ASSERT_FALSE(status.ok());         \
    ASSERT_EQ(status.message(), msg);  \
  }

}  // namespace fhir
}  // namespace google

#endif  // GOOGLE_FHIR_STATUS_STATUS_H_
