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

"""Tests for bundle_to_seqex_util."""

from __future__ import absolute_import
from __future__ import division
from __future__ import print_function

from absl.testing import absltest
from google.protobuf import text_format
from proto.stu3 import google_extensions_pb2
from proto.stu3 import resources_pb2
from py.google.fhir.seqex import bundle_to_seqex_util
from py.google.fhir.testutil import protobuf_compare


class BundleToSeqexUtilTest(protobuf_compare.ProtoAssertions,
                            absltest.TestCase):

  def test_get_trigger_labels_from_input_labels(self):
    trigger1 = text_format.Parse(
        """
      event_time { value_us: 1417392000000000 } # "2014-12-01T00:00:00+00:00"
      source { encounter_id { value: "1" } }
    """, google_extensions_pb2.EventTrigger())
    trigger2 = text_format.Parse(
        """
      event_time { value_us: 1417428000000000 } # "2014-12-01T01:00:00+00:00"
    """, google_extensions_pb2.EventTrigger())
    label1 = text_format.Parse(
        """
      patient { patient_id { value: "14" } }
      type { code { value: "test1" } }
      event_time { value_us: 1417392000000000 } # "2014-12-01T00:00:00+00:00"
      source { encounter_id { value: "1" } }
    """, google_extensions_pb2.EventLabel())
    label2 = text_format.Parse(
        """
      patient { patient_id { value: "14" } }
      type { code { value: "test2" } }
      event_time { value_us: 1417428000000000 } # "2014-12-01T01:00:00+00:00"
      label { class_name {
        system { value: "urn:test:label" }
        code { value: "green " }
      } }
    """, google_extensions_pb2.EventLabel())
    got_list = bundle_to_seqex_util.get_trigger_labels_from_input_labels(
        [label1, label2])
    self.assertEqual(2, len(got_list))
    (got_trigger_1, got_label_list1) = got_list[0]
    (got_trigger_2, got_label_list2) = got_list[1]
    self.assertProtoEqual(got_trigger_1, trigger1)
    self.assertEqual(1, len(got_label_list1))
    self.assertProtoEqual(got_label_list1[0], label1)
    self.assertProtoEqual(got_trigger_2, trigger2)
    self.assertEqual(1, len(got_label_list2))
    self.assertProtoEqual(got_label_list2[0], label2)

  def test_get_trigger_labels_pair(self):
    # pylint: disable=line-too-long
    bundle = text_format.Parse(
        """
      entry { resource { patient {
        id { value: "20" }
      } } }
      entry { resource { encounter {
        id { value: "1" }
        extension {
          url { value: "https://g.co/fhir/StructureDefinition/eventTrigger" }
          extension {
            url { value: "type" }
            value { coding {
              system { value: "urn:test:trigger"}
              code { value: "at_discharge" }
            } }
          }
          extension {
            url { value: "eventTime" }
            value { date_time {
              value_us: 1388566800000000  # "2014-01-01T09:00:00+00:00"
            } }
          }
        }
        extension {
          url { value: "https://g.co/fhir/StructureDefinition/eventLabel" }
          extension {
            url { value: "type" }
            value { coding {
              system { value: "urn:test:label" }
              code { value: "test1" }
            } }
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
      } } }
      entry { resource { encounter {
        id { value: "2" }
        extension {
          url { value: "https://g.co/fhir/StructureDefinition/eventTrigger" }
          extension {
            url { value: "type" }
            value { coding {
              system { value: "urn:test:trigger"}
              code { value: "at_discharge" }
            } }
          }
          extension {
            url { value: "eventTime" }
            value { date_time {
              value_us: 1420102800000000  # "2015-01-01T09:00:00+00:00"
            } }
          }
        }
        extension {
          url { value: "https://g.co/fhir/StructureDefinition/eventLabel" }
          extension {
            url { value: "type" }
            value { coding {
              system { value: "urn:test:label" }
              code { value: "test1" }
            } }
          }
          extension {
            url { value: "source" }
            value { reference { encounter_id { value: "2" } } }
          }
          extension {
            url { value: "label" }
            extension {
              url { value: "className" }
              value { coding {
                system { value: "urn:test:label:test1" }
                code { value: "red" }
              } }
            }
          }
        }
      } } }
    """, resources_pb2.Bundle())
    trigger1 = text_format.Parse(
        """
      type {
        system { value: "urn:test:trigger" }
        code { value: "at_discharge" }
      }
      event_time { value_us: 1388566800000000 }  # "2014-01-01T09:00:00+00:00"
    """, google_extensions_pb2.EventTrigger())
    trigger2 = text_format.Parse(
        """
      type {
        system { value: "urn:test:trigger" }
        code { value: "at_discharge" }
      }
      event_time { value_us: 1420102800000000 }  # "2015-01-01T09:00:00+00:00"
    """, google_extensions_pb2.EventTrigger())
    label1 = text_format.Parse(
        """
      type {
        system { value: "urn:test:label" }
        code { value: "test1" }
      }
      event_time { value_us: 1388566800000000 }
      source { encounter_id { value: "1" } }
    """, google_extensions_pb2.EventLabel())
    label2 = text_format.Parse(
        """
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
    """, google_extensions_pb2.EventLabel())
    # pylint: enable=line-too-long

    (filtered_count, got_list) = bundle_to_seqex_util.get_trigger_labels_pair(
        bundle, ["test1"], "at_discharge")
    self.assertEqual(0, filtered_count)
    self.assertEqual(2, len(got_list))
    (got_trigger_1, got_label_list1) = got_list[0]
    (got_trigger_2, got_label_list2) = got_list[1]
    self.assertProtoEqual(got_trigger_1, trigger1)
    self.assertEqual(1, len(got_label_list1))
    self.assertProtoEqual(got_label_list1[0], label1)
    self.assertProtoEqual(got_trigger_2, trigger2)
    self.assertEqual(1, len(got_label_list2))
    self.assertProtoEqual(got_label_list2[0], label2)


if __name__ == "__main__":
  absltest.main()
