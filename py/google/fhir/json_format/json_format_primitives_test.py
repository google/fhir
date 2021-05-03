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
"""Tests json primitives in //com_google_fhir/testdata/primitives."""
import os
from typing import Any, Type, cast

from google.protobuf import message
from absl.testing import absltest
from absl.testing import parameterized
from proto.google.fhir.proto.r4 import primitive_test_suite_pb2
from proto.google.fhir.proto.r4.core import datatypes_pb2
from google.fhir import extensions
from google.fhir import fhir_errors
from google.fhir.r4 import json_format
from google.fhir.r4 import primitive_handler
from google.fhir.testing import protobuf_compare
from google.fhir.testing import testdata_utils
from google.fhir.utils import proto_utils

_PRIMITIVE_TESTS_PATH = os.path.join('testdata', 'primitives')


# TODO: Parameterize on version types (like STU3, R4) and
# parameterize on primitive datatypes too.
class PrimitivesFormatTest(protobuf_compare.ProtoAssertions,
                           parameterized.TestCase):

  def setUp(self):
    super().setUp()

    self.primitive_handler = primitive_handler.PrimitiveHandler()

  def get_test_suite(
      self, file_name: str) -> primitive_test_suite_pb2.PrimitiveTestSuite:
    file_path = os.path.join(_PRIMITIVE_TESTS_PATH, file_name + '.prototxt')
    return testdata_utils.read_proto(
        file_path, primitive_test_suite_pb2.PrimitiveTestSuite)

  def _set_primitive_has_no_value_extension(self, primitive: message.Message):
    """Sets the PrimitiveHasNoValue FHIR extension on the provided primitive."""
    extensions_field = primitive.DESCRIPTOR.fields_by_name['extension']
    primitive_has_no_value = extensions.create_primitive_has_no_value(
        extensions_field.message_type)
    proto_utils.set_value_at_field(primitive, 'extension',
                                   [primitive_has_no_value])

  @parameterized.named_parameters(
      ('_withBase64Binary', 'Base64Binary', 'base64_binary',
       datatypes_pb2.Base64Binary),
      ('_withBoolean', 'Boolean', 'boolean', datatypes_pb2.Boolean),
      ('_withCanonical', 'Canonical', 'canonical', datatypes_pb2.Canonical),
      ('_withCode', 'Code', 'code', datatypes_pb2.Code),
      ('_withDate', 'Date', 'date', datatypes_pb2.Date),
      ('_withDateTime', 'DateTime', 'date_time', datatypes_pb2.DateTime),
      ('_withDecimal', 'Decimal', 'decimal', datatypes_pb2.Decimal),
      ('_withID', 'Id', 'id', datatypes_pb2.Id),
      ('_withInstant', 'Instant', 'instant', datatypes_pb2.Instant),
      ('_withInteger', 'Integer', 'integer', datatypes_pb2.Integer),
      ('_withMarkdown', 'Markdown', 'markdown', datatypes_pb2.Markdown),
      ('_withOid', 'Oid', 'oid', datatypes_pb2.Oid),
      ('_withPositiveInt', 'PositiveInt', 'positive_int',
       datatypes_pb2.PositiveInt),
      ('_WithString', 'String', 'string_proto', datatypes_pb2.String),
      ('_WithTime', 'Time', 'time', datatypes_pb2.Time),
      ('_withUnsignedInt', 'UnsignedInt', 'unsigned_int',
       datatypes_pb2.UnsignedInt),
      ('_withUri', 'Uri', 'uri', datatypes_pb2.Uri),
      ('_withUrl', 'Url', 'url', datatypes_pb2.Url),
      ('_withXhtml', 'Xhtml', 'xhtml', datatypes_pb2.Xhtml),
  )
  def test_valid_pairs(
      self, file_name: str, field_name: str, field_type: Type[message.Message]):
    test_suite = self.get_test_suite(file_name)

    for pair in test_suite.valid_pairs:
      expected_proto = proto_utils.get_value_at_field(pair.proto, field_name)
      expected_json = pair.json_string

      actual_json = json_format.print_fhir_to_json_string(expected_proto)
      actual_proto = json_format.json_fhir_string_to_proto(
          actual_json, field_type, default_timezone='Australia/Sydney')

      self.assertEqual(expected_json, actual_json)
      self.assertProtoEqual(expected_proto, actual_proto)

  @parameterized.named_parameters(
      ('_withBase64Binary', 'Base64Binary', datatypes_pb2.Base64Binary),
      ('_withBoolean', 'Boolean', datatypes_pb2.Boolean),
      ('_withCanonical', 'Canonical', datatypes_pb2.Canonical),
      ('_withCode', 'Code', datatypes_pb2.Code),
      ('_withDate', 'Date', datatypes_pb2.Date),
      ('_withDateTime', 'DateTime', datatypes_pb2.DateTime),
      ('_withDecimal', 'Decimal', datatypes_pb2.Decimal),
      ('_withID', 'Id', datatypes_pb2.Id),
      ('_withInstant', 'Instant', datatypes_pb2.Instant),
      ('_withInteger', 'Integer', datatypes_pb2.Integer),
      ('_withMarkdown', 'Markdown', datatypes_pb2.Markdown),
      ('_withOid', 'Oid', datatypes_pb2.Oid),
      ('_withPositiveInt', 'PositiveInt', datatypes_pb2.PositiveInt),
      ('_WithString', 'String', datatypes_pb2.String),
      ('_WithTime', 'Time', datatypes_pb2.Time),
      ('_withUnsignedInt', 'UnsignedInt', datatypes_pb2.UnsignedInt),
      ('_withUri', 'Uri', datatypes_pb2.Uri),
      ('_withUrl', 'Url', datatypes_pb2.Url),
      ('_withXhtml', 'Xhtml', datatypes_pb2.Xhtml),
  )
  def test_invalid_json(self, file_name: str,
                        field_type: Type[message.Message]):
    test_suite = self.get_test_suite(file_name)

    if not test_suite.invalid_json:
      self.fail('Must have at least one invalid json example!')

    for invalid_json in test_suite.invalid_json:
      # Some of the invalid examples may cause non InvalidFhirError errors,
      # which is acceptable.
      with self.assertRaises(Exception):
        _ = json_format.json_fhir_string_to_proto(invalid_json, field_type)

  @parameterized.named_parameters(
      ('_withBase64Binary', 'Base64Binary'),
      ('_withBoolean', 'Boolean'),
      ('_withCanonical', 'Canonical'),
      ('_withCode', 'Code'),
      ('_withDate', 'Date'),
      ('_withDateTime', 'DateTime'),
      ('_withDecimal', 'Decimal'),
      ('_withID', 'Id'),
      ('_withInstant', 'Instant'),
      ('_withInteger', 'Integer'),
      ('_withMarkdown', 'Markdown'),
      ('_withOid', 'Oid'),
      ('_withPositiveInt', 'PositiveInt'),
      ('_WithString', 'String'),
      ('_WithTime', 'Time'),
      ('_withUnsignedInt', 'UnsignedInt'),
      ('_withUri', 'Uri'),
      ('_withUrl', 'Url'),
      ('_withXhtml', 'Xhtml'),
  )
  def test_invalid_protos(self, file_name: str):
    test_suite = self.get_test_suite(file_name)

    if not (test_suite.invalid_protos or test_suite.no_invalid_protos_reason):
      self.fail('Must have at least one invalid_proto or set '
                'no_invalid_protos_reason.')

    for invalid_proto in test_suite.invalid_protos:
      with self.assertRaises(fhir_errors.InvalidFhirError):
        _ = json_format.print_fhir_to_json_string(invalid_proto)

  @parameterized.named_parameters(
      ('_withBase64Binary', datatypes_pb2.Base64Binary),
      ('_withBoolean', datatypes_pb2.Boolean),
      ('_withCanonical', datatypes_pb2.Canonical),
      ('_withCode', datatypes_pb2.Code),
      ('_withDate', datatypes_pb2.Date),
      ('_withDateTime', datatypes_pb2.DateTime),
      ('_withDecimal', datatypes_pb2.Decimal),
      ('_withID', datatypes_pb2.Id),
      ('_withInstant', datatypes_pb2.Instant),
      ('_withInteger', datatypes_pb2.Integer),
      ('_withMarkdown', datatypes_pb2.Markdown),
      ('_withOid', datatypes_pb2.Oid),
      ('_withPositiveInt', datatypes_pb2.PositiveInt),
      ('_WithString', datatypes_pb2.String),
      ('_WithTime', datatypes_pb2.Time),
      ('_withUnsignedInt', datatypes_pb2.UnsignedInt),
      ('_withUri', datatypes_pb2.Uri),
      ('_withUrl', datatypes_pb2.Url),
      # Xhtml cannot have extentions. It is always consdered to have a value.
  )
  def test_no_value_behaviour_valid(self, field_type: Type[message.Message]):
    # It's ok to have no value if there's another extension present
    # (Two extensions in total).
    primitive_with_no_value = field_type()

    self._set_primitive_has_no_value_extension(primitive_with_no_value)

    # Add arbitrary extension.
    extensions_field = primitive_with_no_value.DESCRIPTOR.fields_by_name[
        'extension']
    extension = proto_utils.create_message_from_descriptor(
        extensions_field.message_type)
    cast(Any, extension).url.value = 'abcd'
    cast(Any, extension).value.boolean.value = True

    proto_utils.append_value_at_field(primitive_with_no_value, 'extension',
                                      extension)
    _ = json_format.print_fhir_to_json_string(primitive_with_no_value)

  @parameterized.named_parameters(
      ('_withBase64Binary', datatypes_pb2.Base64Binary),
      ('_withBoolean', datatypes_pb2.Boolean),
      ('_withCanonical', datatypes_pb2.Canonical),
      ('_withCode', datatypes_pb2.Code),
      ('_withDate', datatypes_pb2.Date),
      ('_withDateTime', datatypes_pb2.DateTime),
      ('_withDecimal', datatypes_pb2.Decimal),
      ('_withID', datatypes_pb2.Id),
      ('_withInstant', datatypes_pb2.Instant),
      ('_withInteger', datatypes_pb2.Integer),
      ('_withMarkdown', datatypes_pb2.Markdown),
      ('_withOid', datatypes_pb2.Oid),
      ('_withPositiveInt', datatypes_pb2.PositiveInt),
      ('_WithString', datatypes_pb2.String),
      ('_WithTime', datatypes_pb2.Time),
      ('_withUnsignedInt', datatypes_pb2.UnsignedInt),
      ('_withUri', datatypes_pb2.Uri),
      ('_withUrl', datatypes_pb2.Url),
      # Xhtml cannot have extentions. It is always consdered to have a value.
  )
  def test_no_value_behaviour_invalid_if_no_other_extensions(
      self, field_type: Type[message.Message]):
    # It's invalid to have no extensions (other than the no-value extension) and
    # no value.
    primitive_with_no_value = field_type()

    self._set_primitive_has_no_value_extension(primitive_with_no_value)

    with self.assertRaises(fhir_errors.InvalidFhirError):
      _ = json_format.print_fhir_to_json_string(primitive_with_no_value)


if __name__ == '__main__':
  absltest.main()
