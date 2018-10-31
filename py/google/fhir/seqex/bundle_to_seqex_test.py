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

"""Tests for bundle_to_seqex."""

from __future__ import absolute_import
from __future__ import division
from __future__ import print_function

import os

from absl.testing import absltest
import apache_beam as beam
from apache_beam.testing import test_pipeline
from apache_beam.testing import util
from google.protobuf import text_format
from proto.stu3 import google_extensions_pb2
from proto.stu3 import resources_pb2
from proto.stu3 import version_config_pb2
from py.google.fhir.seqex import bundle_to_seqex
from py.google.fhir.testutil import protobuf_compare
from tensorflow.core.example import example_pb2

_VERSION_CONFIG_PATH = "com_google_fhir/proto/stu3/version_config.textproto"


class BundleToSeqexTest(protobuf_compare.ProtoAssertions, absltest.TestCase):

  def setUp(self):
    with open(
        os.path.join(absltest.get_default_test_srcdir(),
                     _VERSION_CONFIG_PATH)) as f:
      self._version_config = text_format.Parse(
          f.read(), version_config_pb2.VersionConfig())

  def testKeyBundleByPatientId(self):
    bundle = text_format.Parse(
        """
      entry { resource { patient {
        id { value: "14" }
      } } }""", resources_pb2.Bundle())
    with test_pipeline.TestPipeline() as p:
      result = (
          p
          | "CreateBundles" >> beam.Create([bundle])
          | "KeyBundleByPatientId" >> beam.ParDo(
              bundle_to_seqex.KeyBundleByPatientIdFn()))

      def check_result(got):
        try:
          self.assertEqual(1, len(got))
          (got_key, got_bundle) = got[0]
          self.assertEqual("Patient/14", got_key)
          self.assertProtoEqual(got_bundle, bundle)

        except AssertionError as err:
          raise util.BeamAssertException(err)

      util.assert_that(result, check_result)

  def testKeyEventLabelByPatientIdFn(self):
    event_label = text_format.Parse(
        """
      patient { patient_id { value: "14" } }
      type { code { value: "test2" } }
      event_time { value_us: 1417428000000000 } # "2014-12-01T01:00:00+00:00"
      label { class_name {
        system { value: "urn:test:label" }
        code { value: "green" }
      } }
    """, google_extensions_pb2.EventLabel())
    with test_pipeline.TestPipeline() as p:
      result = (
          p
          | "CreateEventLabels" >> beam.Create([event_label])
          | "KeyEventLabelByPatientId" >> beam.ParDo(
              bundle_to_seqex.KeyEventLabelByPatientIdFn()))

      def check_result(got):
        try:
          self.assertEqual(1, len(got))
          (got_key, got_event_label) = got[0]
          self.assertEqual("Patient/14", got_key)
          self.assertProtoEqual(got_event_label, event_label)

        except AssertionError as err:
          raise util.BeamAssertException(err)

      util.assert_that(result, check_result)

  def testCreateTriggerLabelsPairLists(self):
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
        code { value: "green" }
      } }
    """, google_extensions_pb2.EventLabel())
    with test_pipeline.TestPipeline() as p:
      event_labels_pcoll = (
          p | "CreateEventLabels" >> beam.Create([label1, label2]))
      result = bundle_to_seqex.CreateTriggerLabelsPairLists(event_labels_pcoll)

      def check_result(got):
        try:
          self.assertEqual(1, len(got))
          (got_key, got_trigger_labels_pairs_list) = got[0]
          self.assertEqual("Patient/14", got_key)
          self.assertEqual(2, len(got_trigger_labels_pairs_list))
          # Sort got_trigger_labels_pairs_list by trigger.event_time, so that
          # the ordering is always consistent in ordering.
          sorted_list = sorted(
              got_trigger_labels_pairs_list,
              key=lambda x: x[0].event_time.value_us)
          (got_trigger1, got_label_list1) = sorted_list[0]
          self.assertProtoEqual(got_trigger1, trigger1)
          self.assertEqual(1, len(got_label_list1))
          self.assertProtoEqual(got_label_list1[0], label1)
          (got_trigger2, got_label_list2) = sorted_list[1]
          self.assertProtoEqual(got_trigger2, trigger2)
          self.assertEqual(1, len(got_label_list2))
          self.assertProtoEqual(got_label_list2[0], label2)

        except AssertionError as err:
          raise util.BeamAssertException(err)

      util.assert_that(result, check_result)

  def testCreateBundleAndLabels(self):
    bundle1 = text_format.Parse(
        """
      entry { resource { patient {
        id { value: "14" }
      } } }
      entry { resource { condition {
        id { value: "1" }
        subject { patient_id { value: "14" } }
        code {
          coding {
            system { value: "http://hl7.org/fhir/sid/icd-9-cm/diagnosis" }
            code { value: "bar" }
          }
        }
        asserted_date {
          value_us: 1417392000000000 # "2014-12-01T00:00:00+00:00"
        }
      } } }""", resources_pb2.Bundle())
    bundle1_event_trigger = text_format.Parse(
        """
      event_time { value_us: 1388566800000000 }  # "2014-01-01T09:00:00+00:00"
    """, google_extensions_pb2.EventTrigger())

    # For the purpose of testing, bundle2 does not exist.
    bundle2_event_trigger = text_format.Parse(
        """
      event_time { value_us: 1388566800000000 }  # "2014-01-01T09:00:00+00:00"
    """, google_extensions_pb2.EventTrigger())

    bundle3 = text_format.Parse(
        """
      entry { resource { patient {
        id { value: "30" }
      } } }""", resources_pb2.Bundle())
    bundle1_event_trigger_labels_list = [
        (
            bundle1_event_trigger,
            list(),
        ),
    ]
    bundle2_event_trigger_labels_list = [
        (
            bundle2_event_trigger,
            list(),
        ),
    ]
    with test_pipeline.TestPipeline() as p:
      bundle_pcoll = p | "CreateBundles" >> beam.Create([
          ("Patient/14", bundle1),
          ("Patient/30", bundle3),
      ])
      trigger_list_pcoll = p | "CreateTriggerLists" >> beam.Create([
          ("Patient/14", bundle1_event_trigger_labels_list),
          ("Patient/20", bundle2_event_trigger_labels_list),
      ])
      result = bundle_to_seqex.CreateBundleAndLabels(bundle_pcoll,
                                                     trigger_list_pcoll)

      def check_result(got):
        try:
          self.assertEqual(1, len(got))
          (got_key, got_bundle_and_labels) = got[0]
          self.assertEqual("Patient/14", got_key)
          (got_bundle, got_trigger_labels_list) = got_bundle_and_labels
          self.assertProtoEqual(got_bundle, bundle1)
          self.assertEqual(1, len(got_trigger_labels_list))
          self.assertProtoEqual(got_trigger_labels_list[0][0],
                                bundle1_event_trigger)
          self.assertFalse(len(got_trigger_labels_list[0][1]))

        except AssertionError as err:
          raise util.BeamAssertException(err)

      util.assert_that(result, check_result)

  def testBundleAndLabelsToSeqexDoFn(self):
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
    event_trigger_labels_list = [
        event_trigger_labels1,
        event_trigger_labels2,
    ]

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
    with test_pipeline.TestPipeline() as p:
      result = (
          p
          | beam.Create([("Patient/14", (bundle, event_trigger_labels_list))])
          | "BundleAndLabelsToSeqex" >> beam.ParDo(
              bundle_to_seqex.BundleAndLabelsToSeqexDoFn(
                  version_config=self._version_config,
                  enable_attribution=False,
                  generate_sequence_label=False)))

      def check_result(got):
        try:
          self.assertEqual(2, len(got), "got: %s" % got)
          got_seqex1 = got[0]
          got_seqex2 = got[1]
          self.assertProtoEqual(expected_seqex1, got_seqex1)
          self.assertProtoEqual(expected_seqex2, got_seqex2)
        except AssertionError as err:
          raise util.BeamAssertException(err)

      util.assert_that(result, check_result)


if __name__ == "__main__":
  absltest.main()
