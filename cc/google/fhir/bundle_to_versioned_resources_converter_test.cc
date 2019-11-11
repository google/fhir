// Copyright 2018 Google LLC
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

#include "google/fhir/bundle_to_versioned_resources_converter.h"

#include <string>

#include "google/protobuf/text_format.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "google/fhir/testutil/proto_matchers.h"
#include "tensorflow/core/platform/env.h"

namespace google {
namespace fhir {


using ::google::fhir::stu3::proto::Bundle;
using ::google::fhir::stu3::proto::ContainedResource;
using ::google::fhir::testutil::EqualsProto;
using ::testing::Contains;
using ::testing::Pair;
using ::testing::UnorderedElementsAre;

void AssertCounter(const std::map<std::string, int>& counter_stats,
                   const std::string& counter, int value) {
  ASSERT_EQ(value, counter_stats.find(counter)->second)
      << "Expected counter " << counter << " to be " << value << " but was "
      << counter_stats.find(counter)->second;
}

const stu3::proto::VersionConfig GetConfig() {
  stu3::proto::VersionConfig result;
  TF_CHECK_OK(::tensorflow::ReadTextProto(
      ::tensorflow::Env::Default(),
      "proto/stu3/version_config.textproto",
      &result));
  return result;
}

std::vector<ContainedResource> RunBundleToVersionedResources(
    const Bundle& input, std::map<std::string, int>* counter_stats) {
  return BundleToVersionedResources(input, GetConfig(), counter_stats);
}

TEST(BundleToVersionedResourcesConverterTest, PatientWithTimeOfDeath) {
  Bundle input;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(R"proto(
    entry {
      resource {
        patient {
          id { value: "14" }
          birth_date {
            value_us: 200000000000
            precision: DAY
            timezone: "America/New_York"
          }
          deceased {
            date_time {
              value_us: 700000000000
              precision: MICROSECOND
              timezone: "America/New_York"
            }
          }
        }
      }
    }
    entry {
      resource {
        encounter {
          period {
            start {
              value_us: 500000000000
              precision: MICROSECOND
              timezone: "America/New_York"
            }
            end {
              value_us: 1000000000000
              precision: MICROSECOND
              timezone: "America/New_York"
            }
          }
        }
      }
    }
    entry {
      resource {
        encounter {
          period {
            start {
              value_us: 300000000000
              precision: MICROSECOND
              timezone: "America/New_York"
            }
            end {
              value_us: 1200000000000
              precision: MICROSECOND
              timezone: "America/New_York"
            }
          }
        }
      }
    })proto", &input));

  ContainedResource patient_v1;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(R"proto(
    patient {
      id { value: "14" }
      birth_date {
        value_us: 200000000000
        precision: DAY
        timezone: "America/New_York"
      }
      meta {
        version_id { value: "0" }
        last_updated {
          value_us: 277199000000
          precision: SECOND
          timezone: "America/New_York"
        }
      }
    })proto", &patient_v1));
  ContainedResource patient_v2;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(R"proto(
    patient {
      id { value: "14" }
      birth_date {
        value_us: 200000000000
        precision: DAY
        timezone: "America/New_York"
      }
      deceased {
        date_time {
          value_us: 700000000000
          precision: MICROSECOND
          timezone: "America/New_York"
        }
      }
      meta {
        version_id { value: "1" }
        last_updated {
          value_us: 700000000000
          precision: MICROSECOND
          timezone: "America/New_York"
        }
      }
    }
  )proto", &patient_v2));

  std::map<std::string, int> counter_stats;
  auto output = RunBundleToVersionedResources(input, &counter_stats);
  // 2 patient versions + 2 versions of each encounter = 6
  ASSERT_EQ(6, output.size());
  ASSERT_THAT(output, Contains(EqualsProto(patient_v1)));
  ASSERT_THAT(output, Contains(EqualsProto(patient_v2)));  // time of death
  AssertCounter(counter_stats, "num-unversioned-resources-in", 3);
  AssertCounter(counter_stats, "num-patient-split-to-2", 1);
}

TEST(BundleToVersionedResourcesConverterTest, PatientWithDeceasedBoolean) {
  Bundle input;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(R"proto(
    entry {
      resource {
        patient {
          id { value: "14" }
          birth_date {
            value_us: 200000000000
            precision: DAY
            timezone: "America/New_York"
          }
          deceased { boolean { value: true } }
        }
      }
    }
    entry {
      resource {
        encounter {
          period {
            start {
              value_us: 500000000000
              precision: MICROSECOND
              timezone: "America/New_York"
            }
            end {
              value_us: 100000000000000
              precision: MICROSECOND
              timezone: "America/New_York"
            }
          }
        }
      }
    }
    entry {
      resource {
        encounter {
          period {
            start {
              value_us: 300000000000
              precision: MICROSECOND
              timezone: "America/New_York"
            }
            end {
              value_us: 1200000000000
              precision: MICROSECOND
              timezone: "America/New_York"
            }
          }
        }
      }
    })proto", &input));

  ContainedResource patient_v1;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(R"proto(
    patient {
      id { value: "14" }
      birth_date {
        value_us: 200000000000
        precision: DAY
        timezone: "America/New_York"
      }
      meta {
        version_id { value: "0" }
        last_updated {
          value_us: 277199000000
          precision: SECOND
          timezone: "America/New_York"
        }
      }
    })proto", &patient_v1));

  ContainedResource patient_v2;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(R"proto(
    patient {
      id { value: "14" }
      birth_date {
        value_us: 200000000000
        precision: DAY
        timezone: "America/New_York"
      }
      deceased { boolean { value: true } }
      meta {
        version_id { value: "1" }
        last_updated {
          value_us: 131536000000000
          precision: MICROSECOND
          timezone: "America/New_York"
        }
      }
    }
  )proto", &patient_v2));

  std::map<std::string, int> counter_stats;
  auto output = RunBundleToVersionedResources(input, &counter_stats);
  // 2 patient versions + 2 versions of each encounter = 6
  ASSERT_EQ(6, output.size());
  ASSERT_THAT(output, Contains(EqualsProto(patient_v1)));
  ASSERT_THAT(output, Contains(EqualsProto(patient_v2)));  // time of death
  AssertCounter(counter_stats, "num-unversioned-resources-in", 3);
  AssertCounter(counter_stats, "num-patient-split-to-2", 1);
}

TEST(BundleToVersionedResourcesConverterTest, PatientWithoutEncounters) {
  Bundle input;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(R"proto(
    entry {
      resource {
        patient {
          id { value: "14" }
          birth_date {
            value_us: 200000000000
            precision: DAY
            timezone: "America/New_York"
          }
          deceased { boolean { value: true } }
        }
      }
    })proto", &input));

  ContainedResource patient_v1;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(R"proto(
    patient {
      id { value: "14" }
      birth_date {
        value_us: 200000000000
        precision: DAY
        timezone: "America/New_York"
      }
      meta {
        version_id { value: "0" }
        last_updated {
          value_us: 277199000000
          precision: SECOND
          timezone: "America/New_York"
        }
      }
    })proto", &patient_v1));

  std::map<std::string, int> counter_stats;
  auto output = RunBundleToVersionedResources(input, &counter_stats);
  ASSERT_EQ(1, output.size());
  ASSERT_THAT(output, testing::ElementsAre(EqualsProto(patient_v1)));
  AssertCounter(counter_stats, "patient-unknown-death-time-needs-attention", 1);
}

TEST(BundleToVersionedResourcesConverterTest,
     PatientWithoutEncountersOrBirthdate) {
  Bundle input;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(R"proto(
    entry {
      resource {
        patient {
          id { value: "14" }
          deceased { boolean { value: true } }
        }
      }
    })proto", &input));

  std::map<std::string, int> counter_stats;
  auto output = RunBundleToVersionedResources(input, &counter_stats);
  ASSERT_EQ(0, output.size());
  AssertCounter(counter_stats, "patient-unknown-death-time-needs-attention", 1);
  AssertCounter(counter_stats, "patient-unknown-entry-time-needs-attention", 1);
}

TEST(BundleToVersionedResourcesConverterTest, PatientWithoutDeath) {
  Bundle input;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(R"proto(
    entry {
      resource {
        patient {
          id { value: "14" }
          birth_date {
            value_us: 200000000000
            precision: DAY
            timezone: "America/New_York"
          }
        }
      }
    }
    entry {
      resource {
        encounter {
          period {
            start {
              value_us: 300000000000
              precision: MICROSECOND
              timezone: "America/New_York"
            }
            end {
              value_us: 1200000000000
              precision: MICROSECOND
              timezone: "America/New_York"
            }
          }
        }
      }
    })proto", &input));

  ContainedResource patient_v1;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(R"proto(
    patient {
      id { value: "14" }
      birth_date {
        value_us: 200000000000
        precision: DAY
        timezone: "America/New_York"
      }
      meta {
        version_id { value: "0" }
        last_updated {
          value_us: 277199000000
          precision: SECOND
          timezone: "America/New_York"
        }
      }
    })proto", &patient_v1));

  std::map<std::string, int> counter_stats;
  auto output = RunBundleToVersionedResources(input, &counter_stats);
  // 1 patient versions + 2 versions of each encounter = 4
  ASSERT_EQ(3, output.size());
  ASSERT_THAT(output, Contains(EqualsProto(patient_v1)));
  AssertCounter(counter_stats, "num-unversioned-resources-in", 2);
  AssertCounter(counter_stats, "num-patient-split-to-1", 1);
}

TEST(BundleToVersionedResourcesConverterTest, DefaultTimestampOnly) {
  Bundle input;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(R"proto(
    entry {
      resource {
        claim {
          language { value: "Klingon" }
          created {
            value_us: 200000000000
            precision: MICROSECOND
            timezone: "America/New_York"
          }
        }
      }
    })proto", &input));

  ContainedResource claim_v1;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(R"proto(
    claim {
      language { value: "Klingon" }
      created {
        value_us: 200000000000
        precision: MICROSECOND
        timezone: "America/New_York"
      }
      meta {
        version_id { value: "0" }
        last_updated {
          value_us: 200000000000
          precision: MICROSECOND
          timezone: "America/New_York"
        }
      }
    })proto", &claim_v1));

  std::map<std::string, int> counter_stats;
  ASSERT_THAT(RunBundleToVersionedResources(input, &counter_stats),
              UnorderedElementsAre(EqualsProto(claim_v1)));
  AssertCounter(counter_stats, "num-unversioned-resources-in", 1);
  AssertCounter(counter_stats, "num-claim-split-to-1", 1);
}

TEST(BundleToVersionedResourcesConverterTest, DefaultTimestampOnlyEmpty) {
  Bundle input;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(R"proto(
    entry { resource { claim { language { value: "Klingon" } } } })proto",
                                                  &input));

