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

"""Tests for encounter."""

from __future__ import absolute_import
from __future__ import division
from __future__ import print_function

from datetime import datetime
import os

from absl.testing import absltest
from google.protobuf import text_format
from proto.stu3 import resources_pb2
from py.google.fhir.labels import encounter


_TESTDATA_PATH = 'com_google_fhir/testdata/stu3/labels'


class EncounterTest(absltest.TestCase):

  def setUp(self):
    self._test_data_dir = os.path.join(absltest.get_default_test_srcdir(),
                                       _TESTDATA_PATH)
    self._enc = resources_pb2.Encounter()
    with open(os.path.join(self._test_data_dir, 'encounter_1.pbtxt')) as f:
      text_format.Parse(f.read(), self._enc)
    self._bundle = resources_pb2.Bundle()
    self._bundle.entry.add().resource.encounter.CopyFrom(self._enc)
    self._synthea_enc = resources_pb2.Encounter()
    with open(os.path.join(self._test_data_dir,
                           'encounter_synthea.pbtxt')) as f:
      text_format.Parse(f.read(), self._synthea_enc)
    self._synthea_bundle = resources_pb2.Bundle()
    self._synthea_bundle.entry.add().resource.encounter.CopyFrom(
        self._synthea_enc)

  def testOnEncounter(self):
    self.assertTrue(encounter.EncounterIsValidHospitalization(self._enc))
    self.assertTrue(encounter.EncounterIsFinished(self._enc))

  def testAtDuration(self):
    inp24hr = [inp for inp in encounter.Inpatient24HrEncounters(self._bundle)]
    self.assertLen(inp24hr, 1)
    enc = inp24hr[0]
    at_duration_24_before = encounter.AtDuration(enc, -24)
    at_duration_0 = encounter.AtDuration(enc, 0)
    at_duration_24 = encounter.AtDuration(enc, 24)
    self.assertEqual(at_duration_24_before,
                     datetime(2009, 2, 12, 23, 31, 30))
    self.assertEqual(at_duration_0,
                     datetime(2009, 2, 13, 23, 31, 30))
    self.assertEqual(at_duration_24,
                     datetime(2009, 2, 14, 23, 31, 30))

  def testEncounterLengthDays(self):
    inp24hr = [inp for inp in encounter.Inpatient24HrEncounters(self._bundle)]
    self.assertLen(inp24hr, 1)
    enc = inp24hr[0]
    self.assertEqual(12860, int(encounter.EncounterLengthDays(enc)))
    self.assertLess(12860.0, encounter.EncounterLengthDays(enc))

  def testOnBundle(self):
    inp24hr = [inp for inp in encounter.Inpatient24HrEncounters(self._bundle)]
    self.assertLen(inp24hr, 1)
    # make sure we are not creating new copies of encounter.
    self.assertEqual(id(inp24hr[0]),
                     id(self._bundle.entry[0].resource.encounter))

  def testOnSyntheaBundle(self):
    inp24hr = [
        inp for inp in encounter.Inpatient24HrEncounters(
            bundle=self._synthea_bundle, for_synthea=True)
    ]
    self.assertLen(inp24hr, 1)
    self.assertEqual(
        id(inp24hr[0]), id(self._synthea_bundle.entry[0].resource.encounter))


if __name__ == '__main__':
  absltest.main()
