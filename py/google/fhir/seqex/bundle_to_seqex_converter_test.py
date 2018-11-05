#
# Copyright 2018 Google LLC
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

"""Tests for bundle_to_seqex_converter python wrapper."""

from __future__ import absolute_import
from __future__ import division
from __future__ import print_function

import os

from absl import flags
from absl.testing import absltest
from google.protobuf import text_format
from proto.stu3 import google_extensions_pb2
from proto.stu3 import resources_pb2
from proto.stu3 import version_config_pb2
from py.google.fhir.seqex import bundle_to_seqex_converter
from py.google.fhir.testutil import protobuf_compare
from tensorflow.core.example import example_pb2

_VERSION_CONFIG_PATH = "com_google_fhir/proto/stu3/version_config.textproto"


class BundleToSeqexConverterTest(protobuf_compare.ProtoAssertions,
                                 absltest.TestCase):

  def setUp(self):
    with open(
        os.path.join(absltest.get_default_test_srcdir(),
                     _VERSION_CONFIG_PATH)) as f:
      self._version_config = text_format.Parse(
          f.read(), version_config_pb2.VersionConfig())

  def _runTest(self, patient_key, bundle, event_trigger_labels_list,
               expected_outcomes):
    converter = bundle_to_seqex_converter.PyBundleToSeqexConverter(
        self._version_config, False, False)
    (begin_result, stats) = converter.begin(patient_key, bundle,
                                            event_trigger_labels_list)
    self.assertTrue(begin_result)
    got_results = list()
    next_example_and_key = converter.get_next_example_and_key()
    while next_example_and_key:
      got_results.append(next_example_and_key)
      next_example_and_key = converter.get_next_example_and_key()
    self.assertEqual(len(expected_outcomes), len(got_results))
    for expected_outcome, got_result in zip(expected_outcomes, got_results):
      (expected_key, expected_seqex, expected_length) = expected_outcome
      (got_key, got_seqex, got_length) = got_result
      self.assertEqual(expected_key, got_key)
      self.assertProtoEqual(expected_seqex, got_seqex)
      self.assertEqual(expected_length, got_length)
    return stats

  def testSingleTriggerAndStats(self):
    event_trigger = text_format.Parse(
        """
      event_time { value_us: 1420102800000000 }
    """, google_extensions_pb2.EventTrigger())
    event_trigger_labels = (event_trigger, list())
    event_trigger_labels_list = (event_trigger_labels,)

    bundle = text_format.Parse(
        """
      entry { resource { patient {
        id { value: "14" }
        birth_date { value_us: 8640000000000 }
      } } }
      entry { resource { medication_request {
        id { value: "1" }
        subject { patient_id { value: "14" } }
        contained {
          medication {
            id { value: "med" }
            code { coding {
              system { value:
                "http://hl7.org/fhir/sid/ndc"
              }
              code { value: "123" }
            } }
          }
        }
        authored_on {
          value_us: 1420095600000000 # "2015-01-01T07:00:00+00:00"
        }
        medication { reference {
          medication_id { value: "med" }
        } }
      } } }
    """, resources_pb2.Bundle())
    expected_seqex = text_format.Parse(
        """
      context: {
        feature {
          key: "currentEncounterId"
          value { int64_list { value: 1420095600 } }
        }
        feature {
          key: "patientId"
          value { bytes_list { value: "14" } }
        }
        feature {
          key: "Patient.birthDate"
          value { int64_list { value: 8640000 } }
        }
        feature {
          key: "sequenceLength"
          value { int64_list { value: 1 } }
        }
        feature {
          key: "timestamp"
          value { int64_list { value: 1420102800 } }
        } }
        feature_lists: {
          feature_list {
            key: "MedicationRequest.contained.medication.code.ndc"
            value { feature { bytes_list { value: "123" } } }
          }
          feature_list {
            key: "MedicationRequest.meta.lastUpdated"
            value { feature { int64_list { value: 1420095600 } } }
          }
          feature_list {
            key: "MedicationRequest.authoredOn"
            value { feature { int64_list { value: 1420095600 } } }
          }
          feature_list {
            key: "eventId"
            value { feature { int64_list { value: 1420095600 } } }
          }
          feature_list {
            key: "MedicationRequest.contained.medication.code"
            value { feature { bytes_list { value: "ndc:123" } } }
          }
          feature_list {
            key: "encounterId"
            value { feature { int64_list { value: 1420095600 } } }
          }
        }
    """, example_pb2.SequenceExample())
    got_stats = self._runTest(
        "Patient/14", bundle, event_trigger_labels_list,
        (("Patient/14:0-1@1420102800", expected_seqex, 1),))
    # We only spot check a few counters here. The main goal is to make sure
    # we get the counters back from c++ land.
    self.assertEqual(1, got_stats.get("num-examples"))

  def testSingleTriggerAndLabel(self):
    event_trigger = text_format.Parse(
        """
      event_time { value_us: 1420102800000000 }
      source { encounter_id { value: "1" } }
    """, google_extensions_pb2.EventTrigger())
    event_label = text_format.Parse(
        """
      patient { patient_id { value: "14" } }
      type { code { value: "test1" }}
      event_time { value_us: 1420102800000000 }
      label {
        class_name {
          system { value: "urn:test:label"
          }
          code { value: "red" }
        }
      }
    """, google_extensions_pb2.EventLabel())
    event_trigger_labels = (
        event_trigger,
        (event_label,),
    )
    event_trigger_labels_list = (event_trigger_labels,)

    bundle = text_format.Parse(
        """
      entry { resource { patient {
        id { value: "14" }
      } } }
      entry { resource { condition {
        id { value: "1" }
        subject { patient_id { value: "14" } }
        code { coding {
          system { value: "http://hl7.org/fhir/sid/icd-9-cm/diagnosis" }
          code { value: "bar" }
        } }
        asserted_date {
          value_us: 1417392000000000 # "2014-12-01T00:00:00+00:00"
        }
      } } }
      entry { resource { condition {
        id { value: "2" }
        subject { patient_id { value: "14" } }
        code { coding {
          system { value: "http://hl7.org/fhir/sid/icd-9-cm/diagnosis" }
          code { value: "baz" }
        } }
        asserted_date {
          value_us: 1420099200000000 # "2015-01-01T08:00:00+00:00"
        }
      } } }
      entry { resource { composition {
        id { value: "1" }
        subject { patient_id { value: "14" } }
        encounter { encounter_id { value: "1" } }
        section { text { div { value: "test text" } } }
        date {
          value_us: 1420102800000000
          timezone: "UTC"
          precision: SECOND
        }
      } } }
      entry { resource { encounter {
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
            value_us: 1420099200000000 # "2015-01-01T08:00:00+00:00"
          }
          end {
            value_us: 1420102800000000 # "2015-01-01T09:00:00+00:00"
          }
        }
      } } }
    """, resources_pb2.Bundle())
    # pylint: disable=line-too-long
    expected_seqex = text_format.Parse(
        """
      context: {
        feature {
          key: "currentEncounterId"
          value { int64_list { value: 1420099200 } }
        }
        feature {
          key: "label.test1.class"
          value { bytes_list { value: "red" } }
        }
        feature {
          key: "label.test1.timestamp_secs"
          value { int64_list { value: 1420102800 } }
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
          key: "Composition.meta.lastUpdated"
          value { feature { int64_list { } } feature { int64_list { } } feature { int64_list { } } feature { int64_list { value: 1420102800 } } feature { int64_list { } } }
        }
        feature_list {
          key: "Composition.date"
          value { feature { int64_list { } } feature { int64_list { } } feature { int64_list { } } feature { int64_list { value: 1420102800 } } feature { int64_list { } } }
        }
        feature_list {
          key: "Composition.section.text.div.tokenized"
          value { feature { bytes_list { } } feature { bytes_list { } } feature { bytes_list { } } feature { bytes_list { value: "test" value: "text" } } feature { bytes_list { } } }
        }
        feature_list {
          key: "Condition.meta.lastUpdated"
          value { feature { int64_list { value: 1417392000 } } feature { int64_list { value: 1420099200 } } feature { int64_list { } } feature { int64_list { } } feature { int64_list { } } }
        }
        feature_list {
          key: "Condition.code"
          value { feature { bytes_list { value: "icd9:bar" } } feature { bytes_list { value: "icd9:baz" } } feature { bytes_list { } } feature { bytes_list { } } feature { bytes_list { } } }
        }
        feature_list {
          key: "Condition.code.icd9"
          value { feature { bytes_list { value: "bar" } } feature { bytes_list { value: "baz" } } feature { bytes_list { } } feature { bytes_list { } } feature { bytes_list { } } }
        }
        feature_list {
          key: "Condition.assertedDate"
          value { feature { int64_list { value: 1417392000 } } feature { int64_list { value: 1420099200 } } feature { int64_list { } } feature { int64_list { } } feature { int64_list { } } }
        }
        feature_list {
          key: "Encounter.class"
          value { feature { bytes_list { } } feature { bytes_list { } } feature { bytes_list { value: "actcode:IMP" } } feature { bytes_list { } } feature { bytes_list { value: "actcode:IMP" } } }
        }
        feature_list {
          key: "Encounter.meta.lastUpdated"
          value { feature { int64_list { } } feature { int64_list { } } feature { int64_list { value: 1420099200 } } feature { int64_list { } } feature { int64_list { value: 1420102800 } } }
        }
        feature_list {
          key: "Encounter.period.start"
          value { feature { int64_list { } } feature { int64_list { } } feature { int64_list { value: 1420099200 } } feature { int64_list { } } feature { int64_list { value: 1420099200 } } }
        }
        feature_list {
          key: "Encounter.period.end"
          value { feature { int64_list { } } feature { int64_list { } } feature { int64_list { } } feature { int64_list { } } feature { int64_list { value: 1420102800 } } }
        }
        feature_list {
          key: "Encounter.reason"
          value { feature { bytes_list { } } feature { bytes_list { } } feature { bytes_list { } } feature { bytes_list { } } feature { bytes_list { value: "icd9:191.4" } } }
        }
        feature_list {
          key: "Encounter.reason.icd9"
          value { feature { bytes_list { } } feature { bytes_list { } } feature { bytes_list { } } feature { bytes_list { } } feature { bytes_list { value: "191.4" } } }
        }
        feature_list {
          key: "Encounter.reason.icd9.display.tokenized"
          value { feature { bytes_list { } } feature { bytes_list { } } feature { bytes_list { } } feature { bytes_list { } } feature { bytes_list { value: "malignant" value: "neoplasm" value: "of" value: "occipital" value: "lobe" } } }
        }
        feature_list {
          key: "encounterId"
          value { feature { int64_list { value: 1417392000 } } feature { int64_list { value: 1420099200 } } feature { int64_list { value: 1420099200 } } feature { int64_list { value: 1420099200 } } feature { int64_list { value: 1420099200 } } }
        }
        feature_list {
          key: "eventId"
          value { feature { int64_list { value: 1417392000 } } feature { int64_list { value: 1420099200 } } feature { int64_list { value: 1420099200 } } feature { int64_list { value: 1420102800 } } feature { int64_list { value: 1420102800 } } }
        }
      }
    """, example_pb2.SequenceExample())
    # pylint: enable=line-too-long
    _ = self._runTest(
        "Patient/14", bundle, event_trigger_labels_list,
        (("Patient/14:0-5@1420102800:Encounter/1", expected_seqex, 5),))

  def testMultipleTriggersAndExamples(self):
    event_trigger1 = text_format.Parse(
        """
      event_time { value_us: 1417424400000000 }
      source { encounter_id { value: "1" } }
    """, google_extensions_pb2.EventTrigger())
    event_trigger2 = text_format.Parse(
        """
      event_time { value_us: 1420102800000000 }
      source { encounter_id { value: "2" } }
    """, google_extensions_pb2.EventTrigger())
    event_trigger_labels1 = (
        event_trigger1,
        list(),
    )
    event_trigger_labels2 = (
        event_trigger2,
        list(),
    )
    event_trigger_labels_list = (
        event_trigger_labels1,
        event_trigger_labels2,
    )

    bundle = text_format.Parse(
        """
      entry { resource { patient {
        id { value: "14" }
      } } }
      entry {
        resource { encounter {
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
              value_us: 1417420800000000 # "2014-12-01T08:00:00+00:00"
            }
            end {
              value_us: 1417424400000000 # "2014-12-01T09:00:00+00:00"
            }
          }
        } }
      }
      entry {
        resource { encounter {
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
              value_us: 1420099200000000 # "2015-01-01T08:00:00+00:00"
            }
            end {
              value_us: 1420102800000000 # "2015-01-01T09:00:00+00:00"
            }
          }
        } }
      }
    """, resources_pb2.Bundle())
    # pylint: disable=line-too-long
    expected_seqex1 = text_format.Parse(
        """
      context: {
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
          value { int64_list { value: 1417424400 } } }
        }
        feature_lists: {
          feature_list {
            key: "Encounter.class"
            value { feature { bytes_list { value: "actcode:IMP" } } feature { bytes_list { value: "actcode:IMP" } } }
          }
          feature_list {
            key: "Encounter.meta.lastUpdated"
            value { feature { int64_list { value: 1417420800 } } feature { int64_list { value: 1417424400 } } }
          }
          feature_list {
            key: "Encounter.period.start"
            value { feature { int64_list { value: 1417420800 } } feature { int64_list { value: 1417420800 } } }
          }
          feature_list {
            key: "Encounter.period.end"
            value { feature { int64_list { } } feature { int64_list { value: 1417424400 } } }
          }
          feature_list {
            key: "Encounter.reason"
            value { feature { bytes_list { } } feature { bytes_list { value: "icd9:V410.9" } } }
          }
          feature_list {
            key: "Encounter.reason.icd9"
            value { feature { bytes_list { } } feature { bytes_list { value: "V410.9" } } }
          }
          feature_list {
            key: "Encounter.reason.icd9.display.tokenized"
            value { feature { bytes_list { } } feature { bytes_list { value: "standard" value: "issue" } } }
          }
          feature_list {
            key: "encounterId"
            value { feature { int64_list { value: 1417420800 } } feature { int64_list { value: 1417420800 } } }
          }
          feature_list {
            key: "eventId"
            value { feature { int64_list { value: 1417420800 } } feature { int64_list { value: 1417424400 } } }
          }
        }
    """, example_pb2.SequenceExample())
    expected_seqex2 = text_format.Parse(
        """
      context: {
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
          value { int64_list { value: 1420102800 } } }
        }
        feature_lists: {
          feature_list {
            key: "Encounter.class"
            value {
              feature { bytes_list { value: "actcode:IMP" } } feature { bytes_list { value: "actcode:IMP" } } feature { bytes_list { value: "actcode:IMP" } } feature { bytes_list { value: "actcode:IMP" } }
            }
          }
          feature_list {
            key: "Encounter.meta.lastUpdated"
            value { feature { int64_list { value: 1417420800 } } feature { int64_list { value: 1417424400 } } feature { int64_list { value: 1420099200 } } feature { int64_list { value: 1420102800 } } }
          }
          feature_list {
            key: "Encounter.period.start"
            value {
              feature { int64_list { value: 1417420800 } } feature { int64_list { value: 1417420800 } } feature { int64_list { value: 1420099200 } } feature { int64_list { value: 1420099200 } }
            }
          }
          feature_list {
            key: "Encounter.period.end"
            value {
              feature { int64_list { } } feature { int64_list { value: 1417424400 } } feature { int64_list { } } feature { int64_list { value: 1420102800 } }
            }
          }
          feature_list {
            key: "Encounter.reason"
            value {
              feature { bytes_list { } } feature { bytes_list { value: "icd9:V410.9" } } feature { bytes_list { } } feature { bytes_list { value: "icd9:191.4" } }
            }
          }
          feature_list {
            key: "Encounter.reason.icd9"
            value {
              feature { bytes_list { } } feature { bytes_list { value: "V410.9" } } feature { bytes_list { } } feature { bytes_list { value: "191.4" } }
            }
          }
          feature_list {
            key: "Encounter.reason.icd9.display.tokenized"
            value {
              feature { bytes_list { } } feature { bytes_list { value: "standard" value: "issue" } } feature { bytes_list { } } feature { bytes_list { value: "malignant" value: "neoplasm" value: "of" value: "occipital" value: "lobe" } }
            }
          }
          feature_list {
            key: "encounterId"
            value {
              feature { int64_list { value: 1417420800 } } feature { int64_list { value: 1417420800 } } feature { int64_list { value: 1420099200 } } feature { int64_list { value: 1420099200 } }
            }
          }
          feature_list {
            key: "eventId"
            value {
              feature { int64_list { value: 1417420800 } } feature { int64_list { value: 1417424400 } } feature { int64_list { value: 1420099200 } } feature { int64_list { value: 1420102800 } }
            }
          }
        }
    """, example_pb2.SequenceExample())
    # pylint: enable=line-too-long
    _ = self._runTest("Patient/14", bundle, event_trigger_labels_list, (
        ("Patient/14:0-2@1417424400:Encounter/1", expected_seqex1, 2),
        ("Patient/14:0-4@1420102800:Encounter/2", expected_seqex2, 4),
    ))


if __name__ == "__main__":
  absltest.main()