  std::map<std::string, int> counter_stats;
  ASSERT_THAT(RunBundleToVersionedResources(input, &counter_stats),
              testing::ElementsAre());
  AssertCounter(counter_stats, "split-failed-no-default_timestamp_field-Claim",
                1);
}

TEST(BundleToVersionedResourcesConverterTest, WithPopulatedOverride) {
  Bundle input;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(R"proto(
    entry {
      resource {
        encounter {
          period {
            start {
              value_us: 500000000000
              precision: MICROSECOND
              timezone: "America/New_York"
            }
            end {
              value_us: 1000000000000
              precision: MICROSECOND
              timezone: "America/New_York"
            }
          }
          language { value: "Klingon" }
          location { location { location_id { value: "dqd" } } }
          location { location { location_id { value: "kfa" } } }
        }
      }
    })proto", &input));

  ContainedResource v1;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(R"proto(
    encounter {
      period {
        start {
          value_us: 500000000000
          precision: MICROSECOND
          timezone: "America/New_York"
        }
      }
      language { value: "Klingon" }
      meta {
        version_id { value: "0" }
        last_updated {
          value_us: 500000000000
          precision: MICROSECOND
          timezone: "America/New_York"
        }
      }
    })proto", &v1));

  ContainedResource v2;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(R"proto(
    encounter {
      period {
        start {
          value_us: 500000000000
          precision: MICROSECOND
          timezone: "America/New_York"
        }
        end {
          value_us: 1000000000000
          precision: MICROSECOND
          timezone: "America/New_York"
        }
      }
      language { value: "Klingon" }
      location { location { location_id { value: "dqd" } } }
      location { location { location_id { value: "kfa" } } }
      meta {
        version_id { value: "1" }
        last_updated {
          value_us: 1000000000000
          precision: MICROSECOND
          timezone: "America/New_York"
        }
      }
    })proto", &v2));

  std::map<std::string, int> counter_stats;
  ASSERT_THAT(RunBundleToVersionedResources(input, &counter_stats),
              UnorderedElementsAre(EqualsProto(v1), EqualsProto(v2)));
  AssertCounter(counter_stats, "num-unversioned-resources-in", 1);
  AssertCounter(counter_stats, "num-encounter-split-to-2", 1);
}

