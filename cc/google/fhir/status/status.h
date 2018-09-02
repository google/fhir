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

#include "tensorflow/core/lib/core/status.h"
#include "tensorflow/core/platform/macros.h"

namespace google {
namespace fhir {

using ::tensorflow::Status;  // TENSORFLOW_STATUS_OK

#define FHIR_RETURN_IF_ERROR(x) TF_RETURN_IF_ERROR(x)

}  // namespace fhir
}  // namespace google

#endif  // GOOGLE_FHIR_STATUS_STATUS_H_
