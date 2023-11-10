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

#include "google/fhir/footprint/proto_footprint.h"

#include <algorithm>
#include <string>
#include <unordered_set>
#include <vector>

#include "google/protobuf/descriptor.h"
#include "absl/container/btree_set.h"
#include "absl/container/flat_hash_set.h"
#include "absl/container/node_hash_set.h"
#include "absl/strings/str_cat.h"
#include "google/fhir/annotations.h"
#include "proto/google/fhir/proto/r4/core/datatypes.pb.h"
#include "proto/google/fhir/proto/stu3/datatypes.pb.h"
#include "re2/re2.h"

using ::google::protobuf::Descriptor;
using ::google::protobuf::EnumDescriptor;
using ::google::protobuf::FieldDescriptor;
using ::google::protobuf::FileDescriptor;
using ::std::string;
using ::std::vector;

namespace google {
namespace fhir {

namespace {

enum FpType { SIMPLE_TYPES, RESOURCES };

struct Token {
  int tag;
  string name;
};

string PrintTokens(const vector<Token>& tokens) {
  string tokens_string;
  for (const Token& token : tokens) {
    tokens_string += absl::StrCat(token.tag, "|", token.name, ",");
  }
  return tokens_string;
}

string ComputeEnumFootprint(const EnumDescriptor* enum_desc,
                            const vector<Token>& tokens) {
  string footprint;

  for (int i = 0; i < enum_desc->value_count(); i++) {
    footprint +=
        absl::StrCat(PrintTokens(tokens), enum_desc->value(i)->number(), "=",
                     enum_desc->value(i)->name(), "\n");
  }

  return footprint;
}

bool IsSimpleType(const Descriptor* descriptor) {
  return (google::fhir::IsPrimitive(descriptor) ||
          google::fhir::IsComplex(descriptor)) &&
         !google::fhir::IsProfile(descriptor) &&
         !google::fhir::HasValueset(descriptor);
}

string MessageName(const Descriptor* descriptor) {
  static const LazyRE2 core_regex{
      "google\\.fhir\\.(r4\\.core|stu3\\.proto)\\.[A-Za-z]*"};
  if (RE2::FullMatch(descriptor->full_name(), *core_regex)) {
    return descriptor->name();
  }
  return descriptor->full_name();
}

// Note: Visited Types contains ONLY types that are already in the parent chain
// of a given field, NOT types that were visited in other branches of the tree.
// Its only goal is to avoid infinite recursion.
string ComputeProtoFootprint(const Descriptor* descriptor,
                             const vector<Token>& tokens,
                             const absl::flat_hash_set<string>& visited_types,
                             const FpType fp_types) {
  static const absl::node_hash_set<string>* skip_types =
      new absl::node_hash_set<string>{
          google::fhir::r4::core::Extension::descriptor()->full_name(),
          google::fhir::r4::core::Reference::descriptor()->full_name(),
          google::fhir::stu3::proto::Extension::descriptor()->full_name(),
          google::fhir::stu3::proto::Reference::descriptor()->full_name()};

  string footprint;

  vector<const FieldDescriptor*> fields;
  fields.reserve(descriptor->field_count());
  for (int i = 0; i < descriptor->field_count(); i++) {
    fields.push_back(descriptor->field(i));
  }
  std::sort(fields.begin(), fields.end(),
            [](auto a, auto b) -> bool { return a->number() < b->number(); });

  for (const FieldDescriptor* field : fields) {
    vector<Token> new_tokens = tokens;
    new_tokens.push_back({field->number(), field->name()});

    if (field->message_type()) {
      if (visited_types.find(field->message_type()->full_name()) ==
              visited_types.end() &&
          skip_types->find(field->message_type()->full_name()) ==
              skip_types->end() &&
          field->name() != "contained" &&
          (!IsSimpleType(field->message_type()) || fp_types == SIMPLE_TYPES)) {
        absl::flat_hash_set<string> new_visited_types = visited_types;
        new_visited_types.insert(field->message_type()->full_name());
        footprint += ComputeProtoFootprint(field->message_type(), new_tokens,
                                           new_visited_types, fp_types);
      } else {
        footprint += absl::StrCat(PrintTokens(new_tokens),
                                  MessageName(field->message_type()), "\n");
      }
    } else if (field->enum_type()) {
      footprint += ComputeEnumFootprint(field->enum_type(), new_tokens);
    } else {
      footprint +=
          absl::StrCat(PrintTokens(new_tokens), field->type_name(), "\n");
    }
  }

  return footprint;
}

bool IsValueSetOrCodeSystemEnum(const Descriptor* type) {
  return google::fhir::HasValueset(type) ||
         (type->enum_type_count() == 1 &&
          !type->enum_type(0)
               ->options()
               .GetExtension(google::fhir::proto::fhir_code_system_url)
               .empty());
}

struct DescriptorComparator {
  bool operator()(const Descriptor* lhs, const Descriptor* rhs) const {
    return lhs->name().compare(rhs->name()) < 0;
  }
};

string ComputeFileFootprint(const FileDescriptor* file, const FpType fp_type) {
  string footprint;
  absl::btree_set<const Descriptor*, DescriptorComparator> descriptors;

  for (int i = 0; i < file->message_type_count(); i++) {
    descriptors.insert(file->message_type(i));
  }
  for (const Descriptor* type : descriptors) {
    // Skip ElementDefinition because it's enormous and only used by specs.
    if (type->name() == "ElementDefinition") continue;
    if (!IsValueSetOrCodeSystemEnum(type)) {
      footprint += type->full_name() + "\n";
      footprint += ComputeProtoFootprint(
          type, vector<Token>(), absl::flat_hash_set<string>(), fp_type);
    }
  }

  return footprint;
}

}  // namespace

string ComputeResourceFileFootprint(const FileDescriptor* file) {
  return ComputeFileFootprint(file, RESOURCES);
}

string ComputeResourceFootprint(const Descriptor* type) {
  return ComputeProtoFootprint(type, vector<Token>(),
                               absl::flat_hash_set<string>{type->full_name()},
                               RESOURCES);
}

string ComputeDatatypesFootprint(const FileDescriptor* file) {
  return ComputeFileFootprint(file, SIMPLE_TYPES);
}

}  // namespace fhir
}  // namespace google