TEST(BundleToVersionedResourcesConverterTest, WithEmptyOverride) {
  Bundle input;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(R"proto(
    entry {
      resource {
        encounter {
          period {
            start {
              value_us: 500000000000
              precision: MICROSECOND
              timezone: "America/New_York"
            }
          }
          language { value: "Klingon" }
          location { location { location_id { value: "dqd" } } }
          location { location { location_id { value: "kfa" } } }
        }
      }
    })proto", &input));

  ContainedResource v1;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(R"proto(
    encounter {
      period {
        start {
          value_us: 500000000000
          precision: MICROSECOND
          timezone: "America/New_York"
        }
      }
      language { value: "Klingon" }
      meta {
        version_id { value: "0" }
        last_updated {
          value_us: 500000000000
          precision: MICROSECOND
          timezone: "America/New_York"
        }
      }
    })proto", &v1));

  std::map<std::string, int> counter_stats;
  ASSERT_THAT(RunBundleToVersionedResources(input, &counter_stats),
              UnorderedElementsAre(EqualsProto(v1)));
  AssertCounter(counter_stats, "num-unversioned-resources-in", 1);
  AssertCounter(counter_stats, "num-encounter-split-to-1", 1);
}

TEST(BundleToVersionedResourcesConverterTest,
     WithPopulatedEncounterRefAndPopulatedDefault) {
  Bundle input;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(R"proto(
    entry {
      resource {
        encounter {
          period {
            start {
              value_us: 500000000000
              precision: MICROSECOND
              timezone: "America/New_York"
            }
            end {
              value_us: 1000000000000
              precision: MICROSECOND
              timezone: "America/New_York"
            }
          }
          id { value: "enc_id" }
        }
      }
    }
    entry {
      resource {
        condition {
          language { value: "Klingon" }
          asserted_date {
            value_us: 4700000000000
            precision: SECOND
            timezone: "America/New_York"
          }
          context { encounter_id { value: "enc_id" } }
        }
      }
    })proto", &input));

  ContainedResource v1;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(R"proto(
    condition {
      language { value: "Klingon" }
      asserted_date {
        value_us: 4700000000000
        precision: SECOND
        timezone: "America/New_York"
      }
      context { encounter_id { value: "enc_id" } }
      meta {
        version_id { value: "0" }
        last_updated {
          value_us: 4700000000000
          precision: SECOND
          timezone: "America/New_York"
        }
      }
    })proto", &v1));

  std::map<std::string, int> counter_stats;
  auto output = RunBundleToVersionedResources(input, &counter_stats);
  // 2 encounters + 1 condition = 3
  ASSERT_EQ(3, output.size());
  // encounter/offset ignored
  ASSERT_THAT(output, Contains(EqualsProto(v1)));
  AssertCounter(counter_stats, "num-unversioned-resources-in", 2);
  AssertCounter(counter_stats, "num-condition-split-to-1", 1);
}

