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

import abc
import os
from typing import Any, List, Type, cast

from google.protobuf import descriptor
from google.protobuf import message
from absl.testing import absltest
from proto.google.fhir.proto.r4 import primitive_test_suite_pb2
from proto.google.fhir.proto.r4.core import datatypes_pb2
from google.fhir import extensions
from google.fhir import fhir_errors
from google.fhir.r4 import json_format
from google.fhir.testing import protobuf_compare
from google.fhir.testing import testdata_utils
from google.fhir.utils import annotation_utils
from google.fhir.utils import fhir_types
from google.fhir.utils import path_utils
from google.fhir.utils import proto_utils

_PRIMITIVE_TESTS_PATH = os.path.join('testdata', 'primitives')


class _PrimitiveData:
  """Stores data needed for each primitive test.

  Attributes:
    name: Name of the primitive being tested.
    field_name: Name of the proto field in valid_pairs containing the primitive.
    field_type: The message class of the primitive.
  """
  name: str
  field_name: str
  field_type: Type[message.Message]

  def __init__(self, name: str, field_name: str,
               field_type: Type[message.Message]) -> None:
    self.name = name
    self.field_name = field_name
    self.field_type = field_type


class _Base():

  class JsonFormatPrimitivesTest(
      protobuf_compare.ProtoAssertions,
      absltest.TestCase,
      metaclass=abc.ABCMeta):
    """A base class for testing json primitives in a common directory.

    Attributes:
      datatypes_descriptor: The datatypes_pb2.DESCRIPTOR of the version under
        test.
      primitive_test_suite_class: The primitive_test_suite class (The actual
        class, not an instance of it) of the version under test.
      json_format: The json_format module of the version under test.
    """

    @property
    @abc.abstractproperty
    def datatypes_descriptor(self) -> descriptor.FileDescriptor:
      raise NotImplementedError(
          'Subclasses *must* implement `datatypes_descriptor`')

    @property
    @abc.abstractproperty
    def primitive_test_suite_class(
        self) -> Type[primitive_test_suite_pb2.PrimitiveTestSuite]:
      raise NotImplementedError(
          'Subclasses *must* implement `primitive_test_suite_class`')

    @property
    @abc.abstractproperty
    def json_format(self) -> Any:
      raise NotImplementedError('Subclasses *must* implement `json_format`')

    def setUp(self):
      super().setUp()
      self.primitive_data_list = _get_list_of_primitive_data(
          self.datatypes_descriptor)

    def _set_primitive_has_no_value_extension(
        self, primitive: message.Message) -> None:
      """Sets the PrimitiveHasNoValue FHIR extension on the primitive."""
      extensions_field = primitive.DESCRIPTOR.fields_by_name['extension']
      primitive_has_no_value = extensions.create_primitive_has_no_value(
          extensions_field.message_type)
      proto_utils.set_value_at_field(primitive, 'extension',
                                     [primitive_has_no_value])

    def test_valid_pairs(self):
      for primitive_data in self.primitive_data_list:

        file_name = primitive_data.name
        field_name = primitive_data.field_name
        field_type = primitive_data.field_type

        file_path = os.path.join(_PRIMITIVE_TESTS_PATH, file_name + '.prototxt')
        test_suite = testdata_utils.read_proto(file_path,
                                               self.primitive_test_suite_class)

        for pair in test_suite.valid_pairs:
          expected_proto = proto_utils.get_value_at_field(
              pair.proto, field_name)
          expected_json = pair.json_string

          actual_json = self.json_format.print_fhir_to_json_string(
              expected_proto)
          actual_proto = self.json_format.json_fhir_string_to_proto(
              actual_json, field_type, default_timezone='Australia/Sydney')

          self.assertEqual(expected_json, actual_json)
          self.assertProtoEqual(expected_proto, actual_proto)

    def test_invalid_json(self):
      for primitive_data in self.primitive_data_list:

        file_name = primitive_data.name
        field_type = primitive_data.field_type

        file_path = os.path.join(_PRIMITIVE_TESTS_PATH, file_name + '.prototxt')
        test_suite = testdata_utils.read_proto(file_path,
                                               self.primitive_test_suite_class)

        if not test_suite.invalid_json:
          self.fail('Must have at least one invalid json example!')

        for invalid_json in test_suite.invalid_json:
          # Some of the invalid examples may cause non InvalidFhirError errors,
          # which is acceptable.
          with self.assertRaises(Exception):
            _ = self.json_format.json_fhir_string_to_proto(
                invalid_json, field_type)

    def test_invalid_protos(self):
      for primitive_data in self.primitive_data_list:

        file_name = primitive_data.name

        file_path = os.path.join(_PRIMITIVE_TESTS_PATH, file_name + '.prototxt')
        test_suite = testdata_utils.read_proto(file_path,
                                               self.primitive_test_suite_class)

        if not (test_suite.invalid_protos or
                test_suite.no_invalid_protos_reason):
          self.fail('Must have at least one invalid_proto or set '
                    'no_invalid_protos_reason.')

        for invalid_proto in test_suite.invalid_protos:
          with self.assertRaises(fhir_errors.InvalidFhirError):
            _ = self.json_format.print_fhir_to_json_string(invalid_proto)

    def test_no_value_behaviour_valid(self):
      for primitive_data in self.primitive_data_list:

        file_name = primitive_data.name
        field_type = primitive_data.field_type

        # Xhtml cannot have extentions. It is always consdered to have a value.
        # So we are not testing it here.
        if file_name == 'Xhtml':
          continue

        # It's ok to have no value if there's at least another extension present
        # (More than one extension).
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
        _ = self.json_format.print_fhir_to_json_string(primitive_with_no_value)

    def test_no_value_behaviour_invalid_if_no_other_extensions(self):

      for primitive_data in self.primitive_data_list:

        file_name = primitive_data.name
        field_type = primitive_data.field_type

        # Xhtml cannot have extentions. It is always consdered to have a value.
        # So we are not testing it here.
        if file_name == 'Xhtml':
          continue

        # It's invalid to have no extensions (other than the no-value extension)
        # and no value.
        primitive_with_no_value = field_type()

        self._set_primitive_has_no_value_extension(primitive_with_no_value)

        with self.assertRaises(fhir_errors.InvalidFhirError):
          _ = self.json_format.print_fhir_to_json_string(
              primitive_with_no_value)


