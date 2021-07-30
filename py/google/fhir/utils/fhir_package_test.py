#
# Copyright 2021 Google LLC
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
"""Test fhir_package functionality."""

import os
from absl import flags

from absl.testing import absltest
from proto.google.fhir.proto import profile_config_pb2
from google.fhir.utils import fhir_package

FLAGS = flags.FLAGS

_R4_DEFINITIONS_COUNT = 653
_R4_CODESYSTEMS_COUNT = 1062
_R4_VALUESETS_COUNT = 1316
_R4_SEARCH_PARAMETERS_COUNT = 1385


class FhirPackageTest(absltest.TestCase):

  def testFhirPackageEquality_withEqualOperands_succeeds(self):
    lhs = fhir_package.FhirPackage(
        package_info=profile_config_pb2.PackageInfo(proto_package='Foo'),
        structure_definitions=[],
        search_parameters=[],
        code_systems=[],
        value_sets=[])
    rhs = fhir_package.FhirPackage(
        package_info=profile_config_pb2.PackageInfo(proto_package='Foo'),
        structure_definitions=[],
        search_parameters=[],
        code_systems=[],
        value_sets=[])
    self.assertEqual(lhs, rhs)

  def testFhirPackageEquality_withNonEqualOperands_succeeds(self):
    lhs = fhir_package.FhirPackage(
        package_info=profile_config_pb2.PackageInfo(proto_package='Foo'),
        structure_definitions=[],
        search_parameters=[],
        code_systems=[],
        value_sets=[])
    rhs = fhir_package.FhirPackage(
        package_info=profile_config_pb2.PackageInfo(proto_package='Bar'),
        structure_definitions=[],
        search_parameters=[],
        code_systems=[],
        value_sets=[])
    self.assertNotEqual(lhs, rhs)

  def testFhirPackageLoad_withValidFhirPackage_succeeds(self):
    package_filepath = os.path.join(
        FLAGS.test_srcdir, 'com_google_fhir/spec/fhir_r4_package.zip')
    package = fhir_package.FhirPackage.load(package_filepath)
    self.assertEqual(package.package_info.proto_package, 'google.fhir.r4.core')
    self.assertLen(package.structure_definitions, _R4_DEFINITIONS_COUNT)
    self.assertLen(package.code_systems, _R4_CODESYSTEMS_COUNT)
    self.assertLen(package.value_sets, _R4_VALUESETS_COUNT)
    self.assertLen(package.search_parameters, _R4_SEARCH_PARAMETERS_COUNT)


if __name__ == '__main__':
  absltest.main()