TEST(BundleToVersionedResourcesConverterTest,
     WithNonResolvingEncounterRefAndEmptyDefault) {
  Bundle input;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(R"proto(
    entry {
      resource {
        condition {
          language { value: "Klingon" }
          context { encounter_id { value: "enc_id" } }
        }
      }
    })proto", &input));

  std::map<std::string, int> counter_stats;
  ASSERT_THAT(RunBundleToVersionedResources(input, &counter_stats),
              testing::ElementsAre());
  AssertCounter(counter_stats,
                "split-failed-no-default_timestamp_field-Condition", 1);
}

TEST(BundleToVersionedResourcesConverterTest,
     WithNonResolvingEncounterRefAndSetDefault) {
  Bundle input;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(R"proto(
    entry {
      resource {
        condition {
          language { value: "Klingon" }
          context { encounter_id { value: "enc_id" } }
          asserted_date {
            value_us: 4700000000000
            precision: MILLISECOND
            timezone: "America/New_York"
          }
        }
      }
    })proto", &input));

  ContainedResource v1;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(R"proto(
    condition {
      language { value: "Klingon" }
      context { encounter_id { value: "enc_id" } }
      asserted_date {
        value_us: 4700000000000
        precision: MILLISECOND
        timezone: "America/New_York"
      }
      meta {
        version_id { value: "0" }
        last_updated {
          value_us: 4700000000000
          precision: MILLISECOND
          timezone: "America/New_York"
        }
      }
    })proto", &v1));

  std::map<std::string, int> counter_stats;
  auto output = RunBundleToVersionedResources(input, &counter_stats);
  // encounter/offset ignored
  ASSERT_THAT(output, UnorderedElementsAre(EqualsProto(v1)));
  AssertCounter(counter_stats, "num-unversioned-resources-in", 1);
  AssertCounter(counter_stats, "num-condition-split-to-1", 1);
}

