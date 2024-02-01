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

#ifndef GOOGLE_FHIR_TYPE_MACROS_H_
#define GOOGLE_FHIR_TYPE_MACROS_H_

// Macro for specifying the ContainedResources type within a bundle-like object,
// given the Bundle-like type
#define BUNDLE_CONTAINED_RESOURCE(b)                         \
  typename std::remove_const<typename std::remove_reference< \
      decltype(std::declval<b>().entry(0).resource())>::type>::type

// Macro for specifying a type contained within a bundle-like object, given
// the Bundle-like type, and a field on the contained resource from that bundle.
// E.g., in a template with a BundleLike type,
// BUNDLE_TYPE(BundleLike, observation)
// will return the Observation type from that bundle.
#define BUNDLE_TYPE(b, r)                                    \
  typename std::remove_const<typename std::remove_reference< \
      decltype(std::declval<b>().entry(0).resource().r())>::type>::type

// Macro for specifying the extension type associated with a FHIR type.
#define EXTENSION_TYPE(t)                                    \
  typename std::remove_const<typename std::remove_reference< \
      decltype(std::declval<t>().id().extension(0))>::type>::type

// Given a FHIR type, returns the datatype with a given name associated with
// the input type's version of fhir
#define FHIR_DATATYPE(t, d)                                  \
  typename std::remove_const<typename std::remove_reference< \
      decltype(std::declval<t>().id().extension(0).value().d())>::type>::type

// Given a FHIR Reference type, gets the corresponding ReferenceId type.
#define REFERENCE_ID_TYPE(r)                                 \
  typename std::remove_const<typename std::remove_reference< \
      decltype(std::declval<r>().patient_id())>::type>::type

// Given a FHIR StructureDefinition type, gets the corresponding
// ElementDefinition type.
#define ELEMENT_DEFINITION_TYPE(t)                           \
  typename std::remove_const<typename std::remove_reference< \
      decltype(std::declval<t>().snapshot().element(0))>::type>::type

#endif  // GOOGLE_FHIR_TYPE_MACROS_H_
