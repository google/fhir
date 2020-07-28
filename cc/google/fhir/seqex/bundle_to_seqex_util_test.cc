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

#include "google/fhir/seqex/bundle_to_seqex_util.h"

#include <memory>

#include "google/protobuf/text_format.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "absl/strings/substitute.h"
#include "google/fhir/seqex/r4.h"
#include "google/fhir/seqex/stu3.h"
#include "google/fhir/testutil/fhir_test_env.h"
#include "google/fhir/testutil/proto_matchers.h"

using ::google::fhir::testutil::EqualsProto;
using ::testing::ElementsAre;
using ::testing::Pair;
using ::testing::UnorderedElementsAre;

namespace google {
namespace fhir {
namespace seqex {

namespace {

template <typename TestEnv>
class BundleToSeqexUtilTest : public ::testing::Test {};

struct Stu3TestEnv : public testutil::Stu3CoreTestEnv,
                     public seqex_stu3::ConverterTypes {
  using ConverterTypes = seqex_stu3::ConverterTypes;
  constexpr static auto& condition_recorded_date_field() {
    return "assertedDate";
  }
};

struct R4TestEnv : public testutil::R4CoreTestEnv,
                   public seqex_r4::ConverterTypes {
  using ConverterTypes = seqex_r4::ConverterTypes;
  constexpr static auto& condition_recorded_date_field() {
    return "recordedDate";
  }
};

using TestEnvs = ::testing::Types<Stu3TestEnv, R4TestEnv>;
TYPED_TEST_SUITE(BundleToSeqexUtilTest, TestEnvs);

TYPED_TEST(BundleToSeqexUtilTest, GetTriggerLabelsPairFromInputLabels) {
  typename TypeParam::EventLabel input_labels1;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(R"proto(
    patient { patient_id { value: "14" } }
    type { code { value: "test1" } }
    event_time { value_us: 1417392000000000 }  # "2014-12-01T00:00:00+00:00"
    source { encounter_id { value: "1" } }
  )proto", &input_labels1));
  typename TypeParam::EventLabel input_labels2;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(R"proto(
    patient { patient_id { value: "14" } }
    type { code { value: "test2" } }
    event_time { value_us: 1417392000000000 }  # "2014-12-01T00:00:00+00:00"
  )proto", &input_labels2));
  typename TypeParam::EventLabel input_labels3;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(R"proto(
    patient { patient_id { value: "14" } }
    type { code { value: "test2" } }
    event_time { value_us: 1417428000000000 }  # "2014-12-01T01:00:00+00:00"
    label {
      class_name {
        system { value: "urn:test:label" }
        code { value: "green" }
      }
    }
  )proto", &input_labels3));
  typename TypeParam::EventTrigger trigger1;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(R"proto(
    event_time { value_us: 1417392000000000 }  # "2014-12-01T00:00:00+00:00"
    source { encounter_id { value: "1" } }
  )proto", &trigger1));
  typename TypeParam::EventTrigger trigger2;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(R"proto(
    event_time { value_us: 1417428000000000 }  # "2014-12-01T01:00:00+00:00"
  )proto", &trigger2));

  typename TypeParam::EventLabel label1;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(R"proto(
    patient { patient_id { value: "14" } }
    type { code { value: "test1" } }
    event_time { value_us: 1417392000000000 }  # "2014-12-01T00:00:00+00:00"
    source { encounter_id { value: "1" } }
  )proto", &label1));
  typename TypeParam::EventLabel label2;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(R"proto(
    patient { patient_id { value: "14" } }
    type { code { value: "test2" } }
    event_time { value_us: 1417392000000000 }  # "2014-12-01T00:00:00+00:00"
  )proto", &label2));
  typename TypeParam::EventLabel label3;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(R"proto(
    patient { patient_id { value: "14" } }
    type { code { value: "test2" } }
    event_time { value_us: 1417428000000000 }  # "2014-12-01T01:00:00+00:00"
    label {
      class_name {
        system { value: "urn:test:label" }
        code { value: "green" }
      }
    }
  )proto", &label3));
  std::vector<typename TypeParam::TriggerLabelsPair> got;
  GetTriggerLabelsPairFromInputLabels<typename TypeParam::ConverterTypes>(
      {input_labels1, input_labels2, input_labels3}, &got);
  EXPECT_EQ(2, got.size());
  EXPECT_THAT(
      got,
      UnorderedElementsAre(
          Pair(EqualsProto(trigger1),
               UnorderedElementsAre(EqualsProto(label1), EqualsProto(label2))),
          Pair(EqualsProto(trigger2),
               UnorderedElementsAre(EqualsProto(label3)))));
}

TYPED_TEST(BundleToSeqexUtilTest, GetTriggerLabelsPair_NoLabel) {
  const std::set<std::string> label_names({"test1"});
  typename TypeParam::Bundle bundle;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(
      absl::Substitute(
          R"proto(
            entry { resource { patient { id { value: "14" } } } }
            entry {
              resource {
                encounter {
                  id { value: "1" }
                  extension {
                    url {
                      value: "https://g.co/fhir/StructureDefinition/eventTrigger"
                    }
                    extension {
                      url { value: "type" }
                      value {
                        coding {
                          system { value: "urn:test:trigger" }
                          code { value: "at_discharge" }
                        }
                      }
                    }
                    extension {
                      url { value: "eventTime" }
                      value {
                        date_time {
                          value_us:
                              1388566800000000  # "2014-01-01T09:00:00+00:00"
                        }
                      }
                    }
                  }
                }
              }
            }
            entry {
              resource {
                encounter {
                  id { value: "2" }
                  extension {
                    url {
                      value: "https://g.co/fhir/StructureDefinition/eventTrigger"
                    }
                    extension {
                      url { value: "type" }
                      value {
                        coding {
                          system { value: "urn:test:trigger" }
                          code { value: "at_discharge" }
                        }
                      }
                    }
                    extension {
                      url { value: "eventTime" }
                      value {
                        date_time {
                          value_us:
                              1420102800000000  # "2015-01-01T09:00:00+00:00"
                        }
                      }
                    }
                  }
                  extension {
                    url {
                      value: "https://g.co/fhir/StructureDefinition/eventLabel"
                    }
                    extension {
                      url { value: "type" }
                      value {
                        coding {
                          system { value: "urn:test:label" }
                          code { value: "green" }
                        }
                      }
                    }
                    extension {
                      url { value: "eventTime" }
                      value { date_time { value_us: 1420102800000000 } }
                    }
                    extension {
                      url { value: "source" }
                      value { reference { encounter_id { value: "2" } } }
                    }
                    extension {
                      url { value: "label" }
                      extension {
                        url { value: "className" }
                        value {
                          coding {
                            system { value: "urn:test:label" }
                            code { value: "green" }
                          }
                        }
                      }
                    }
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
                      system {
                        value: "http://hl7.org/fhir/sid/icd-9-cm/diagnosis"
                      }
                      code { value: "bar" }
                    }
                  }
                  $0 {
                    value_us: 1417392000000000  # "2014-12-01T00:00:00+00:00"
                  }
                }
              }
            })proto",
          ToSnakeCase(TypeParam::condition_recorded_date_field())),
      &bundle));
  typename TypeParam::EventTrigger trigger1;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(R"proto(
    type {
      system { value: "urn:test:trigger" }
      code { value: "at_discharge" }
    }
    event_time { value_us: 1388566800000000 }  # "2014-01-01T09:00:00+00:00"
  )proto", &trigger1));
  typename TypeParam::EventTrigger trigger2;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(R"proto(
    type {
      system { value: "urn:test:trigger" }
      code { value: "at_discharge" }
    }
    event_time { value_us: 1420102800000000 }  # "2015-01-01T09:00:00+00:00"
  )proto", &trigger2));
  std::vector<typename TypeParam::TriggerLabelsPair>
      got_trigger_labels_pair_vector;
  int num_triggers_filtered = 0;
  GetTriggerLabelsPair<typename TypeParam::ConverterTypes>(
      bundle, label_names, "at_discharge", &got_trigger_labels_pair_vector,
      &num_triggers_filtered);
  EXPECT_THAT(got_trigger_labels_pair_vector,
              UnorderedElementsAre(Pair(EqualsProto(trigger1), ElementsAre()),
                                   Pair(EqualsProto(trigger2), ElementsAre())));
  EXPECT_EQ(0, num_triggers_filtered);
}

TYPED_TEST(BundleToSeqexUtilTest, GetTriggerLabelsPair_WithLabels) {
  const std::set<std::string> label_names({"test1"});
  typename TypeParam::Bundle bundle;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(R"proto(
    entry { resource { patient { id { value: "20" } } } }
    entry {
      resource {
        encounter {
          id { value: "1" }
          extension {
            url { value: "https://g.co/fhir/StructureDefinition/eventTrigger" }
            extension {
              url { value: "type" }
              value {
                coding {
                  system { value: "urn:test:trigger" }
                  code { value: "at_discharge" }
                }
              }
            }
            extension {
              url { value: "eventTime" }
              value {
                date_time {
                  value_us: 1388566800000000  # "2014-01-01T09:00:00+00:00"
                }
              }
            }
          }
          extension {
            url { value: "https://g.co/fhir/StructureDefinition/eventLabel" }
            extension {
              url { value: "type" }
              value {
                coding {
                  system { value: "urn:test:label" }
                  code { value: "test1" }
                }
              }
            }
            extension {
              url { value: "eventTime" }
              value { date_time { value_us: 1388566800000000 } }
            }
            extension {
              url { value: "source" }
              value { reference { encounter_id { value: "1" } } }
            }
          }
        }
      }
    }
    entry {
      resource {
        encounter {
          id { value: "2" }
          extension {
            url { value: "https://g.co/fhir/StructureDefinition/eventTrigger" }
            extension {
              url { value: "type" }
              value {
                coding {
                  system { value: "urn:test:trigger" }
                  code { value: "at_discharge" }
                }
              }
            }
            extension {
              url { value: "eventTime" }
              value {
                date_time {
                  value_us: 1420102800000000  # "2015-01-01T09:00:00+00:00"
                }
              }
            }
          }
          extension {
            url { value: "https://g.co/fhir/StructureDefinition/eventLabel" }
            extension {
              url { value: "type" }
              value {
                coding {
                  system { value: "urn:test:label" }
                  code { value: "test1" }
                }
              }
            }
            extension {
              url { value: "source" }
              value { reference { encounter_id { value: "2" } } }
            }
            extension {
              url { value: "label" }
              extension {
                url { value: "className" }
                value {
                  coding {
                    system { value: "urn:test:label:test1" }
                    code { value: "red" }
                  }
                }
              }
            }
          }
        }
      }
    }
  )proto", &bundle));
  typename TypeParam::EventTrigger trigger1;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(R"proto(
    type {
      system { value: "urn:test:trigger" }
      code { value: "at_discharge" }
    }
    event_time { value_us: 1388566800000000 }  # "2014-01-01T09:00:00+00:00"
  )proto", &trigger1));
  // Missing class means false binary label.
  typename TypeParam::EventLabel label1;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(R"proto(
    type {
      system { value: "urn:test:label" }
      code { value: "test1" }
    }
    event_time { value_us: 1388566800000000 }
    source { encounter_id { value: "1" } }
  )proto", &label1));
  typename TypeParam::EventTrigger trigger2;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(R"proto(
    type {
      system { value: "urn:test:trigger" }
      code { value: "at_discharge" }
    }
    event_time { value_us: 1420102800000000 }  # "2015-01-01T09:00:00+00:00"
  )proto", &trigger2));
  typename TypeParam::EventLabel label2;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(R"proto(
    type {
      system { value: "urn:test:label" }
      code { value: "test1" }
    }
    source { encounter_id { value: "2" } }
    label {
      class_name {
        system { value: "urn:test:label:test1" }
        code { value: "red" }
      }
    }
  )proto", &label2));
  std::vector<typename TypeParam::TriggerLabelsPair>
      got_trigger_labels_pair_vector;
  int num_triggers_filtered = 0;
  GetTriggerLabelsPair<typename TypeParam::ConverterTypes>(
      bundle, label_names, "at_discharge", &got_trigger_labels_pair_vector,
      &num_triggers_filtered);
  EXPECT_THAT(
      got_trigger_labels_pair_vector,
      UnorderedElementsAre(
          Pair(EqualsProto(trigger1), ElementsAre(EqualsProto(label1))),
          Pair(EqualsProto(trigger2), ElementsAre(EqualsProto(label2)))));
  EXPECT_EQ(0, num_triggers_filtered);
}

TYPED_TEST(BundleToSeqexUtilTest, GetTriggerLabelsPair_TriggerFiltered) {
  const std::set<std::string> label_names({"test2"});
  // The trigger would be filtered due to label time before trigger time.
  typename TypeParam::Bundle bundle;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(R"proto(
    entry { resource { patient { id { value: "40" } } } }
    entry {
      resource {
        encounter {
          id { value: "41" }
          extension {
            url { value: "https://g.co/fhir/StructureDefinition/eventTrigger" }
            extension {
              url { value: "type" }
              value {
                coding {
                  system { value: "urn:test:trigger" }
                  code { value: "at_discharge" }
                }
              }
            }
            extension {
              url { value: "eventTime" }
              value { date_time { value_us: 1388566900000000 } }
            }
          }
          extension {
            url { value: "https://g.co/fhir/StructureDefinition/eventLabel" }
            extension {
              url { value: "type" }
              value {
                coding {
                  system { value: "urn:test:label" }
                  code { value: "test2" }
                }
              }
            }
            extension {
              url { value: "eventTime" }
              value { date_time { value_us: 1388566800000000 } }
            }
            extension {
              url { value: "source" }
              value { reference { encounter_id { value: "41" } } }
            }
          }
        }
      }
    }
  )proto", &bundle));

  std::vector<typename TypeParam::TriggerLabelsPair>
      got_trigger_labels_pair_vector;
  int num_triggers_filtered = 0;
  GetTriggerLabelsPair<typename TypeParam::ConverterTypes>(
      bundle, label_names, "at_discharge", &got_trigger_labels_pair_vector,
      &num_triggers_filtered);
  EXPECT_TRUE(got_trigger_labels_pair_vector.empty());
  EXPECT_EQ(1, num_triggers_filtered);
}

TYPED_TEST(BundleToSeqexUtilTest, GetTriggerLabelsPair_WithMultipleTriggers) {
  const std::set<std::string> label_names({"test1"});
  typename TypeParam::Bundle bundle;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(R"proto(
    entry { resource { patient { id { value: "20" } } } }
    entry {
      resource {
        encounter {
          id { value: "1" }
          extension {
            url { value: "https://g.co/fhir/StructureDefinition/eventTrigger" }
            extension {
              url { value: "type" }
              value {
                coding {
                  system { value: "urn:test:trigger" }
                  code { value: "at_7am" }
                }
              }
            }
            extension {
              url { value: "eventTime" }
              value {
                date_time {
                  value_us: 1388473200000000  # "2013-12-31T07:00:00+00:00"
                }
              }
            }
            extension {
              url { value: "source" }
              value { reference { encounter_id { value: "1" } } }
            }
          }
          extension {
            url { value: "https://g.co/fhir/StructureDefinition/eventTrigger" }
            extension {
              url { value: "type" }
              value {
                coding {
                  system { value: "urn:test:trigger" }
                  code { value: "at_7am" }
                }
              }
            }
            extension {
              url { value: "eventTime" }
              value {
                date_time {
                  value_us: 1388559600000000  # "2014-01-01T07:00:00+00:00"
                }
              }
            }
            extension {
              url { value: "source" }
              value { reference { encounter_id { value: "1" } } }
            }
          }
          extension {
            url { value: "https://g.co/fhir/StructureDefinition/eventLabel" }
            extension {
              url { value: "type" }
              value {
                coding {
                  system { value: "urn:test:label" }
                  code { value: "test1" }
                }
              }
            }
            extension {
              url { value: "eventTime" }
              value {
                date_time { value_us: 1388566800000000 }
              }  # "2014-01-01T09:00:00+00:00"
            }
            extension {
              url { value: "source" }
              value { reference { encounter_id { value: "1" } } }
            }
          }
        }
      }
    }
  )proto", &bundle));
  typename TypeParam::EventTrigger trigger1;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(R"proto(
    type {
      system { value: "urn:test:trigger" }
      code { value: "at_7am" }
    }
    event_time {
      value_us: 1388473200000000  # "2013-12-31T07:00:00+00:00"
    }
    source { encounter_id { value: "1" } }
  )proto", &trigger1));
  typename TypeParam::EventTrigger trigger2;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(R"proto(
    type {
      system { value: "urn:test:trigger" }
      code { value: "at_7am" }
    }
    event_time { value_us: 1388559600000000 }  # "2014-01-01T07:00:00+00:00"
    source { encounter_id { value: "1" } }
  )proto", &trigger2));
  // Missing class means false binary label.
  typename TypeParam::EventLabel label1;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(R"proto(
    type {
      system { value: "urn:test:label" }
      code { value: "test1" }
    }
    event_time { value_us: 1388566800000000 }  # "2014-01-01T09:00:00+00:00"
    source { encounter_id { value: "1" } }
  )proto", &label1));
  std::vector<typename TypeParam::TriggerLabelsPair>
      got_trigger_labels_pair_vector;
  int num_triggers_filtered = 0;
  GetTriggerLabelsPair<typename TypeParam::ConverterTypes>(
      bundle, label_names, "at_7am", &got_trigger_labels_pair_vector,
      &num_triggers_filtered);
  EXPECT_THAT(
      got_trigger_labels_pair_vector,
      UnorderedElementsAre(
          Pair(EqualsProto(trigger1), ElementsAre(EqualsProto(label1))),
          Pair(EqualsProto(trigger2), ElementsAre(EqualsProto(label1)))));
  EXPECT_EQ(0, num_triggers_filtered);
}

TYPED_TEST(BundleToSeqexUtilTest, ExtractEventLabelProtoFromBundle) {
  const std::set<std::string> label_names({"test1"});
  typename TypeParam::Bundle bundle;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(R"proto(
    entry { resource { patient { id { value: "14" } } } }
    entry {
      resource {
        encounter {
          id { value: "2" }
          extension {
            url { value: "https://g.co/fhir/StructureDefinition/eventLabel" }
            extension {
              url { value: "type" }
              value {
                coding {
                  system { value: "urn:test:label" }
                  code { value: "test1" }
                }
              }
            }
            extension {
              url { value: "eventTime" }
              value { date_time { value_us: 1420102800000000 } }
            }
            extension {
              url { value: "source" }
              value { reference { encounter_id { value: "2" } } }
            }
            extension {
              url { value: "label" }
              extension {
                url { value: "className" }
                value {
                  coding {
                    system { value: "urn:test:label:test1" }
                    code { value: "green" }
                  }
                }
              }
            }
          }
        }
      }
    }
  )proto", &bundle));
  typename TypeParam::EventLabel label;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(R"proto(
    type {
      system { value: "urn:test:label" }
      code { value: "test1" }
    }
    event_time { value_us: 1420102800000000 }
    source { encounter_id { value: "2" } }
    label {
      class_name {
        system { value: "urn:test:label:test1" }
        code { value: "green" }
      }
    }
  )proto", &label));
  EXPECT_THAT(ExtractLabelsFromBundle<typename TypeParam::ConverterTypes>(
                  bundle, {"test1"}),
              UnorderedElementsAre(EqualsProto(label)));
}
}  // namespace

}  // namespace seqex
}  // namespace fhir
}  // namespace google