TEST(BundleToVersionedResourcesConverterTest, ExpandRepeatedTargets) {
  Bundle input;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(R"proto(
    entry {
      resource {
        composition {
          date {
            value_us: 1000000000000
            precision: SECOND
            timezone: "America/New_York"
          }
          section { title { value: "foo" } }
          section { title { value: "bar" } }
          attester {
            time {
              value_us: 1200000000000
              precision: SECOND
              timezone: "America/New_York"
            }
            party { id { value: "1999" } }
          }
          attester {
            time {
              value_us: 1400000000000
              precision: SECOND
              timezone: "America/New_York"
            }
            party { id { value: "animal" } }
          }
        }
      }
    })proto", &input));

  ContainedResource v1;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(R"proto(
    composition {
      date {
        value_us: 1000000000000
        precision: SECOND
        timezone: "America/New_York"
      }
      section { title { value: "foo" } }
      section { title { value: "bar" } }
      meta {
        version_id { value: "0" }
        last_updated {
          value_us: 1000000000000
          precision: SECOND
          timezone: "America/New_York"
        }
      }
    })proto", &v1));

  ContainedResource v2;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(R"proto(
    composition {
      date {
        value_us: 1000000000000
        precision: SECOND
        timezone: "America/New_York"
      }
      section { title { value: "foo" } }
      section { title { value: "bar" } }
      attester {
        time {
          value_us: 1200000000000
          precision: SECOND
          timezone: "America/New_York"
        }
        party { id { value: "1999" } }
      }
      meta {
        version_id { value: "1" }
        last_updated {
          value_us: 1200000000000
          precision: SECOND
          timezone: "America/New_York"
        }
      }
    })proto", &v2));
  ContainedResource v3;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(R"proto(
    composition {
      date {
        value_us: 1000000000000
        precision: SECOND
        timezone: "America/New_York"
      }
      section { title { value: "foo" } }
      section { title { value: "bar" } }
      attester {
        time {
          value_us: 1200000000000
          precision: SECOND
          timezone: "America/New_York"
        }
        party { id { value: "1999" } }
      }
      attester {
        time {
          value_us: 1400000000000
          precision: SECOND
          timezone: "America/New_York"
        }
        party { id { value: "animal" } }
      }
      meta {
        version_id { value: "2" }
        last_updated {
          value_us: 1400000000000
          precision: SECOND
          timezone: "America/New_York"
        }
      }
    })proto", &v3));

  std::map<std::string, int> counter_stats;
  auto output = RunBundleToVersionedResources(input, &counter_stats);
  ASSERT_THAT(output, UnorderedElementsAre(EqualsProto(v1), EqualsProto(v2),
                                           EqualsProto(v3)));
  AssertCounter(counter_stats, "num-unversioned-resources-in", 1);
  AssertCounter(counter_stats, "num-composition-split-to-3", 1);
}

