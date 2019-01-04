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

#include <map>
#include <string>
#include <utility>
#include <vector>

#include "gflags/gflags.h"
#include "google/protobuf/text_format.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "absl/strings/substitute.h"
#include "google/fhir/seqex/bundle_to_seqex_converter.h"
#include "google/fhir/seqex/example_key.h"
#include "google/fhir/testutil/proto_matchers.h"
#include "proto/stu3/google_extensions.pb.h"
#include "proto/stu3/resources.pb.h"
#include "tensorflow/core/example/example.pb.h"
#include "tensorflow/core/platform/env.h"

DECLARE_string(trigger_time_redacted_features);
DECLARE_bool(tokenize_code_text_features);

namespace google {
namespace fhir {
namespace seqex {

using std::string;

using google::fhir::stu3::google::EventTrigger;
using google::fhir::stu3::proto::Bundle;
using google::fhir::stu3::proto::VersionConfig;
using ::google::fhir::testutil::EqualsProto;
using ::tensorflow::SequenceExample;

class BundleToSeqexConverterTest : public ::testing::Test {
 public:
  void SetUp() override {
    TF_CHECK_OK(::tensorflow::ReadTextProto(
        tensorflow::Env::Default(),
        "proto/stu3/version_config.textproto",
        &fhir_version_config_));
    parser_.AllowPartialMessage(true);
    FLAGS_tokenize_code_text_features = true;
    FLAGS_trigger_time_redacted_features = "";
  }

  void PerformTest(const string& input_key, const Bundle& bundle,
                   const std::vector<TriggerLabelsPair>& trigger_labels_pair,
                   const std::map<string, SequenceExample>& expected) {
    // Until all config options for this object can be passed as args, we need
    // to initialize it after overriing the flags settings.
    BundleToSeqexConverter converter(fhir_version_config_,
                                     false /* enable_attribution */,
                                     false /* generate_sequence_label */);
    std::map<string, int> counter_stats;
    ASSERT_TRUE(converter.Begin(input_key, bundle, trigger_labels_pair,
                                &counter_stats));
    for (const auto& iter : expected) {
      EXPECT_EQ(converter.ExampleKey(), iter.first);
      EXPECT_THAT(converter.GetExample(), EqualsProto(iter.second))
          << "\nfor key: " << converter.ExampleKey();
      ASSERT_TRUE(converter.Next());
    }

    ASSERT_TRUE(converter.Done())
        << "key: " << converter.ExampleKey()
        << "\nvalue: " << converter.GetExample().DebugString();
  }

