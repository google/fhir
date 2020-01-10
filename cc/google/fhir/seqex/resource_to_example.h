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

#ifndef GOOGLE_FHIR_SEQEX_RESOURCE_TO_EXAMPLE_H_
#define GOOGLE_FHIR_SEQEX_RESOURCE_TO_EXAMPLE_H_

#include "google/protobuf/message.h"
#include "google/fhir/seqex/text_tokenizer.h"
#include "tensorflow/core/example/example.pb.h"

namespace google {
namespace fhir {
namespace seqex {

// Converts a FHIR resource into a tf.Example. If enable_attribution is true,
// additional features will be added to make it easier to run inference with
// attribution downstream. String features will be tokenized using the provided
// tokenizer.
void ResourceToExample(const google::protobuf::Message& message,
                       const TextTokenizer& tokenizer,
                       ::tensorflow::Example* example, bool enable_attribution);

}  // namespace seqex
}  // namespace fhir
}  // namespace google

#endif  // GOOGLE_FHIR_SEQEX_RESOURCE_TO_EXAMPLE_H_