TEST(BundleToVersionedResourcesConverterTest, PrecisionConversionDay) {
  Bundle input;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(R"proto(
    entry {
      resource {
        claim {
          language { value: "Klingon" }
          created {
            value_us: 1522209600000000
            precision: DAY
            timezone: "America/New_York"
          }
        }
      }
    })proto", &input));

  ContainedResource claim_v1;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(R"proto(
    claim {
      language { value: "Klingon" }
      created {
        value_us: 1522209600000000
        precision: DAY
        timezone: "America/New_York"
      }
      meta {
        version_id { value: "0" }
        last_updated {
          value_us: 1522295999000000
          precision: SECOND
          timezone: "America/New_York"
        }
      }
    })proto", &claim_v1));

  std::map<std::string, int> counter_stats;
  ASSERT_THAT(RunBundleToVersionedResources(input, &counter_stats),
              UnorderedElementsAre(EqualsProto(claim_v1)));
  AssertCounter(counter_stats, "num-unversioned-resources-in", 1);
  AssertCounter(counter_stats, "num-claim-split-to-1", 1);
}

TEST(BundleToVersionedResourcesConverterTest, PrecisionConversionMonth) {
  Bundle input;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(R"proto(
    entry {
      resource {
        claim {
          language { value: "Klingon" }
          created {
            value_us: 1519880400000000
            precision: MONTH
            timezone: "America/New_York"
          }
        }
      }
    })proto", &input));

  ContainedResource claim_v1;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(R"proto(
    claim {
      language { value: "Klingon" }
      created {
        value_us: 1519880400000000
        precision: MONTH
        timezone: "America/New_York"
      }
      meta {
        version_id { value: "0" }
        last_updated {
          value_us: 1522555199000000
          precision: SECOND
          timezone: "America/New_York"
        }
      }
    })proto", &claim_v1));

  std::map<std::string, int> counter_stats;
  ASSERT_THAT(RunBundleToVersionedResources(input, &counter_stats),
              UnorderedElementsAre(EqualsProto(claim_v1)));
  AssertCounter(counter_stats, "num-unversioned-resources-in", 1);
  AssertCounter(counter_stats, "num-claim-split-to-1", 1);
}

TEST(BundleToVersionedResourcesConverterTest, PrecisionConversionYear) {
  Bundle input;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(R"proto(
    entry {
      resource {
        claim {
          language { value: "Klingon" }
          created {
            value_us: 1519880400000000
            precision: YEAR
            timezone: "America/New_York"
          }
        }
      }
    })proto", &input));

  ContainedResource claim_v1;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(R"proto(
    claim {
      language { value: "Klingon" }
      created {
        value_us: 1519880400000000
        precision: YEAR
        timezone: "America/New_York"
      }
      meta {
        version_id { value: "0" }
        last_updated {
          value_us: 1546318799000000
          precision: SECOND
          timezone: "America/New_York"
        }
      }
    })proto", &claim_v1));

  std::map<std::string, int> counter_stats;
  ASSERT_THAT(RunBundleToVersionedResources(input, &counter_stats),
              UnorderedElementsAre(EqualsProto(claim_v1)));
  AssertCounter(counter_stats, "num-unversioned-resources-in", 1);
  AssertCounter(counter_stats, "num-claim-split-to-1", 1);
}