 protected:
  VersionConfig fhir_version_config_;
  google::protobuf::TextFormat::Parser parser_;
};

TEST_F(BundleToSeqexConverterTest, Observation) {
  FLAGS_tokenize_code_text_features = false;
  EventTrigger trigger;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(R"proto(
    event_time {
      value_us: 1420102800000000
      precision: SECOND
      timezone: "America/New_York"
    }
    source { encounter_id { value: "789" } }
  )proto", &trigger));
  std::vector<TriggerLabelsPair> trigger_labels_pair({{trigger, {}}});
  Bundle input;
  ASSERT_TRUE(parser_.ParseFromString(R"proto(
    entry {
      resource {
        patient {
          id { value: "14" }
          birth_date {
            value_us: -1323388800000000
            precision: DAY
            timezone: "America/New_York"
          }
        }
      }
    }
    entry {
      resource {
        encounter {
          id { value: "1" }
          subject { patient_id { value: "14" } }
          period {
            start {
              value_us: 1420099200000000  # "2015-01-01T08:00:00+00:00"
            }
            end {
              value_us: 1420102800000000  # "2015-01-01T09:00:00+00:00"
            }
          }
        }
      }
    }
    entry {
      resource {
        observation {
          id { value: "123" }
          subject { patient_id { value: "456" } }
          code {
            coding {
              system { value: "http://loinc.org" }
              code { value: "LAB50" }
            }
            text { value: "BILIRUBIN, TOTAL" }
          }
          context { encounter_id { value: "789" } }
          effective { date_time { value_us: 1420102700000000 } }
          value {
            quantity: {
              value { value: "0.5" }
              unit { value: "mEq/L" }
            }
          }
        }
      }
    })proto", &input));
  SequenceExample expected;
  ASSERT_TRUE(parser_.ParseFromString(R"proto(
    context {
      feature: {
        key: "Patient.birthDate"
        value { int64_list { value: -1323388800 } }
      }
      feature {
        key: "currentEncounterId"
        value { int64_list { value: 1420099200 } }
      }
      feature {
        key: "patientId"
        value { bytes_list { value: "14" } }
      }
      feature {
        key: "sequenceLength"
        value { int64_list { value: 3 } }
      }
      feature {
        key: "timestamp"
        value { int64_list { value: 1420102800 } }
      }
    }
    feature_lists {
      feature_list {
        key: "Encounter.meta.lastUpdated"
        value {
          feature { int64_list { value: 1420099200 } }
          feature { int64_list {} }
          feature { int64_list { value: 1420102800 } }
        }
      }
      feature_list {
        key: "Encounter.period.end"
        value {
          feature { int64_list {} }
          feature { int64_list {} }
          feature { int64_list { value: 1420102800 } }
        }
      }
      feature_list {
        key: "Encounter.period.start"
        value {
          feature { int64_list { value: 1420099200 } }
          feature { int64_list {} }
          feature { int64_list { value: 1420099200 } }
        }
      }
      feature_list {
        key: "Observation.meta.lastUpdated"
        value {
          feature { int64_list {} }
          feature { int64_list { value: 1420102700 } }
          feature { int64_list {} }
        }
      }
      feature_list {
        key: "Observation.code"
        value {
          feature { bytes_list {} }
          feature { bytes_list { value: "loinc:LAB50" } }
          feature { bytes_list {} }
        }
      }
      feature_list {
        key: "Observation.code.text"
        value {
          feature { bytes_list {} }
          feature { bytes_list { value: "BILIRUBIN, TOTAL" } }
          feature { bytes_list {} }
        }
      }
      feature_list {
        key: "Observation.code.loinc"
        value {
          feature { bytes_list {} }
          feature { bytes_list { value: "LAB50" } }
          feature { bytes_list {} }
        }
      }
      feature_list {
        key: "Observation.effective.dateTime"
        value {
          feature { int64_list {} }
          feature { int64_list { value: 1420102700 } }
          feature { int64_list {} }
        }
      }
      feature_list {
        key: "Observation.value.quantity.unit"
        value {
          feature { bytes_list {} }
          feature { bytes_list { value: "mEq/L" } }
          feature { bytes_list {} }
        }
      }
      feature_list {
        key: "Observation.value.quantity.value"
        value {
          feature { float_list {} }
          feature { float_list { value: 0.5 } }
          feature { float_list {} }
        }
      }
      feature_list {
        key: "encounterId"
        value {
          feature { int64_list { value: 1420099200 } }
          feature { int64_list { value: 1420099200 } }
          feature { int64_list { value: 1420099200 } }
        }
      }
      feature_list {
        key: "eventId"
        value {
          feature { int64_list { value: 1420099200 } }
          feature { int64_list { value: 1420102700 } }
          feature { int64_list { value: 1420102800 } }
        }
      }
    })proto", &expected));

  PerformTest("Patient/14", input, trigger_labels_pair,
              {{"Patient/14:0-3@1420102800:Encounter/789", expected}});
}

TEST_F(BundleToSeqexConverterTest, TestMultipleResources) {
  EventTrigger trigger;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(R"proto(
    event_time {
      value_us: 1420102800000000
      precision: SECOND
      timezone: "America/New_York"
    }
    source { encounter_id { value: "1" } }
  )proto", &trigger));
  std::vector<TriggerLabelsPair> trigger_labels_pair({{trigger, {}}});
  Bundle bundle;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(R"proto(
    entry {
      resource {
        patient {
          id { value: "14" }
          birth_date {
            value_us: -1323388800000000
            precision: DAY
            timezone: "America/New_York"
          }
        }
      }
    }
    entry {
      resource {
        condition {
          id { value: "1" }
          subject { patient_id { value: "14" } }
          code {
            coding {
              system { value: "http://hl7.org/fhir/sid/icd-9-cm/diagnosis" }
              code { value: "bar" }
            }
          }
          asserted_date {
            value_us: 1417392000000000  # "2014-12-01T00:00:00+00:00"
          }
        }
      }
    }
    entry {
      resource {
        condition {
          id { value: "2" }
          subject { patient_id { value: "14" } }
          code {
            coding {
              system { value: "http://hl7.org/fhir/sid/icd-9-cm/diagnosis" }
              code { value: "baz" }
            }
          }
          asserted_date {
            value_us: 1420099200000000  # "2015-01-01T08:00:00+00:00"
          }
        }
      }
    }
    entry {
      resource {
        composition {
          id { value: "1" }
          subject { patient_id { value: "14" } }
          encounter { encounter_id { value: "1" } }
          section { text { div { value: "test text" } } }
          date { value_us: 1420102800000000 timezone: "UTC" precision: SECOND }
        }
      }
    }
    entry {
      resource {
        encounter {
          id { value: "1" }
          subject { patient_id { value: "14" } }
          class_value {
            system { value: "http://hl7.org/fhir/v3/ActCode" }
            code { value: "IMP" }
          }
          reason {
            coding {
              system { value: "http://hl7.org/fhir/sid/icd-9-cm/diagnosis" }
              code { value: "191.4" }
              display { value: "Malignant neoplasm of occipital lobe" }
            }
          }
          period {
            start {
              value_us: 1420099200000000  # "2015-01-01T08:00:00+00:00"
            }
            end {
              value_us: 1420102800000000  # "2015-01-01T09:00:00+00:00"
            }
          }
        }
      }
    })proto", &bundle));

  SequenceExample seqex;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(R"proto(
    context: {
      feature: {
        key: "Patient.birthDate"
        value { int64_list { value: -1323388800 } }
      }
      feature {
        key: "currentEncounterId"
        value { int64_list { value: 1420099200 } }
      }
      feature {
        key: "patientId"
        value { bytes_list { value: "14" } }
      }
      feature {
        key: "sequenceLength"
        value { int64_list { value: 5 } }
      }
      feature {
        key: "timestamp"
        value { int64_list { value: 1420102800 } }
      }
    }
    feature_lists: {
      feature_list {
        key: "Composition.date"
        value {
          feature { int64_list {} }
          feature { int64_list {} }
          feature { int64_list {} }
          feature { int64_list { value: 1420102800 } }
          feature { int64_list {} }
        }
      }
      feature_list {
        key: "Composition.meta.lastUpdated"
        value {
          feature { int64_list {} }
          feature { int64_list {} }
          feature { int64_list {} }
          feature { int64_list { value: 1420102800 } }
          feature { int64_list {} }
        }
      }
      feature_list {
        key: "Composition.section.text.div.tokenized"
        value {
          feature { bytes_list {} }
          feature { bytes_list {} }
          feature { bytes_list {} }
          feature { bytes_list { value: "test" value: "text" } }
          feature { bytes_list {} }
        }
      }
      feature_list {
        key: "Condition.meta.lastUpdated"
        value {
          feature { int64_list { value: 1417392000 } }
          feature { int64_list { value: 1420099200 } }
          feature { int64_list {} }
          feature { int64_list {} }
          feature { int64_list {} }
        }
      }
      feature_list {
        key: "Condition.code"
        value {
          feature { bytes_list { value: "icd9:bar" } }
          feature { bytes_list { value: "icd9:baz" } }
          feature { bytes_list {} }
          feature { bytes_list {} }
          feature { bytes_list {} }
        }
      }
      feature_list {
        key: "Condition.code.icd9"
        value {
          feature { bytes_list { value: "bar" } }
          feature { bytes_list { value: "baz" } }
          feature { bytes_list {} }
          feature { bytes_list {} }
          feature { bytes_list {} }
        }
      }
      feature_list {
        key: "Condition.assertedDate"
        value {
          feature { int64_list { value: 1417392000 } }
          feature { int64_list { value: 1420099200 } }
          feature { int64_list {} }
          feature { int64_list {} }
          feature { int64_list {} }
        }
      }
      feature_list {
        key: "Encounter.meta.lastUpdated"
        value {
          feature { int64_list {} }
          feature { int64_list {} }
          feature { int64_list { value: 1420099200 } }
          feature { int64_list {} }
          feature { int64_list { value: 1420102800 } }
        }
      }
      feature_list {
        key: "Encounter.class"
        value {
          feature { bytes_list {} }
          feature { bytes_list {} }
          feature { bytes_list { value: "actcode:IMP" } }
          feature { bytes_list {} }
          feature { bytes_list { value: "actcode:IMP" } }
        }
      }
      feature_list {
        key: "Encounter.period.end"
        value {
          feature { int64_list {} }
          feature { int64_list {} }
          feature { int64_list {} }
          feature { int64_list {} }
          feature { int64_list { value: 1420102800 } }
        }
      }
      feature_list {
        key: "Encounter.period.start"
        value {
          feature { int64_list {} }
          feature { int64_list {} }
          feature { int64_list { value: 1420099200 } }
          feature { int64_list {} }
          feature { int64_list { value: 1420099200 } }
        }
      }
      feature_list {
        key: "Encounter.reason"
        value {
          feature { bytes_list {} }
          feature { bytes_list {} }
          feature { bytes_list {} }
          feature { bytes_list {} }
          feature { bytes_list { value: "icd9:191.4" } }
        }
      }
      feature_list {
        key: "Encounter.reason.icd9"
        value {
          feature { bytes_list {} }
          feature { bytes_list {} }
          feature { bytes_list {} }
          feature { bytes_list {} }
          feature { bytes_list { value: "191.4" } }
        }
      }
      feature_list {
        key: "Encounter.reason.icd9.display.tokenized"
        value {
          feature { bytes_list {} }
          feature { bytes_list {} }
          feature { bytes_list {} }
          feature { bytes_list {} }
          feature {
            bytes_list {
              value: "malignant"
              value: "neoplasm"
              value: "of"
              value: "occipital"
              value: "lobe"
            }
          }
        }
      }
      feature_list {
        key: "encounterId"
        value {
          feature { int64_list { value: 1417392000 } }
          feature { int64_list { value: 1420099200 } }
          feature { int64_list { value: 1420099200 } }
          feature { int64_list { value: 1420099200 } }
          feature { int64_list { value: 1420099200 } }
        }
      }
      feature_list {
        key: "eventId"
        value {
          feature { int64_list { value: 1417392000 } }
          feature { int64_list { value: 1420099200 } }
          feature { int64_list { value: 1420099200 } }
          feature { int64_list { value: 1420102800 } }
          feature { int64_list { value: 1420102800 } }
        }
      }
    })proto", &seqex));

  PerformTest("Patient/14", bundle, trigger_labels_pair,
              {{"Patient/14:0-5@1420102800:Encounter/1", seqex}});
}

// Test the case where multiple triggers have the exact same timestamp, but are
// associated with different source encounters.
TEST_F(BundleToSeqexConverterTest, MultipleLabelsSameTimestamp) {
  EventTrigger trigger1;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(R"proto(
    event_time { value_us: 1420099200000000 }
    source { encounter_id { value: "1" } }
  )proto", &trigger1));
  EventTrigger trigger2;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(R"proto(
    event_time { value_us: 1420101000000000 }
    source { encounter_id { value: "3" } }
  )proto", &trigger2));
  EventTrigger trigger3;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(R"proto(
    event_time { value_us: 1420099200000000 }
    source { encounter_id { value: "2" } }
  )proto", &trigger3));
  std::vector<TriggerLabelsPair> trigger_labels_pair(
      {{trigger1, {}}, {trigger2, {}}, {trigger3, {}}});

  Bundle bundle;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(R"proto(
    entry {
      resource {
        patient {
          id { value: "14" }
          birth_date {
            value_us: -1323388800000000
            precision: DAY
            timezone: "America/New_York"
          }
        }
      }
    }
    entry {
      resource {
        encounter {
          id { value: "1" }
          subject { patient_id { value: "14" } }
          class_value {
            system { value: "http://hl7.org/fhir/v3/ActCode" }
            code { value: "IMP" }
          }
          reason {
            coding {
              system { value: "http://hl7.org/fhir/sid/icd-9-cm/diagnosis" }
              code { value: "191.4" }
              display { value: "Malignant neoplasm of occipital lobe" }
            }
          }
          period {
            start {
              value_us: 1420099200000000  # "2015-01-01T08:00:00+00:00"
            }
            end {
              value_us: 1420102800000000  # "2015-01-01T09:00:00+00:00"
            }
          }
        }
      }
    })proto", &bundle));

  string seqex_tmpl = R"(
      context: {
        feature { key: "Patient.birthDate" value { int64_list { value: -1323388800 } } }
        feature {
          key: "currentEncounterId"
          value { int64_list { value: 1420099200 } }
        }
        $0
        feature {
          key: "patientId"
          value { bytes_list { value: "14" } }
        }
        feature {
          key: "sequenceLength"
          value { int64_list { value: 1 } }
        }
        feature {
          key: "timestamp"
          value { int64_list { value: $1 } }
        }
      }
      feature_lists: {
        feature_list {
          key: "Encounter.meta.lastUpdated"
          value { feature { int64_list { value: 1420099200 } } }
        }
        feature_list {
          key: "Encounter.class"
          value { feature { bytes_list { value: "actcode:IMP" } } }
        }
        feature_list {
          key: "Encounter.period.end"
          value { feature { int64_list { } } }
        }
        feature_list {
          key: "Encounter.period.start"
          value { feature { int64_list { value: 1420099200 } } }
        }
        feature_list {
          key: "Encounter.reason"
          value { feature { bytes_list { } } }
        }
        feature_list {
          key: "Encounter.reason.icd9"
          value { feature { bytes_list { } } }
        }
        feature_list {
          key: "Encounter.reason.icd9.display.tokenized"
          value { feature { bytes_list { } } }
        }
        feature_list {
          key: "encounterId"
          value { feature { int64_list { value: 1420099200 } } }
        }
        feature_list {
          key: "eventId"
          value { feature { int64_list { value: 1420099200 } } }
        }
      })";
  SequenceExample seqex;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(
      absl::Substitute(seqex_tmpl, "", "1420099200"), &seqex));
  SequenceExample seqex2;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(
      absl::Substitute(seqex_tmpl, "", "1420101000"), &seqex2));

