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

#ifndef GOOGLE_FHIR_STU3_BUNDLE_TO_VERSIONED_RESOURCES_CONVERTER_H_
#define GOOGLE_FHIR_STU3_BUNDLE_TO_VERSIONED_RESOURCES_CONVERTER_H_

#include <map>
#include <string>
#include <utility>

#include "proto/stu3/resources.pb.h"
#include "proto/stu3/version_config.pb.h"

namespace google {
namespace fhir {
namespace stu3 {

using std::string;

std::vector<stu3::proto::ContainedResource> BundleToVersionedResources(
    const stu3::proto::Bundle& bundle, const stu3::proto::VersionConfig& config,
    std::map<string, int>* counter_stats);

stu3::proto::Bundle BundleToVersionedBundle(
    const stu3::proto::Bundle& bundle, const stu3::proto::VersionConfig& config,
    std::map<string, int>* counter_stats);

}  // namespace stu3
}  // namespace fhir
}  // namespace google

#endif  // GOOGLE_FHIR_STU3_BUNDLE_TO_VERSIONED_RESOURCES_CONVERTER_H_