TEST(BundleToVersionedResourcesConverterTest, MultipleDefaultTimestamps) {
  Bundle input;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(R"proto(
    entry {
      resource {
        medication_administration {
          id { value: "1" }
          subject { patient_id { value: "123" } }
          context { encounter_id { value: "23" } }
          effective {
            date_time {
              # First Default
              value_us: 5500000000000000
              timezone: "UTC"
              precision: SECOND
            }
          }
          medication {
            reference { medication_id { value: "ItemDefinition-225158" } }
          }
          dosage {
            dose {
              value { value: "700" }
              unit { value: "ML" }
            }
            rate {
              quantity {
                value { value: "50" }
                unit { value: "PERCENT" }
              }
            }
          }
          note { text { value: "fluid" } }
        }
      }
    }
    entry {
      resource {
        medication_administration {
          id { value: "2" }
          subject { patient_id { value: "123" } }
          context { encounter_id { value: "23" } }
          effective {
            period {
              start {
                # Second Default only
                value_us: 5700000000000000
                timezone: "UTC"
                precision: SECOND
              }
            }
          }
          medication {
            reference { medication_id { value: "ItemDefinition-225158" } }
          }
          dosage {
            dose {
              value { value: "700" }
              unit { value: "ML" }
            }
            rate {
              quantity {
                value { value: "50" }
                unit { value: "PERCENT" }
              }
            }
          }
          note { text { value: "fluid" } }
        }
      }
    })proto", &input));

  ContainedResource ma_1;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(R"proto(
    medication_administration {
      id { value: "1" }
      subject { patient_id { value: "123" } }
      context { encounter_id { value: "23" } }
      effective {
        date_time {
          # First Default
          value_us: 5500000000000000
          timezone: "UTC"
          precision: SECOND
        }
      }
      medication {
        reference { medication_id { value: "ItemDefinition-225158" } }
      }
      dosage {
        dose {
          value { value: "700" }
          unit { value: "ML" }
        }
        rate {
          quantity {
            value { value: "50" }
            unit { value: "PERCENT" }
          }
        }
      }
      note { text { value: "fluid" } }
      meta {
        version_id { value: "0" }
        last_updated {
          value_us: 5500000000000000
          timezone: "UTC"
          precision: SECOND
        }
      }
    })proto", &ma_1));

  ContainedResource ma_2;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(R"proto(
    medication_administration {
      id { value: "2" }
      subject { patient_id { value: "123" } }
      context { encounter_id { value: "23" } }
      effective {
        period {
          start {
            # Second Default only
            value_us: 5700000000000000
            timezone: "UTC"
            precision: SECOND
          }
        }
      }
      medication {
        reference { medication_id { value: "ItemDefinition-225158" } }
      }
      dosage {
        dose {
          value { value: "700" }
          unit { value: "ML" }
        }
        rate {
          quantity {
            value { value: "50" }
            unit { value: "PERCENT" }
          }
        }
      }
      note { text { value: "fluid" } }
      meta {
        version_id { value: "0" }
        last_updated {
          value_us: 5700000000000000
          timezone: "UTC"
          precision: SECOND
        }
      }
    })proto", &ma_2));

  std::map<std::string, int> counter_stats;
  ASSERT_THAT(RunBundleToVersionedResources(input, &counter_stats),
              UnorderedElementsAre(EqualsProto(ma_1), EqualsProto(ma_2)));
  AssertCounter(counter_stats, "num-unversioned-resources-in", 2);
  AssertCounter(counter_stats, "num-medication_administration-split-to-1", 2);
}

TEST(BundleToVersionedResourcesConverterTest, TimeZoneWithFixedOffset) {
  Bundle input;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(R"proto(
    entry {
      resource {
        claim {
          language { value: "Klingon" }
          created {
            value_us: 1519880400000000
            precision: YEAR
            timezone: "-05:00"
          }
        }
      }
    })proto", &input));

  ContainedResource claim_v1;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(R"proto(
    claim {
      language { value: "Klingon" }
      created { value_us: 1519880400000000 precision: YEAR timezone: "-05:00" }
      meta {
        version_id { value: "0" }
        last_updated {
          value_us: 1546318799000000
          precision: SECOND
          timezone: "-05:00"
        }
      }
    })proto", &claim_v1));

  std::map<std::string, int> counter_stats;
  ASSERT_THAT(RunBundleToVersionedResources(input, &counter_stats),
              UnorderedElementsAre(EqualsProto(claim_v1)));
  AssertCounter(counter_stats, "num-unversioned-resources-in", 1);
  AssertCounter(counter_stats, "num-claim-split-to-1", 1);
}

}  // namespace fhir
}  // namespace google