  PerformTest("Patient/14", bundle, trigger_labels_pair,
              {{"Patient/14:0-1@1420099200:Encounter/1", seqex},
               {"Patient/14:0-1@1420099200:Encounter/2", seqex},
               {"Patient/14:0-1@1420101000:Encounter/3", seqex2}});
}

TEST_F(BundleToSeqexConverterTest, TestClassLabel) {
  EventTrigger trigger;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(R"proto(
    event_time { value_us: 1420444800000000 }
    source { encounter_id { value: "1" } }
  )proto", &trigger));
  std::vector<TriggerLabelsPair> trigger_labels_pair({{trigger, {}}});
  Bundle bundle;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(R"proto(
    entry {
      resource {
        patient {
          id { value: "14" }
          birth_date {
            value_us: -1323388800000000
            precision: DAY
            timezone: "America/New_York"
          }
        }
      }
    }
    entry {
      resource {
        encounter {
          id { value: "1" }
          subject { patient_id { value: "14" } }
          class_value {
            system { value: "http://hl7.org/fhir/v3/ActCode" }
            code { value: "IMP" }
          }
          period {
            start {
              value_us: 1420444800000000  # "2015-01-05T08:00:00+00:00"
            }
            end {
              value_us: 1420455600000000  # "2015-01-05T11:00:00+00:00"
            }
          }
        }
      }
    })proto", &bundle));

  SequenceExample seqex;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(R"proto(
    context: {
      feature {
        key: "Patient.birthDate"
        value { int64_list { value: -1323388800 } }
      }
      feature {
        key: "currentEncounterId"
        value { int64_list { value: 1420444800 } }
      }
      feature {
        key: "patientId"
        value { bytes_list { value: "14" } }
      }
      feature {
        key: "sequenceLength"
        value { int64_list { value: 1 } }
      }
      feature {
        key: "timestamp"
        value { int64_list { value: 1420444800 } }
      }
    }
    feature_lists: {
      feature_list {
        key: "Encounter.meta.lastUpdated"
        value { feature { int64_list { value: 1420444800 } } }
      }
      feature_list {
        key: "Encounter.class"
        value { feature { bytes_list { value: "actcode:IMP" } } }
      }
      feature_list {
        key: "Encounter.period.end"
        value { feature { int64_list {} } }
      }
      feature_list {
        key: "Encounter.period.start"
        value { feature { int64_list { value: 1420444800 } } }
      }
      feature_list {
        key: "encounterId"
        value { feature { int64_list { value: 1420444800 } } }
      }
      feature_list {
        key: "eventId"
        value { feature { int64_list { value: 1420444800 } } }
      }
    })proto", &seqex));

  PerformTest("Patient/14", bundle, trigger_labels_pair,
              {{"Patient/14:0-1@1420444800:Encounter/1", seqex}});
}

TEST_F(BundleToSeqexConverterTest, TestDateTimeLabel) {
  EventTrigger trigger;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(R"proto(
    event_time { value_us: 1420444800000000 }
    source { encounter_id { value: "1" } }
  )proto", &trigger));
  stu3::google::EventLabel label;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(R"proto(
    type {
      system { value: "test_datetime_system" }
      code { value: "test_datetime_label" }
    }
    event_time { value_us: 1420444800000000 }
    label {
      class_value {
        date_time {
          value_us: 1515980100000000  # Monday, January 15, 2018 1:35:00 AM
          timezone: "UTC"
          precision: DAY
        }
      }
    }
  )proto", &label));
  std::vector<TriggerLabelsPair> trigger_labels_pair({{trigger, {label}}});
  Bundle bundle;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(R"proto(
    entry {
      resource {
        patient {
          id { value: "14" }
          birth_date {
            value_us: -1323388800000000
            precision: DAY
            timezone: "America/New_York"
          }
        }
      }
    }
    entry {
      resource {
        encounter {
          id { value: "1" }
          subject { patient_id { value: "14" } }
          class_value {
            system { value: "http://hl7.org/fhir/v3/ActCode" }
            code { value: "IMP" }
          }
          period {
            start {
              value_us: 1420444800000000  # "2015-01-05T08:00:00+00:00"
            }
            end {
              value_us: 1420455600000000  # "2015-01-05T11:00:00+00:00"
            }
          }
        }
      }
    })proto", &bundle));

  SequenceExample seqex;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(R"proto(
    context: {
      feature {
        key: "Patient.birthDate"
        value { int64_list { value: -1323388800 } }
      }
      feature {
        key: "currentEncounterId"
        value { int64_list { value: 1420444800 } }
      }
      feature {
        key: "patientId"
        value { bytes_list { value: "14" } }
      }
      feature {
        key: "sequenceLength"
        value { int64_list { value: 1 } }
      }
      feature {
        key: "timestamp"
        value { int64_list { value: 1420444800 } }
      }
      feature {
        key: "label.test_datetime_label.class"
        value {}
      }
      feature {
        key: "label.test_datetime_label.timestamp_secs"
        value { int64_list { value: 1420444800 } }
      }
      feature {
        key: "label.test_datetime_label.value_datetime_secs"
        value { int64_list { value: 1515980100 } }
      }
    }
    feature_lists: {
      feature_list {
        key: "Encounter.meta.lastUpdated"
        value { feature { int64_list { value: 1420444800 } } }
      }
      feature_list {
        key: "Encounter.class"
        value { feature { bytes_list { value: "actcode:IMP" } } }
      }
      feature_list {
        key: "Encounter.period.end"
        value { feature { int64_list {} } }
      }
      feature_list {
        key: "Encounter.period.start"
        value { feature { int64_list { value: 1420444800 } } }
      }
      feature_list {
        key: "encounterId"
        value { feature { int64_list { value: 1420444800 } } }
      }
      feature_list {
        key: "eventId"
        value { feature { int64_list { value: 1420444800 } } }
      }
    })proto", &seqex));

  PerformTest("Patient/14", bundle, trigger_labels_pair,
              {{"Patient/14:0-1@1420444800:Encounter/1", seqex}});
}

TEST_F(BundleToSeqexConverterTest, RedactedFeatures) {
  // We redact the icd9 flavor features for Encounter.reason, but
  // keep the main Encounter.reason feature for test purposes.
  FLAGS_trigger_time_redacted_features = "Encounter.reason.icd9";

  EventTrigger trigger;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(R"proto(
    event_time {
      value_us: 1420102800000000
      precision: SECOND
      timezone: "America/New_York"
    }
    source { encounter_id { value: "1" } }
  )proto", &trigger));
  std::vector<google::fhir::seqex::TriggerLabelsPair> trigger_labels_pair(
      {{trigger, {}}});
  Bundle bundle;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(R"proto(
    entry {
      resource {
        patient {
          id { value: "14" }
          birth_date {
            value_us: -1323388800000000
            precision: DAY
            timezone: "America/New_York"
          }
        }
      }
    }
    entry {
      resource {
        encounter {
          id { value: "1" }
          subject { patient_id { value: "14" } }
          class_value {
            system { value: "http://hl7.org/fhir/v3/ActCode" }
            code { value: "IMP" }
          }
          reason {
            coding {
              system { value: "http://hl7.org/fhir/sid/icd-9-cm/diagnosis" }
              code { value: "V410.9" }
              display { value: "Standard issue" }
            }
          }
          period {
            start {
              value_us: 1417420800000000  # "2014-12-01T08:00:00+00:00"
            }
            end {
              value_us: 1417424400000000  # "2014-12-01T09:00:00+00:00"
            }
          }
        }
      }
    }
    entry {
      resource {
        encounter {
          id { value: "2" }
          subject { patient_id { value: "14" } }
          class_value {
            system { value: "http://hl7.org/fhir/v3/ActCode" }
            code { value: "IMP" }
          }
          reason {
            coding {
              system { value: "http://hl7.org/fhir/sid/icd-9-cm/diagnosis" }
              code { value: "191.4" }
              display { value: "Malignant neoplasm of occipital lobe" }
            }
          }
          period {
            start {
              value_us: 1420099200000000  # "2015-01-01T08:00:00+00:00"
            }
            end {
              value_us: 1420102800000000  # "2015-01-01T09:00:00+00:00"
            }
          }
        }
      }
    })proto", &bundle));

  SequenceExample seqex;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(R"proto(
    context: {
      feature {
        key: "Patient.birthDate"
        value { int64_list { value: -1323388800 } }
      }
      feature {
        key: "currentEncounterId"
        value { int64_list { value: 1420099200 } }
      }
      feature {
        key: "patientId"
        value { bytes_list { value: "14" } }
      }
      feature {
        key: "sequenceLength"
        value { int64_list { value: 4 } }
      }
      feature {
        key: "timestamp"
        value { int64_list { value: 1420102800 } }
      }
    }
    feature_lists: {
      feature_list: {
        key: "Encounter.meta.lastUpdated"
        value {
          feature { int64_list { value: 1417420800 } }
          feature { int64_list { value: 1417424400 } }
          feature { int64_list { value: 1420099200 } }
          feature { int64_list { value: 1420102800 } }
        }
      }
      feature_list {
        key: "Encounter.class"
        value {
          feature { bytes_list { value: "actcode:IMP" } }
          feature { bytes_list { value: "actcode:IMP" } }
          feature { bytes_list { value: "actcode:IMP" } }
          feature { bytes_list { value: "actcode:IMP" } }
        }
      }
      feature_list {
        key: "Encounter.period.end"
        value {
          feature { int64_list {} }
          feature { int64_list { value: 1417424400 } }
          feature { int64_list {} }
          feature { int64_list { value: 1420102800 } }
        }
      }
      feature_list {
        key: "Encounter.period.start"
        value {
          feature { int64_list { value: 1417420800 } }
          feature { int64_list { value: 1417420800 } }
          feature { int64_list { value: 1420099200 } }
          feature { int64_list { value: 1420099200 } }
        }
      }
      feature_list {
        key: "Encounter.reason"
        value {
          feature { bytes_list {} }
          feature { bytes_list { value: "icd9:V410.9" } }
          feature { bytes_list {} }
          feature { bytes_list { value: "icd9:191.4" } }
        }
      }
      feature_list {
        key: "Encounter.reason.icd9"
        value {
          feature { bytes_list {} }
          feature { bytes_list { value: "V410.9" } }
          feature { bytes_list {} }
          feature { bytes_list {} }
        }
      }
      feature_list {
        key: "Encounter.reason.icd9.display.tokenized"
        value {
          feature { bytes_list {} }
          feature { bytes_list { value: "standard" value: "issue" } }
          feature { bytes_list {} }
          feature { bytes_list {} }
        }
      }
      feature_list {
        key: "encounterId"
        value {
          feature { int64_list { value: 1417420800 } }
          feature { int64_list { value: 1417420800 } }
          feature { int64_list { value: 1420099200 } }
          feature { int64_list { value: 1420099200 } }
        }
      }
      feature_list {
        key: "eventId"
        value {
          feature { int64_list { value: 1417420800 } }
          feature { int64_list { value: 1417424400 } }
          feature { int64_list { value: 1420099200 } }
          feature { int64_list { value: 1420102800 } }
        }
      }
    })proto", &seqex));

  PerformTest("Patient/14", bundle, trigger_labels_pair,
              {{"Patient/14:0-4@1420102800:Encounter/1", seqex}});
}

TEST_F(BundleToSeqexConverterTest, JoinMedication) {
  EventTrigger trigger;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(R"proto(
    event_time {
      value_us: 1420102800000000
      precision: SECOND
      timezone: "America/New_York"
    }
  )proto", &trigger));
  std::vector<google::fhir::seqex::TriggerLabelsPair> trigger_labels_pair(
      {{trigger, {}}});
  Bundle bundle;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(R"proto(
    entry {
      resource {
        patient {
          id { value: "14" }
          birth_date {
            value_us: -1323388800000000
            precision: DAY
            timezone: "America/New_York"
          }
        }
      }
    }
    entry {
      resource {
        encounter {
          id { value: "1" }
          subject { patient_id { value: "14" } }
          period {
            start {
              value_us: 1420099200000000  # "2015-01-01T08:00:00+00:00"
            }
            end {
              value_us: 1420102800000000  # "2015-01-01T09:00:00+00:00"
            }
          }
        }
      }
    }
    entry {
      resource {
        medication_request {
          id { value: "1" }
          subject { patient_id { value: "14" } }
          authored_on {
            value_us: 1420102700000000  # "2015-01-01T07:00:00+00:00"
          }
          medication { reference { medication_id { value: "med" } } }
          contained {
            medication {
              id { value: "med" }
              code {
                coding {
                  system { value: "http://hl7.org/fhir/sid/ndc" }
                  code { value: "123" }
                }
              }
            }
          }
        }
      }
    }
    entry {
      resource {
        medication {
          id { value: "med" }
          code {
            coding {
              system { value: "http://hl7.org/fhir/sid/ndc" }
              code { value: "123" }
            }
          }
        }
      }
    })proto", &bundle));

  SequenceExample seqex;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(R"proto(
    context: {
      feature {
        key: "Patient.birthDate"
        value { int64_list { value: -1323388800 } }
      }
      feature {
        key: "currentEncounterId"
        value { int64_list { value: 1420099200 } }
      }
      feature {
        key: "patientId"
        value { bytes_list { value: "14" } }
      }
      feature {
        key: "sequenceLength"
        value { int64_list { value: 3 } }
      }
      feature {
        key: "timestamp"
        value { int64_list { value: 1420102800 } }
      }
    }
    feature_lists: {
      feature_list {
        key: "Encounter.meta.lastUpdated"
        value {
          feature { int64_list { value: 1420099200 } }
          feature { int64_list {} }
          feature { int64_list { value: 1420102800 } }
        }
      }
      feature_list {
        key: "Encounter.period.end"
        value {
          feature { int64_list {} }
          feature { int64_list {} }
          feature { int64_list { value: 1420102800 } }
        }
      }
      feature_list {
        key: "Encounter.period.start"
        value {
          feature { int64_list { value: 1420099200 } }
          feature { int64_list {} }
          feature { int64_list { value: 1420099200 } }
        }
      }
      feature_list {
        key: "MedicationRequest.meta.lastUpdated"
        value {
          feature { int64_list {} }
          feature { int64_list { value: 1420102700 } }
          feature { int64_list {} }
        }
      }
      feature_list {
        key: "MedicationRequest.contained.medication.code.ndc"
        value {
          feature { bytes_list {} }
          feature { bytes_list { value: "123" } }
          feature { bytes_list {} }
        }
      }
      feature_list {
        key: "MedicationRequest.authoredOn"
        value {
          feature { int64_list {} }
          feature { int64_list { value: 1420102700 } }
          feature { int64_list {} }
        }
      }
      feature_list {
        key: "eventId"
        value {
          feature { int64_list { value: 1420099200 } }
          feature { int64_list { value: 1420102700 } }
          feature { int64_list { value: 1420102800 } }
        }
      }
      feature_list {
        key: "MedicationRequest.contained.medication.code"
        value {
          feature { bytes_list {} }
          feature { bytes_list { value: "ndc:123" } }
          feature { bytes_list {} }
        }
      }
      feature_list {
        key: "encounterId"
        value {
          feature { int64_list { value: 1420099200 } }
          feature { int64_list { value: 1420099200 } }
          feature { int64_list { value: 1420099200 } }
        }
      }
    })proto", &seqex));

  PerformTest("Patient/14", bundle, trigger_labels_pair,
              {{"Patient/14:0-3@1420102800", seqex}});
}

TEST_F(BundleToSeqexConverterTest, EmptyLabel) {
  EventTrigger trigger;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(R"proto(
    event_time {
      value_us: 1420102800000000
      precision: SECOND
      timezone: "America/New_York"
    }
  )proto", &trigger));
  std::vector<google::fhir::seqex::TriggerLabelsPair> trigger_labels_pair(
      {{trigger, {}}});
  Bundle bundle;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(R"proto(
    entry {
      resource {
        patient {
          id { value: "14" }
          birth_date {
            value_us: -1323388800000000
            precision: DAY
            timezone: "America/New_York"
          }
        }
      }
    }
    entry {
      resource {
        encounter {
          id { value: "1" }
          subject { patient_id { value: "14" } }
          period {
            start {
              value_us: 1420099200000000  # "2015-01-01T08:00:00+00:00"
            }
            end {
              value_us: 1420102800000000  # "2015-01-01T09:00:00+00:00"
            }
          }
        }
      }
    }
    entry {
      resource {
        medication_request {
          id { value: "1" }
          subject { patient_id { value: "14" } }
          authored_on {
            value_us: 1420100000000000
          }
          medication { reference { medication_id { value: "med" } } }
          contained {
            medication {
              id { value: "med" }
              code {
                coding {
                  system { value: "http://hl7.org/fhir/sid/ndc" }
                  code { value: "123" }
                }
              }
            }
          }
        }
      }
    }
    entry {
      resource {
        medication {
          id { value: "med" }
          code {
            coding {
              system { value: "http://hl7.org/fhir/sid/ndc" }
              code { value: "123" }
            }
          }
        }
      }
    })proto", &bundle));
  SequenceExample seqex;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(R"proto(
    context: {
      feature: {
        key: "Patient.birthDate"
        value { int64_list { value: -1323388800 } }
      }
      feature {
        key: "currentEncounterId"
        value { int64_list { value: 1420099200 } }
      }
      feature {
        key: "patientId"
        value { bytes_list { value: "14" } }
      }
      feature {
        key: "sequenceLength"
        value { int64_list { value: 3 } }
      }
      feature {
        key: "timestamp"
        value { int64_list { value: 1420102800 } }
      }
    }
    feature_lists: {
      feature_list {
        key: "Encounter.meta.lastUpdated"
        value {
          feature { int64_list { value: 1420099200 } }
          feature { int64_list {} }
          feature { int64_list { value: 1420102800 } }
        }
      }
      feature_list {
        key: "Encounter.period.end"
        value {
          feature { int64_list {} }
          feature { int64_list {} }
          feature { int64_list { value: 1420102800 } }
        }
      }
      feature_list {
        key: "Encounter.period.start"
        value {
          feature { int64_list { value: 1420099200 } }
          feature { int64_list {} }
          feature { int64_list { value: 1420099200 } }
        }
      }
      feature_list {
        key: "MedicationRequest.meta.lastUpdated"
        value {
          feature { int64_list {} }
          feature { int64_list { value: 1420100000 } }
          feature { int64_list {} }
        }
      }
      feature_list {
        key: "MedicationRequest.contained.medication.code.ndc"
        value {
          feature { bytes_list {} }
          feature { bytes_list { value: "123" } }
          feature { bytes_list {} }
        }
      }
      feature_list {
        key: "MedicationRequest.authoredOn"
        value {
          feature { int64_list {} }
          feature { int64_list { value: 1420100000 } }
          feature { int64_list {} }
        }
      }
      feature_list {
        key: "eventId"
        value {
          feature { int64_list { value: 1420099200 } }
          feature { int64_list { value: 1420100000 } }
          feature { int64_list { value: 1420102800 } }
        }
      }
      feature_list {
        key: "MedicationRequest.contained.medication.code"
        value {
          feature { bytes_list {} }
          feature { bytes_list { value: "ndc:123" } }
          feature { bytes_list {} }
        }
      }
      feature_list {
        key: "encounterId"
        value {
          feature { int64_list { value: 1420099200 } }
          feature { int64_list { value: 1420099200 } }
          feature { int64_list { value: 1420099200 } }
        }
      }
    })proto", &seqex));

  PerformTest("Patient/14", bundle, trigger_labels_pair,
              {{"Patient/14:0-3@1420102800", seqex}});
}

TEST_F(BundleToSeqexConverterTest, TwoExamples) {
  // We redact the icd9 flavor features for Encounter.reason, but
  // keep the main Encounter.reason feature for test purposes.
  FLAGS_trigger_time_redacted_features = "Encounter.reason.icd9";

  EventTrigger trigger1;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(R"proto(
    event_time { value_us: 1417424400000000 }
    source { encounter_id { value: "1" } }
  )proto", &trigger1));
  EventTrigger trigger2;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(R"proto(
    event_time {
      value_us: 1420102800000000
      precision: SECOND
      timezone: "America/New_York"
    }
    source { encounter_id { value: "2" } }
  )proto", &trigger2));
  std::vector<google::fhir::seqex::TriggerLabelsPair> trigger_labels_pair(
      {{trigger1, {}}, {trigger2, {}}});
  Bundle bundle;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(R"proto(
    entry {
      resource {
        patient {
          id { value: "14" }
          birth_date {
            value_us: -1323388800000000
            precision: DAY
            timezone: "America/New_York"
          }
        }
      }
    }
    entry {
      resource {
        encounter {
          id { value: "1" }
          subject { patient_id { value: "14" } }
          class_value {
            system { value: "http://hl7.org/fhir/v3/ActCode" }
            code { value: "IMP" }
          }
          reason {
            coding {
              system { value: "http://hl7.org/fhir/sid/icd-9-cm/diagnosis" }
              code { value: "V410.9" }
              display { value: "Standard issue" }
            }
          }
          period {
            start {
              value_us: 1417420800000000  # "2014-12-01T08:00:00+00:00"
            }
            end {
              value_us: 1417424400000000  # "2014-12-01T09:00:00+00:00"
            }
          }
        }
      }
    }
    entry {
      resource {
        encounter {
          id { value: "2" }
          subject { patient_id { value: "14" } }
          class_value {
            system { value: "http://hl7.org/fhir/v3/ActCode" }
            code { value: "IMP" }
          }
          reason {
            coding {
              system { value: "http://hl7.org/fhir/sid/icd-9-cm/diagnosis" }
              code { value: "191.4" }
              display { value: "Malignant neoplasm of occipital lobe" }
            }
          }
          period {
            start {
              value_us: 1420099200000000  # "2015-01-01T08:00:00+00:00"
            }
            end {
              value_us: 1420102800000000  # "2015-01-01T09:00:00+00:00"
            }
          }
        }
      }
    })proto", &bundle));

  SequenceExample seqex1;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(R"proto(
    context: {
      feature {
        key: "Patient.birthDate"
        value { int64_list { value: -1323388800 } }
      }
      feature {
        key: "currentEncounterId"
        value { int64_list { value: 1417420800 } }
      }
      feature {
        key: "patientId"
        value { bytes_list { value: "14" } }
      }
      feature {
        key: "sequenceLength"
        value { int64_list { value: 2 } }
      }
      feature {
        key: "timestamp"
        value { int64_list { value: 1417424400 } }
      }
    }
    feature_lists: {
      feature_list {
        key: "Encounter.meta.lastUpdated"
        value {
          feature { int64_list { value: 1417420800 } }
          feature { int64_list { value: 1417424400 } }
        }
      }
      feature_list {
        key: "Encounter.class"
        value {
          feature { bytes_list { value: "actcode:IMP" } }
          feature { bytes_list { value: "actcode:IMP" } }
        }
      }
      feature_list {
        key: "Encounter.period.end"
        value {
          feature { int64_list {} }
          feature { int64_list { value: 1417424400 } }
        }
      }
      feature_list {
        key: "Encounter.period.start"
        value {
          feature { int64_list { value: 1417420800 } }
          feature { int64_list { value: 1417420800 } }
        }
      }
      feature_list {
        key: "Encounter.reason"
        value {
          feature { bytes_list {} }
          feature { bytes_list { value: "icd9:V410.9" } }
        }
      }
      feature_list {
        key: "Encounter.reason.icd9"
        value {
          feature { bytes_list {} }
          feature { bytes_list {} }
        }
      }
      feature_list {
        key: "Encounter.reason.icd9.display.tokenized"
        value {
          feature { bytes_list {} }
          feature { bytes_list {} }
        }
      }
      feature_list {
        key: "encounterId"
        value {
          feature { int64_list { value: 1417420800 } }
          feature { int64_list { value: 1417420800 } }
        }
      }
      feature_list {
        key: "eventId"
        value {
          feature { int64_list { value: 1417420800 } }
          feature { int64_list { value: 1417424400 } }
        }
      }
    })proto", &seqex1));

  SequenceExample seqex2;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(R"proto(
    context: {
      feature {
        key: "Patient.birthDate"
        value { int64_list { value: -1323388800 } }
      }
      feature {
        key: "currentEncounterId"
        value { int64_list { value: 1420099200 } }
      }
      feature {
        key: "patientId"
        value { bytes_list { value: "14" } }
      }
      feature {
        key: "sequenceLength"
        value { int64_list { value: 4 } }
      }
      feature {
        key: "timestamp"
        value { int64_list { value: 1420102800 } }
      }
    }
    feature_lists: {
      feature_list {
        key: "Encounter.meta.lastUpdated"
        value {
          feature { int64_list { value: 1417420800 } }
          feature { int64_list { value: 1417424400 } }
          feature { int64_list { value: 1420099200 } }
          feature { int64_list { value: 1420102800 } }
        }
      }
      feature_list {
        key: "Encounter.class"
        value {
          feature { bytes_list { value: "actcode:IMP" } }
          feature { bytes_list { value: "actcode:IMP" } }
          feature { bytes_list { value: "actcode:IMP" } }
          feature { bytes_list { value: "actcode:IMP" } }
        }
      }
      feature_list {
        key: "Encounter.period.end"
        value {
          feature { int64_list {} }
          feature { int64_list { value: 1417424400 } }
          feature { int64_list {} }
          feature { int64_list { value: 1420102800 } }
        }
      }
      feature_list {
        key: "Encounter.period.start"
        value {
          feature { int64_list { value: 1417420800 } }
          feature { int64_list { value: 1417420800 } }
          feature { int64_list { value: 1420099200 } }
          feature { int64_list { value: 1420099200 } }
        }
      }
      feature_list {
        key: "Encounter.reason"
        value {
          feature { bytes_list {} }
          feature { bytes_list { value: "icd9:V410.9" } }
          feature { bytes_list {} }
          feature { bytes_list { value: "icd9:191.4" } }
        }
      }
      feature_list {
        key: "Encounter.reason.icd9"
        value {
          feature { bytes_list {} }
          feature { bytes_list { value: "V410.9" } }
          feature { bytes_list {} }
          feature { bytes_list {} }
        }
      }
      feature_list {
        key: "Encounter.reason.icd9.display.tokenized"
        value {
          feature { bytes_list {} }
          feature { bytes_list { value: "standard" value: "issue" } }
          feature { bytes_list {} }
          feature { bytes_list {} }
        }
      }
      feature_list {
        key: "encounterId"
        value {
          feature { int64_list { value: 1417420800 } }
          feature { int64_list { value: 1417420800 } }
          feature { int64_list { value: 1420099200 } }
          feature { int64_list { value: 1420099200 } }
        }
      }
      feature_list {
        key: "eventId"
        value {
          feature { int64_list { value: 1417420800 } }
          feature { int64_list { value: 1417424400 } }
          feature { int64_list { value: 1420099200 } }
          feature { int64_list { value: 1420102800 } }
        }
      }
    })proto", &seqex2));

  BundleToSeqexConverter converter(fhir_version_config_,
                                   false /* enable_attribution */,
                                   false /*generate_sequence_label */);
  std::map<string, int> counter_stats;
  ASSERT_TRUE(converter.Begin("Patient/14", bundle, trigger_labels_pair,
                              &counter_stats));
  ASSERT_FALSE(converter.Done());
  EXPECT_EQ("a8c128978feaab69-Patient/14:0-2@1417424400:Encounter/1",
            converter.ExampleKeyWithPrefix());
  EXPECT_THAT(seqex1, EqualsProto(converter.GetExample()));
  ASSERT_TRUE(converter.Next());
  ASSERT_FALSE(converter.Done());
  EXPECT_EQ("a87dd8b5f6221497-Patient/14:0-4@1420102800:Encounter/2",
            converter.ExampleKeyWithPrefix());
  EXPECT_THAT(seqex2, EqualsProto(converter.GetExample()));
  ASSERT_TRUE(converter.Next());
  ASSERT_TRUE(converter.Done());
}

TEST_F(BundleToSeqexConverterTest, TwoExamples_EnableAttribution) {
  // We redact the icd9 flavor features for Encounter.reason, but
  // keep the main Encounter.reason feature for test purposes.
  FLAGS_trigger_time_redacted_features = "Encounter.reason.icd9";

  EventTrigger trigger1;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(R"proto(
    event_time { value_us: 1417424400000000 }
    source { encounter_id { value: "1" } }
  )proto", &trigger1));
  EventTrigger trigger2;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(R"proto(
    event_time {
      value_us: 1420102800000000
      precision: SECOND
      timezone: "America/New_York"
    }
    source { encounter_id { value: "2" } }
  )proto", &trigger2));
  std::vector<google::fhir::seqex::TriggerLabelsPair> trigger_labels_pair(
      {{trigger1, {}}, {trigger2, {}}});
  Bundle bundle;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(R"proto(
    entry {
      resource {
        patient {
          id { value: "14" }
          birth_date {
            value_us: -1323388800000000
            precision: DAY
            timezone: "America/New_York"
          }
        }
      }
    }
    entry {
      resource {
        encounter {
          id { value: "1" }
          subject { patient_id { value: "14" } }
          class_value {
            system { value: "http://hl7.org/fhir/v3/ActCode" }
            code { value: "IMP" }
          }
          reason {
            coding {
              system { value: "http://hl7.org/fhir/sid/icd-9-cm/diagnosis" }
              code { value: "V410.9" }
              display { value: "Standard issue" }
            }
          }
          period {
            start {
              value_us: 1417420800000000  # "2014-12-01T08:00:00+00:00"
            }
            end {
              value_us: 1417424400000000  # "2014-12-01T09:00:00+00:00"
            }
          }
        }
      }
    }
    entry {
      resource {
        encounter {
          id { value: "2" }
          subject { patient_id { value: "14" } }
          class_value {
            system { value: "http://hl7.org/fhir/v3/ActCode" }
            code { value: "IMP" }
          }
          reason {
            coding {
              system { value: "http://hl7.org/fhir/sid/icd-9-cm/diagnosis" }
              code { value: "191.4" }
              display { value: "Malignant neoplasm of occipital lobe" }
            }
          }
          period {
            start {
              value_us: 1420099200000000  # "2015-01-01T08:00:00+00:00"
            }
            end {
              value_us: 1420102800000000  # "2015-01-01T09:00:00+00:00"
            }
          }
        }
      }
    })proto", &bundle));

  SequenceExample seqex1;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(R"proto(
    context: {
      feature {
        key: "Patient.birthDate"
        value { int64_list { value: -1323388800 } }
      }
      feature {
        key: "currentEncounterId"
        value { int64_list { value: 1417420800 } }
      }
      feature {
        key: "patientId"
        value { bytes_list { value: "14" } }
      }
      feature {
        key: "sequenceLength"
        value { int64_list { value: 2 } }
      }
      feature {
        key: "timestamp"
        value { int64_list { value: 1417424400 } }
      }
    }
    feature_lists: {
      feature_list {
        key: "Encounter.meta.lastUpdated"
        value {
          feature { int64_list { value: 1417420800 } }
          feature { int64_list { value: 1417424400 } }
        }
      }
      feature_list {
        key: "Encounter.class"
        value {
          feature { bytes_list { value: "actcode:IMP" } }
          feature { bytes_list { value: "actcode:IMP" } }
        }
      }
      feature_list {
        key: "Encounter.period.end"
        value {
          feature { int64_list {} }
          feature { int64_list { value: 1417424400 } }
        }
      }
      feature_list {
        key: "Encounter.period.start"
        value {
          feature { int64_list { value: 1417420800 } }
          feature { int64_list { value: 1417420800 } }
        }
      }
      feature_list {
        key: "Encounter.reason"
        value {
          feature { bytes_list {} }
          feature { bytes_list { value: "icd9:V410.9" } }
        }
      }
      feature_list {
        key: "Encounter.reason.icd9"
        value {
          feature { bytes_list {} }
          feature { bytes_list {} }
        }
      }
      feature_list {
        key: "Encounter.reason.icd9.display.tokenized"
        value {
          feature { bytes_list {} }
          feature { bytes_list {} }
        }
      }
      feature_list {
        key: "Encounter.reason.icd9.display.token_start"
        value {
          feature { int64_list {} }
          feature { int64_list {} }
        }
      }
      feature_list {
        key: "Encounter.reason.icd9.display.token_end"
        value {
          feature { int64_list {} }
          feature { int64_list {} }
        }
      }
      feature_list {
        key: "encounterId"
        value {
          feature { int64_list { value: 1417420800 } }
          feature { int64_list { value: 1417420800 } }
        }
      }
      feature_list {
        key: "eventId"
        value {
          feature { int64_list { value: 1417420800 } }
          feature { int64_list { value: 1417424400 } }
        }
      }
      feature_list {
        key: "resourceId"
        value {
          feature { bytes_list { value: "Encounter/1" } }
          feature { bytes_list { value: "Encounter/1" } }
        }
      }
    })proto", &seqex1));

  SequenceExample seqex2;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(R"proto(
    context: {
      feature {
        key: "Patient.birthDate"
        value { int64_list { value: -1323388800 } }
      }
      feature {
        key: "currentEncounterId"
        value { int64_list { value: 1420099200 } }
      }
      feature {
        key: "patientId"
        value { bytes_list { value: "14" } }
      }
      feature {
        key: "sequenceLength"
        value { int64_list { value: 4 } }
      }
      feature {
        key: "timestamp"
        value { int64_list { value: 1420102800 } }
      }
    }
    feature_lists: {
      feature_list {
        key: "Encounter.meta.lastUpdated"
        value {
          feature { int64_list { value: 1417420800 } }
          feature { int64_list { value: 1417424400 } }
          feature { int64_list { value: 1420099200 } }
          feature { int64_list { value: 1420102800 } }
        }
      }
      feature_list {
        key: "Encounter.class"
        value {
          feature { bytes_list { value: "actcode:IMP" } }
          feature { bytes_list { value: "actcode:IMP" } }
          feature { bytes_list { value: "actcode:IMP" } }
          feature { bytes_list { value: "actcode:IMP" } }
        }
      }
      feature_list {
        key: "Encounter.period.end"
        value {
          feature { int64_list {} }
          feature { int64_list { value: 1417424400 } }
          feature { int64_list {} }
          feature { int64_list { value: 1420102800 } }
        }
      }
      feature_list {
        key: "Encounter.period.start"
        value {
          feature { int64_list { value: 1417420800 } }
          feature { int64_list { value: 1417420800 } }
          feature { int64_list { value: 1420099200 } }
          feature { int64_list { value: 1420099200 } }
        }
      }
      feature_list {
        key: "Encounter.reason"
        value {
          feature { bytes_list {} }
          feature { bytes_list { value: "icd9:V410.9" } }
          feature { bytes_list {} }
          feature { bytes_list { value: "icd9:191.4" } }
        }
      }
      feature_list {
        key: "Encounter.reason.icd9"
        value {
          feature { bytes_list {} }
          feature { bytes_list { value: "V410.9" } }
          feature { bytes_list {} }
          feature { bytes_list {} }
        }
      }
      feature_list {
        key: "Encounter.reason.icd9.display.tokenized"
        value {
          feature { bytes_list {} }
          feature { bytes_list { value: "standard" value: "issue" } }
          feature { bytes_list {} }
          feature { bytes_list {} }
        }
      }
      feature_list {
        key: "Encounter.reason.icd9.display.token_start"
        value {
          feature { int64_list {} }
          feature { int64_list { value: 0 value: 9 } }
          feature { int64_list {} }
          feature { int64_list {} }
        }
      }
      feature_list {
        key: "Encounter.reason.icd9.display.token_end"
        value {
          feature { int64_list {} }
          feature { int64_list { value: 8 value: 14 } }
          feature { int64_list {} }
          feature { int64_list {} }
        }
      }
      feature_list {
        key: "encounterId"
        value {
          feature { int64_list { value: 1417420800 } }
          feature { int64_list { value: 1417420800 } }
          feature { int64_list { value: 1420099200 } }
          feature { int64_list { value: 1420099200 } }
        }
      }
      feature_list {
        key: "eventId"
        value {
          feature { int64_list { value: 1417420800 } }
          feature { int64_list { value: 1417424400 } }
          feature { int64_list { value: 1420099200 } }
          feature { int64_list { value: 1420102800 } }
        }
      }
      feature_list {
        key: "resourceId"
        value {
          feature { bytes_list { value: "Encounter/1" } }
          feature { bytes_list { value: "Encounter/1" } }
          feature { bytes_list { value: "Encounter/2" } }
          feature { bytes_list { value: "Encounter/2" } }
        }
      }
    })proto", &seqex2));

  BundleToSeqexConverter converter(fhir_version_config_,
                                   true /* enable_attribution */,
                                   false /* generate_sequence_label */);
  std::map<string, int> counter_stats;
  ASSERT_TRUE(converter.Begin("Patient/14", bundle, trigger_labels_pair,
                              &counter_stats));
  ASSERT_FALSE(converter.Done());
  EXPECT_EQ("a8c128978feaab69-Patient/14:0-2@1417424400:Encounter/1",
            converter.ExampleKeyWithPrefix());
  EXPECT_THAT(seqex1, EqualsProto(converter.GetExample()));
  ASSERT_TRUE(converter.Next());
  ASSERT_FALSE(converter.Done());
  EXPECT_EQ("a87dd8b5f6221497-Patient/14:0-4@1420102800:Encounter/2",
            converter.ExampleKeyWithPrefix());
  EXPECT_THAT(seqex2, EqualsProto(converter.GetExample()));
  ASSERT_TRUE(converter.Next());
  ASSERT_TRUE(converter.Done());
}

}  // namespace seqex
}  // namespace fhir
}  // namespace google
