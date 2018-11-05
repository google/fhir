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

"""Tests for label."""

from __future__ import absolute_import
from __future__ import division
from __future__ import print_function
from datetime import datetime
import os

from absl.testing import absltest
from absl.testing import parameterized
from google.protobuf import text_format
from proto.stu3 import datatypes_pb2
from proto.stu3 import google_extensions_pb2
from proto.stu3 import resources_pb2
from py.google.fhir.labels import label


_TESTDATA_PATH = 'com_google_fhir/testdata/stu3/labels'


class LabelTest(parameterized.TestCase):

  def setUp(self):
    self._test_data_dir = os.path.join(absltest.get_default_test_srcdir(),
                                       _TESTDATA_PATH)
    self._enc = resources_pb2.Encounter()
    with open(os.path.join(self._test_data_dir, 'encounter_1.pbtxt')) as f:
      text_format.Parse(f.read(), self._enc)
    self._patient = resources_pb2.Patient()
    self._patient.id.value = 'Patient/1'

    self._expected_label = google_extensions_pb2.EventLabel()
    with open(os.path.join(self._test_data_dir, 'label_1.pbtxt')) as f:
      text_format.Parse(f.read(), self._expected_label)

  def testExtractCodeBySystem(self):
    sample_codeable_concept = datatypes_pb2.CodeableConcept()
    text_format.Merge('''
    coding: {
      system: {
        value    : "urn:sample_system"
      }
      code: {
        value    : "sample_code"
      }
    }
    coding: {
      system: {
        value    : "urn:unrelated_system"
      }
      code: {
        value    : "unrelated_code"
      }
    }
    ''', sample_codeable_concept)

    code = label.ExtractCodeBySystem(sample_codeable_concept,
                                     'urn:sample_system')
    self.assertEqual('sample_code', code)

  def testComposeLabel(self):
    output_label = label.ComposeLabel(self._patient, self._enc,
                                      'label.test', 'true',
                                      datetime(2003, 1, 2, 4, 5, 6))
    self.assertEqual(self._expected_label, output_label)

  @parameterized.parameters(
      # February 27, 2009 11:31:31 PM UTC
      {'end_us': 1235777491000000, 'label_val': 'above_14'},
      # February 27, 2009 11:31:30 PM UTC
      {'end_us': 1235777490000000, 'label_val': '7_14'},
      # February 20, 2009 11:31:31 PM UTC
      {'end_us': 1235172691000000, 'label_val': '7_14'},
      # February 20, 2009 11:30:30 PM UTC
      {'end_us': 1235172690000000, 'label_val': '3_7'},
      # February 16, 2009 11:31:30 PM UTC
      {'end_us': 1234827091000000, 'label_val': '3_7'},
      # February 26, 2009 11:30:30 PM UTC
      {'end_us': 1234827090000000, 'label_val': 'less_or_equal_3'}
  )
  def testLengthOfStayRangeAt24Hours(self, end_us, label_val):
    enc = resources_pb2.Encounter()
    enc.CopyFrom(self._enc)
    enc.period.end.value_us = end_us
    labels = [l for l in label.LengthOfStayRangeAt24Hours(
        self._patient, enc)]
    expected_label = label.ComposeLabel(
        self._patient, self._enc,
        label.LOS_RANGE_LABEL,
        label_val,
        # 24 hours after admission
        datetime(2009, 2, 14, 23, 31, 30))
    self.assertEqual([expected_label], labels)

  def testLengthOfStayRangeAt24HoursLT24Hours(self):
    enc = resources_pb2.Encounter()
    enc.CopyFrom(self._enc)
    enc.period.end.value_us = 1234567891000000
    with self.assertRaises(AssertionError):
      _ = [l for l in label.LengthOfStayRangeAt24Hours(
          self._patient, enc)]


if __name__ == '__main__':
  absltest.main()
