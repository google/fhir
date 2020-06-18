/*
 * Copyright 2020 Google LLC
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

#ifndef GOOGLE_FHIR_SEQEX_STU3_H_
#define GOOGLE_FHIR_SEQEX_STU3_H_

#include "google/fhir/seqex/resource_to_example.h"
#include "google/fhir/seqex/text_tokenizer.h"
#include "google/fhir/stu3/primitive_handler.h"

namespace google {
namespace fhir {
namespace seqex_stu3 {

// Namespace for aliases for seqex functions for use with the STU3 FHIR version.

inline void ResourceToExample(const google::protobuf::Message& message,
                              const seqex::TextTokenizer& tokenizer,
                              ::tensorflow::Example* example,
                              bool enable_attribution) {
  ResourceToExample(message, tokenizer, example, enable_attribution,
                    ::google::fhir::stu3::Stu3PrimitiveHandler::GetInstance());
}

}  // namespace seqex_stu3
}  // namespace fhir
}  // namespace google

#endif  // GOOGLE_FHIR_SEQEX_STU3_H_
