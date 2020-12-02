#
# Copyright 2020 Google LLC
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
"""Tests for the references module."""

from absl.testing import absltest
from proto.google.fhir.proto.r4.core import datatypes_pb2
from google.fhir import references
from google.fhir.utils import proto_utils


class ReferencesTest(absltest.TestCase):
  """A suite of tests to ensure proper FHIR reference handling."""

  def testSplitIfRelativeReference_withRelativeReference_succeeds(self):
    ref = datatypes_pb2.Reference(
        uri=datatypes_pb2.String(value='Practitioner/example'),
        display=datatypes_pb2.String(value='Dr Adam Careful'))

    uri_field = ref.DESCRIPTOR.fields_by_name['uri']
    practitioner_id_field = ref.DESCRIPTOR.fields_by_name['practitioner_id']
    self.assertTrue(proto_utils.field_is_set(ref, uri_field))
    self.assertFalse(proto_utils.field_is_set(ref, practitioner_id_field))

    references.split_if_relative_reference(ref)

    self.assertFalse(proto_utils.field_is_set(ref, uri_field))
    self.assertTrue(proto_utils.field_is_set(ref, practitioner_id_field))
    self.assertEqual(
        proto_utils.get_value_at_field(ref, practitioner_id_field),
        datatypes_pb2.ReferenceId(value='example'))

  def testSplitIfRelativeReference_withFragmentReference_succeeds(self):
    ref = datatypes_pb2.Reference(uri=datatypes_pb2.String(value='#org-1'))

    uri_field = ref.DESCRIPTOR.fields_by_name['uri']
    fragment_field = ref.DESCRIPTOR.fields_by_name['fragment']
    self.assertTrue(proto_utils.field_is_set(ref, uri_field))
    self.assertFalse(proto_utils.field_is_set(ref, fragment_field))

    references.split_if_relative_reference(ref)

    self.assertFalse(proto_utils.field_is_set(ref, uri_field))
    self.assertTrue(proto_utils.field_is_set(ref, fragment_field))
    self.assertEqual(
        proto_utils.get_value_at_field(ref, fragment_field),
        datatypes_pb2.String(value='org-1'))

  def testSplitIfRelativeReference_withUrlScheme_succeeds(self):
    ref = datatypes_pb2.Reference(
        uri=datatypes_pb2.String(
            value='http://acme.com/ehr/fhir/Practitioner/2323-33-4'))

    references.split_if_relative_reference(ref)

    uri_field = ref.DESCRIPTOR.fields_by_name['uri']
    self.assertEqual(
        proto_utils.get_value_at_field(ref, uri_field),
        datatypes_pb2.String(
            value='http://acme.com/ehr/fhir/Practitioner/2323-33-4'))

if __name__ == '__main__':
  absltest.main()