class R4JsonFormatPrimitivesTest(_Base.JsonFormatPrimitivesTest):

  @property
  def datatypes_descriptor(self) -> descriptor.FileDescriptor:
    return datatypes_pb2.DESCRIPTOR

  @property
  def primitive_test_suite_class(
      self) -> Type[primitive_test_suite_pb2.PrimitiveTestSuite]:
    return primitive_test_suite_pb2.PrimitiveTestSuite

  @property
  def json_format(self) -> Any:
    return json_format


def _get_list_of_primitive_data(
    datatypes_descriptor: descriptor.FileDescriptor,) -> List[_PrimitiveData]:
  """Returns a list of data needed to test primitives.

  Args:
    datatypes_descriptor: The current FHIR version proto descriptor, (e.g
      r4.core.datatypes_pb2.DESCRIPTOR or stu3.datatypes_pb2.DESCRIPTOR) that we
      want to test.
  Returns: A list of _PrimitiveData for all the primitves that need to be
    tested.
  """
  all_messages_map: dict[
      str, descriptor.Descriptor] = datatypes_descriptor.message_types_by_name

  primitive_data_list: List[_PrimitiveData] = []

  for name, msg_descriptor in all_messages_map.items():
    if _is_only_primitive(name, msg_descriptor):

      field_name = _normalize_field_name(
          path_utils.camel_case_to_snake_case(name))

      primtive_data = _PrimitiveData(
          name, field_name,
          proto_utils.get_message_class_from_descriptor(msg_descriptor))

      primitive_data_list.append(primtive_data)

  return primitive_data_list


def _is_only_primitive(name: str,
                       msg_descriptor: descriptor.Descriptor) -> bool:
  """Returns if the message specified is only a primitive and nothing else."""
  return (
      annotation_utils.is_primitive_type(msg_descriptor)
      # TODO: Remove "ReferenceId" handling once ReferenceId
      # is no longer (erroneously) marked as a primitive.
      and name != 'ReferenceId'
      # STU3 profiles of codes are marked as primitive types.
      and not fhir_types.is_profile_of_code(msg_descriptor))


def _normalize_field_name(field_name: str) -> str:
  # There is an exception where the corresponding proto field for
  # `String` is `string_proto` and not `string`.
  if field_name == 'string':
    return 'string_proto'
  return field_name


if __name__ == '__main__':
  absltest.main()
