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
"""Tests for util."""

from __future__ import absolute_import
from __future__ import division
from __future__ import print_function

from absl.testing import absltest
from google.protobuf import text_format
from proto.stu3 import resources_pb2
from py.google.fhir.stu3 import util


class UtilTest(absltest.TestCase):

  def testGetPatient(self):
    patient = resources_pb2.Patient()
    text_format.Merge(
        """
    id: {
      value: "Patient/1"
    }
    birth_date: {
       # February 13, 2009 11:31:30 PM
        value_us : 1234567890000000
        timezone : "America/New_York"
        precision: DAY
    }
    """, patient)
    bundle = resources_pb2.Bundle()
    bundle.entry.add().resource.patient.CopyFrom(patient)
    p = util.GetPatient(bundle)
    self.assertEqual(patient, p)


if __name__ == '__main__':
  absltest.main()
