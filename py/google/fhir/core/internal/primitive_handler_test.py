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
"""Abstract test classes for FHIR-specific primitive_handler modules."""

import abc
import decimal
import json
import os
from typing import Any, Type, cast

from google.protobuf import message
from absl.testing import absltest
from google.fhir.core import extensions
from google.fhir.core import fhir_errors
from google.fhir.core.internal import _primitive_time_utils
from google.fhir.core.internal import primitive_handler
from google.fhir.core.testing import testdata_utils
from google.fhir.core.utils import path_utils
from google.fhir.core.utils import proto_utils


class PrimitiveWrapperPrimitiveHasNoValueTest(
    absltest.TestCase, metaclass=abc.ABCMeta):
  """A suite of tests to ensure proper validation for PrimitiveHasNoValue."""

  @property
  @abc.abstractmethod
  def primitive_handler(self) -> primitive_handler.PrimitiveHandler:
    pass

  def test_primitive_has_no_value_with_valid_base64_binary_succeeds(self):
    self.assert_set_valid_primitive_has_no_value_succeeds(
        self.primitive_handler.base64_binary_cls)

  def test_primitive_has_no_value_with_invalid_base64_binary_raises(self):
    self.assert_set_invalid_primitive_has_no_value_raises(
        self.primitive_handler.base64_binary_cls)

  def test_primitive_has_no_value_with_valid_boolean_succeeds(self):
    self.assert_set_valid_primitive_has_no_value_succeeds(
        self.primitive_handler.boolean_cls)

  def test_primitive_has_no_value_with_invalid_boolean_raises(self):
    self.assert_set_invalid_primitive_has_no_value_raises(
        self.primitive_handler.boolean_cls)

  def test_primitive_has_no_value_with_valid_code_succeeds(self):
    self.assert_set_valid_primitive_has_no_value_succeeds(
        self.primitive_handler.code_cls)

  def test_primitive_has_no_value_with_invalid_code_raises(self):
    self.assert_set_invalid_primitive_has_no_value_raises(
        self.primitive_handler.code_cls)

  def test_primitive_has_no_value_with_valid_date_succeeds(self):
    self.assert_set_valid_primitive_has_no_value_succeeds(
        self.primitive_handler.date_cls)

  def test_primitive_has_no_value_with_invalid_date_raises(self):
    self.assert_set_invalid_primitive_has_no_value_raises(
        self.primitive_handler.date_cls)

  def test_primitive_has_no_value_with_valid_date_time_succeeds(self):
    self.assert_set_valid_primitive_has_no_value_succeeds(
        self.primitive_handler.date_time_cls)

  def test_primitive_has_no_value_with_invalid_date_time_raises(self):
    self.assert_set_invalid_primitive_has_no_value_raises(
        self.primitive_handler.date_time_cls)

  def test_primitive_has_no_value_with_valid_decimal_succeeds(self):
    self.assert_set_valid_primitive_has_no_value_succeeds(
        self.primitive_handler.decimal_cls)

  def test_primitive_has_no_value_with_invalid_decimal_raises(self):
    self.assert_set_invalid_primitive_has_no_value_raises(
        self.primitive_handler.decimal_cls)

  def test_primitive_has_no_value_with_valid_id_succeeds(self):
    self.assert_set_valid_primitive_has_no_value_succeeds(
        self.primitive_handler.id_cls)

  def test_primitive_has_no_value_with_invalid_id_raises(self):
    self.assert_set_invalid_primitive_has_no_value_raises(
        self.primitive_handler.id_cls)

  def test_primitive_has_no_value_with_valid_instant_succeeds(self):
    self.assert_set_valid_primitive_has_no_value_succeeds(
        self.primitive_handler.instant_cls)

  def test_primitive_has_no_value_with_invalid_instant_raises(self):
    self.assert_set_invalid_primitive_has_no_value_raises(
        self.primitive_handler.instant_cls)

  def test_primitive_has_no_value_with_valid_integer_succeeds(self):
    self.assert_set_valid_primitive_has_no_value_succeeds(
        self.primitive_handler.integer_cls)

  def test_primitive_has_no_value_with_invalid_integer_raises(self):
    self.assert_set_invalid_primitive_has_no_value_raises(
        self.primitive_handler.integer_cls)

  def test_primitive_has_no_value_with_valid_markdown_succeeds(self):
    self.assert_set_valid_primitive_has_no_value_succeeds(
        self.primitive_handler.markdown_cls)

  def test_primitive_has_no_value_with_invalid_markdown_raises(self):
    self.assert_set_invalid_primitive_has_no_value_raises(
        self.primitive_handler.markdown_cls)

  def test_primitive_has_no_value_with_valid_oid_succeeds(self):
    self.assert_set_valid_primitive_has_no_value_succeeds(
        self.primitive_handler.oid_cls)

  def test_primitive_has_no_value_with_invalid_oid_raises(self):
    self.assert_set_invalid_primitive_has_no_value_raises(
        self.primitive_handler.oid_cls)

  def test_primitive_has_no_value_with_valid_positive_int_succeeds(self):
    self.assert_set_valid_primitive_has_no_value_succeeds(
        self.primitive_handler.positive_int_cls)

  def test_primitive_has_no_value_with_invalid_positive_int_raises(self):
    self.assert_set_invalid_primitive_has_no_value_raises(
        self.primitive_handler.positive_int_cls)

  def test_primitive_has_no_value_with_valid_string_succeeds(self):
    self.assert_set_valid_primitive_has_no_value_succeeds(
        self.primitive_handler.string_cls)

  def test_primitive_has_no_value_with_invalid_string_raises(self):
    self.assert_set_invalid_primitive_has_no_value_raises(
        self.primitive_handler.string_cls)

  def test_primitive_has_no_value_with_valid_time_succeeds(self):
    self.assert_set_valid_primitive_has_no_value_succeeds(
        self.primitive_handler.time_cls)

  def test_primitive_has_no_value_with_invalid_time_raises(self):
    self.assert_set_invalid_primitive_has_no_value_raises(
        self.primitive_handler.time_cls)

  def test_primitive_has_no_value_with_valid_unsigned_int_succeeds(self):
    self.assert_set_valid_primitive_has_no_value_succeeds(
        self.primitive_handler.unsigned_int_cls)

  def test_primitive_has_no_value_with_invalid_unsigned_int_raises(self):
    self.assert_set_invalid_primitive_has_no_value_raises(
        self.primitive_handler.unsigned_int_cls)

  def test_primitive_has_no_value_with_valid_uri_succeeds(self):
    self.assert_set_valid_primitive_has_no_value_succeeds(
        self.primitive_handler.uri_cls)

  def _set_primitive_has_no_value_extension(self,
                                            primitive: message.Message) -> None:
    """Sets the PrimitiveHasNoValue FHIR extension on the provided primitive."""
    extensions_field = primitive.DESCRIPTOR.fields_by_name['extension']
    primitive_has_no_value = extensions.create_primitive_has_no_value(
        extensions_field.message_type)
    proto_utils.set_value_at_field(primitive, 'extension',
                                   [primitive_has_no_value])

  def assert_set_valid_primitive_has_no_value_succeeds(
      self, primitive_cls: Type[message.Message]) -> None:
    """Tests setting PrimitiveHasNoValue with other extensions present.

    Having a PrimitiveHasNoValue extension is okay provided there are other
    extensions set. This is expected not to raise any exceptions.

    Args:
      primitive_cls: The type of primitive to instantiate and test.
    """
    primitive = primitive_cls()
    self._set_primitive_has_no_value_extension(primitive)

    # Add arbitrary extension
    extensions_field = primitive.DESCRIPTOR.fields_by_name['extension']
    extension = proto_utils.create_message_from_descriptor(
        extensions_field.message_type)

    # Silencing the type checkers as pytype doesn't fully support structural
    # subtyping yet.
    # pylint: disable=line-too-long
    # See: https://mypy.readthedocs.io/en/stable/casts.html#casts-and-type-assertions.
    # pylint: enable=line-too-long
    # Soon: https://www.python.org/dev/peps/pep-0544/.
    cast(Any, extension).url.value = 'abcd'
    cast(Any, extension).value.boolean.value = True

    proto_utils.append_value_at_field(primitive, 'extension', extension)
    try:
      self.primitive_handler.primitive_wrapper_from_primitive(primitive)
    except fhir_errors.InvalidFhirError as e:
      self.fail('PrimitiveHasNoValue validation failed for '
                f'{primitive.DESCRIPTOR.full_name}: {e}.')

  def assert_set_invalid_primitive_has_no_value_raises(
      self, primitive_cls: Type[message.Message]) -> None:
    """Tests setting PrimitiveHasNoValue with no other fields present.

    Having a PrimitiveHasNoValue extension is only acceptable provided there
    are no other fields other than id or extension set. This is expected to
    raise an exception.

    Args:
      primitive_cls: The type of primitive to instantiate and test.
    """
    primitive = primitive_cls()
    self._set_primitive_has_no_value_extension(primitive)

    with self.assertRaises(fhir_errors.InvalidFhirError) as fe:
      self.primitive_handler.primitive_wrapper_from_primitive(primitive)

    self.assertIsInstance(fe.exception, fhir_errors.InvalidFhirError)


class PrimitiveWrapperProtoValidationTest(
    absltest.TestCase, metaclass=abc.ABCMeta):
  """Tests the primitive_wrapper classes for proto validation functionality.

  Validation tests generally consist of two components:
  1. Validation with a valid FHIR primitive (should succeed)
  2. Validation with an *invalid* FHIR primitive (should raise)

  Note that, these tests may *not* be applicable to every type. For example,
  some types, like Boolean, don't have an invalid representation.
  """

  _PROTO_DELIMITER = '\n---\n'

  @property
  @abc.abstractmethod
  def primitive_handler(self) -> primitive_handler.PrimitiveHandler:
    pass

  @property
  @abc.abstractmethod
  def validation_dir(self) -> str:
    pass

  def test_validate_wrapped_with_valid_base64_binary_succeeds(self):
    self.assert_validation_of_valid_primitive_succeeds(
        self.primitive_handler.base64_binary_cls)

  def test_validate_wrapped_with_valid_boolean_succeeds(self):
    self.assert_validation_of_valid_primitive_succeeds(
        self.primitive_handler.boolean_cls)

  def test_validate_wrapped_with_valid_code_succeeds(self):
    self.assert_validation_of_valid_primitive_succeeds(
        self.primitive_handler.code_cls)

  def test_validate_wrapped_with_invalid_code_raises(self):
    self.assert_validation_of_invalid_primitive_raises(
        self.primitive_handler.code_cls)

  def test_validate_wrapped_with_valid_date_succeeds(self):
    self.assert_validation_of_valid_primitive_succeeds(
        self.primitive_handler.date_cls)

  def test_validate_wrapped_with_invalid_date_raises(self):
    self.assert_validation_of_invalid_primitive_raises(
        self.primitive_handler.date_cls)

  def test_validate_wrapped_with_valid_date_time_succeeds(self):
    self.assert_validation_of_valid_primitive_succeeds(
        self.primitive_handler.date_time_cls)

  def test_validate_wrapped_with_invalid_date_time_raises(self):
    self.assert_validation_of_invalid_primitive_raises(
        self.primitive_handler.date_time_cls)

  def test_validate_wrapped_with_valid_decimal_succeeds(self):
    self.assert_validation_of_valid_primitive_succeeds(
        self.primitive_handler.decimal_cls)

  def test_validate_wrapped_with_invalid_decimal_raises(self):
    self.assert_validation_of_invalid_primitive_raises(
        self.primitive_handler.decimal_cls)

  def test_validate_wrapped_with_valid_id_succeeds(self):
    self.assert_validation_of_valid_primitive_succeeds(
        self.primitive_handler.id_cls)

  def test_validate_wrapped_with_invalid_id_raises(self):
    self.assert_validation_of_invalid_primitive_raises(
        self.primitive_handler.id_cls)

  def test_validate_wrapped_with_valid_instant_succeeds(self):
    self.assert_validation_of_valid_primitive_succeeds(
        self.primitive_handler.instant_cls)

  def test_validate_wrapped_with_invalid_instant_raises(self):
    self.assert_validation_of_invalid_primitive_raises(
        self.primitive_handler.instant_cls)

  def test_validate_wrapped_with_valid_integer_succeeds(self):
    self.assert_validation_of_valid_primitive_succeeds(
        self.primitive_handler.integer_cls)

  def test_validate_wrapped_with_valid_markdown_succeeds(self):
    self.assert_validation_of_valid_primitive_succeeds(
        self.primitive_handler.markdown_cls)

  def test_validate_wrapped_with_valid_oid_succeeds(self):
    self.assert_validation_of_valid_primitive_succeeds(
        self.primitive_handler.oid_cls)

  def test_validate_wrapped_with_invalid_oid_raises(self):
    self.assert_validation_of_invalid_primitive_raises(
        self.primitive_handler.oid_cls)

  def test_validate_wrapped_with_valid_positive_int_succeeds(self):
    self.assert_validation_of_valid_primitive_succeeds(
        self.primitive_handler.positive_int_cls)

  def test_validate_wrapped_with_invalid_positive_int_raises(self):
    self.assert_validation_of_invalid_primitive_raises(
        self.primitive_handler.positive_int_cls)

  def test_validate_wrapped_with_valid_string_succeeds(self):
    self.assert_validation_of_valid_primitive_succeeds(
        self.primitive_handler.string_cls)

  def test_validate_wrapped_with_valid_time_succeeds(self):
    self.assert_validation_of_valid_primitive_succeeds(
        self.primitive_handler.time_cls)

  def test_validate_wrapped_with_invalid_time_raises(self):
    self.assert_validation_of_invalid_primitive_raises(
        self.primitive_handler.time_cls)

  def test_validate_wrapped_with_valid_unsigned_int_succeeds(self):
    self.assert_validation_of_valid_primitive_succeeds(
        self.primitive_handler.unsigned_int_cls)

  def test_validate_wrapped_with_valid_uri_succeeds(self):
    self.assert_validation_of_valid_primitive_succeeds(
        self.primitive_handler.uri_cls)

  def test_validate_wrapped_with_valid_xhtml_succeeds(self):
    self.assert_validation_of_valid_primitive_succeeds(
        self.primitive_handler.xhtml_cls)

  def assert_validation_of_valid_primitive_succeeds(
      self, primitive_cls: Type[message.Message]) -> None:
    """Performs a suite of validation tests on valid FHIR primitives."""
    filename = path_utils.camel_case_to_snake_case(
        primitive_cls.DESCRIPTOR.name)
    valid_protos = testdata_utils.read_protos(
        os.path.join(self.validation_dir, filename + '.valid.prototxt'),
        primitive_cls, self._PROTO_DELIMITER)

    for valid_proto in valid_protos:
      try:
        self.primitive_handler.primitive_wrapper_from_primitive(valid_proto)
      except fhir_errors.InvalidFhirError as e:
        self.fail(f'{filename} did not represent valid FHIR: {e}.')

  def assert_validation_of_invalid_primitive_raises(
      self, primitive_cls: Type[message.Message]) -> None:
    """Performs a suite of validation tests on invalid FHIR primitives."""
    filename = path_utils.camel_case_to_snake_case(
        primitive_cls.DESCRIPTOR.name)
    invalid_protos = testdata_utils.read_protos(
        os.path.join(self.validation_dir, filename + '.invalid.prototxt'),
        primitive_cls, self._PROTO_DELIMITER)

    for invalid_proto in invalid_protos:
      with self.assertRaises(fhir_errors.InvalidFhirError) as fe:
        self.primitive_handler.primitive_wrapper_from_primitive(invalid_proto)

      self.assertIsInstance(fe.exception, fhir_errors.InvalidFhirError)


class PrimitiveWrapperJsonValidationTest(
    absltest.TestCase, metaclass=abc.ABCMeta):
  """Tests the primitive_wrapper classes for json parsing/validation."""

  @property
  @abc.abstractmethod
  def primitive_handler(self) -> primitive_handler.PrimitiveHandler:
    pass

  @property
  @abc.abstractmethod
  def validation_dir(self) -> str:
    pass

  def test_validate_wrapped_with_valid_base64_binary_succeeds(self):
    self.assert_json_validation_with_valid_primitive_succeeds(
        self.primitive_handler.base64_binary_cls)

  def test_validate_wrapped_with_invalid_base64_binary_raises(self):
    self.assert_json_validation_with_invalid_primitive_raises(
        self.primitive_handler.base64_binary_cls)

  def test_validate_wrapped_with_valid_boolean_succeeds(self):
    self.assert_json_validation_with_valid_primitive_succeeds(
        self.primitive_handler.boolean_cls)

  def test_validate_wrapped_with_valid_code_succeeds(self):
    self.assert_json_validation_with_valid_primitive_succeeds(
        self.primitive_handler.code_cls)

  def test_validate_wrapped_with_invalid_code_raises(self):
    self.assert_json_validation_with_invalid_primitive_raises(
        self.primitive_handler.code_cls)

  def test_validate_wrapped_with_valid_date_succeeds(self):
    self.assert_json_validation_with_valid_primitive_succeeds(
        self.primitive_handler.date_cls)

  def test_validate_wrapped_with_invalid_date_raises(self):
    self.assert_json_validation_with_invalid_primitive_raises(
        self.primitive_handler.date_cls)

  def test_validate_wrapped_with_valid_date_time_succeeds(self):
    self.assert_json_validation_with_valid_primitive_succeeds(
        self.primitive_handler.date_time_cls)

  def test_validate_wrapped_with_invalid_date_time_raises(self):
    self.assert_json_validation_with_invalid_primitive_raises(
        self.primitive_handler.date_time_cls)

  def test_validate_wrapped_with_valid_decimal_succeeds(self):
    self.assert_json_validation_with_valid_primitive_succeeds(
        self.primitive_handler.decimal_cls)

  def test_validate_wrapped_with_invalid_decimal_raises(self):
    self.assert_json_validation_with_invalid_primitive_raises(
        self.primitive_handler.decimal_cls)

  def test_validate_wrapped_with_valid_id_succeeds(self):
    self.assert_json_validation_with_valid_primitive_succeeds(
        self.primitive_handler.id_cls)

  def test_validate_wrapped_with_invalid_id_raises(self):
    self.assert_json_validation_with_invalid_primitive_raises(
        self.primitive_handler.id_cls)

  def test_validate_wrapped_with_valid_instant_succeeds(self):
    self.assert_json_validation_with_valid_primitive_succeeds(
        self.primitive_handler.instant_cls)

  def test_validate_wrapped_with_invalid_instant_raises(self):
    self.assert_json_validation_with_invalid_primitive_raises(
        self.primitive_handler.instant_cls)

  def test_validate_wrapped_with_valid_integer_succeeds(self):
    self.assert_json_validation_with_valid_primitive_succeeds(
        self.primitive_handler.integer_cls)

  def test_validate_wrapped_with_valid_markdown_succeeds(self):
    self.assert_json_validation_with_valid_primitive_succeeds(
        self.primitive_handler.markdown_cls)

  def test_validate_wrapped_with_valid_oid_succeeds(self):
    self.assert_json_validation_with_valid_primitive_succeeds(
        self.primitive_handler.oid_cls)

  def test_validate_wrapped_with_invalid_oid_raises(self):
    self.assert_json_validation_with_invalid_primitive_raises(
        self.primitive_handler.oid_cls)

  def test_validate_wrapped_with_valid_positive_int_succeeds(self):
    self.assert_json_validation_with_valid_primitive_succeeds(
        self.primitive_handler.positive_int_cls)

  def test_validate_wrapped_with_invalid_positive_int_raises(self):
    self.assert_json_validation_with_invalid_primitive_raises(
        self.primitive_handler.positive_int_cls)

  def test_validate_wrapped_with_valid_string_succeeds(self):
    self.assert_json_validation_with_valid_primitive_succeeds(
        self.primitive_handler.string_cls)

  def test_validate_wrapped_with_valid_time_succeeds(self):
    self.assert_json_validation_with_valid_primitive_succeeds(
        self.primitive_handler.time_cls)

  def test_validate_wrapped_with_invalid_time_raises(self):
    self.assert_json_validation_with_invalid_primitive_raises(
        self.primitive_handler.time_cls)

  def test_validate_wrapped_with_valid_unsigned_int_succeeds(self):
    self.assert_json_validation_with_valid_primitive_succeeds(
        self.primitive_handler.unsigned_int_cls)

  def test_validate_wrapped_with_valid_uri_succeeds(self):
    self.assert_json_validation_with_valid_primitive_succeeds(
        self.primitive_handler.uri_cls)

  def test_validate_wrapped_with_valid_xhtml_succeeds(self):
    self.assert_json_validation_with_valid_primitive_succeeds(
        self.primitive_handler.xhtml_cls)

  def assert_json_validation_with_valid_primitive_succeeds(
      self, primitive_cls: Type[message.Message]) -> None:
    """Performs a suite of validation tests on valid FHIR primitives."""
    filename = path_utils.camel_case_to_snake_case(
        primitive_cls.DESCRIPTOR.name)
    filepath = os.path.join(self.validation_dir, filename + '.valid.ndjson')
    json_lines = testdata_utils.read_data(filepath, delimiter='\n')
    json_values = [
        json.loads(x, parse_float=decimal.Decimal, parse_int=decimal.Decimal)
        for x in json_lines
    ]

    for value in json_values:
      self.primitive_handler.primitive_wrapper_from_json_value(
          value, primitive_cls)

  def assert_json_validation_with_invalid_primitive_raises(
      self, primitive_cls: Type[message.Message]) -> None:
    """Performs a suite of validation tests on invalid FHIR primitives."""
    filename = path_utils.camel_case_to_snake_case(
        primitive_cls.DESCRIPTOR.name)
    filepath = os.path.join(self.validation_dir, filename + '.invalid.ndjson')
    json_lines = testdata_utils.read_data(filepath, delimiter='\n')
    json_values = [
        json.loads(x, parse_float=decimal.Decimal, parse_int=decimal.Decimal)
        for x in json_lines
    ]

    for value in json_values:
      with self.assertRaises(fhir_errors.InvalidFhirError):
        self.primitive_handler.primitive_wrapper_from_json_value(
            value, primitive_cls)


class DateTimeWrapperTest(absltest.TestCase, metaclass=abc.ABCMeta):
  """Tests the DateTimeWrapper class on specific parsing/printing scenarios."""

  @property
  @abc.abstractmethod
  def primitive_handler(self) -> primitive_handler.PrimitiveHandler:
    pass

  def test_parse_date_time_with_year_precision_succeeds(self):
    datetime_str = '1971'
    expected = self.primitive_handler.new_date_time(
        value_us=31536000000000,
        precision=_primitive_time_utils.DateTimePrecision.YEAR,
        timezone='Z')
    wrapper = self.primitive_handler.primitive_wrapper_from_json_value(
        datetime_str, self.primitive_handler.date_time_cls)
    self.assertEqual(wrapper.string_value(), '1971')
    self.assertEqual(wrapper.wrapped, expected)

    expected_alt_timezone = self.primitive_handler.new_date_time(
        value_us=31500000000000,
        precision=_primitive_time_utils.DateTimePrecision.YEAR,
        timezone='Australia/Sydney')
    wrapper = self.primitive_handler.primitive_wrapper_from_json_value(
        datetime_str,
        self.primitive_handler.date_time_cls,
        default_timezone='Australia/Sydney')
    self.assertEqual(wrapper.string_value(), '1971')
    self.assertEqual(wrapper.wrapped, expected_alt_timezone)

  def test_parse_date_time_with_month_precision_succeeds(self):
    datetime_str = '1970-02'
    expected = self.primitive_handler.new_date_time(
        value_us=2678400000000,
        precision=_primitive_time_utils.DateTimePrecision.MONTH,
        timezone='Z')
    wrapper = self.primitive_handler.primitive_wrapper_from_json_value(
        datetime_str, self.primitive_handler.date_time_cls)
    self.assertEqual(wrapper.string_value(), '1970-02')
    self.assertEqual(wrapper.wrapped, expected)

    expected_alt_timezone = self.primitive_handler.new_date_time(
        value_us=2642400000000,
        precision=_primitive_time_utils.DateTimePrecision.MONTH,
        timezone='Australia/Sydney')
    wrapper = self.primitive_handler.primitive_wrapper_from_json_value(
        datetime_str,
        self.primitive_handler.date_time_cls,
        default_timezone='Australia/Sydney')
    self.assertEqual(wrapper.string_value(), '1970-02')
    self.assertEqual(wrapper.wrapped, expected_alt_timezone)

  def test_parse_date_time_with_day_precision_succeeds(self):
    datetime_str = '1970-01-01'
    expected = self.primitive_handler.new_date_time(
        value_us=0,
        precision=_primitive_time_utils.DateTimePrecision.DAY,
        timezone='Z')
    wrapper = self.primitive_handler.primitive_wrapper_from_json_value(
        datetime_str, self.primitive_handler.date_time_cls)
    self.assertEqual(wrapper.string_value(), '1970-01-01')
    self.assertEqual(wrapper.wrapped, expected)

    expected_alt_timezone = self.primitive_handler.new_date_time(
        value_us=-36000000000,
        precision=_primitive_time_utils.DateTimePrecision.DAY,
        timezone='Australia/Sydney')
    wrapper = self.primitive_handler.primitive_wrapper_from_json_value(
        datetime_str,
        self.primitive_handler.date_time_cls,
        default_timezone='Australia/Sydney')
    self.assertEqual(wrapper.string_value(), '1970-01-01')
    self.assertEqual(wrapper.wrapped, expected_alt_timezone)

  def test_parse_date_time_with_second_precision_succeeds(self):
    datetime_str = '2014-10-09T14:58:00+11:00'
    expected = self.primitive_handler.new_date_time(
        value_us=1412827080000000,
        precision=_primitive_time_utils.DateTimePrecision.SECOND,
        timezone='+11:00')
    wrapper = self.primitive_handler.primitive_wrapper_from_json_value(
        datetime_str, self.primitive_handler.date_time_cls)
    self.assertEqual(wrapper.string_value(), '2014-10-09T14:58:00+11:00')
    self.assertEqual(wrapper.wrapped, expected)

  def test_parse_date_time_with_millisecond_precision_succeeds(self):
    datetime_str = '1970-01-01T12:00:00.123Z'
    expected = self.primitive_handler.new_date_time(
        value_us=43200123000,
        precision=_primitive_time_utils.DateTimePrecision.MILLISECOND,
        timezone='Z')
    wrapper = self.primitive_handler.primitive_wrapper_from_json_value(
        datetime_str, self.primitive_handler.date_time_cls)
    self.assertEqual(wrapper.string_value(), '1970-01-01T12:00:00.123Z')
    self.assertEqual(wrapper.wrapped, expected)

    datetime_str = '1970-01-01T12:00:00.123+00:00'
    expected = self.primitive_handler.new_date_time(
        value_us=43200123000,
        precision=_primitive_time_utils.DateTimePrecision.MILLISECOND,
        timezone='+00:00')
    wrapper = self.primitive_handler.primitive_wrapper_from_json_value(
        datetime_str, self.primitive_handler.date_time_cls)
    self.assertEqual(wrapper.string_value(), '1970-01-01T12:00:00.123+00:00')
    self.assertEqual(wrapper.wrapped, expected)

    datetime_str = '1970-01-01T12:00:00.123-00:00'
    expected = self.primitive_handler.new_date_time(
        value_us=43200123000,
        precision=_primitive_time_utils.DateTimePrecision.MILLISECOND,
        timezone='-00:00')
    wrapper = self.primitive_handler.primitive_wrapper_from_json_value(
        datetime_str, self.primitive_handler.date_time_cls)
    self.assertEqual(wrapper.string_value(), '1970-01-01T12:00:00.123-00:00')
    self.assertEqual(wrapper.wrapped, expected)

  def test_parse_date_time_with_microsecond_precision_succeeds(self):
    datetime_str = '1970-01-01T12:00:00.123456Z'
    expected = self.primitive_handler.new_date_time(
        value_us=43200123456,
        precision=_primitive_time_utils.DateTimePrecision.MICROSECOND,
        timezone='Z')
    wrapper = self.primitive_handler.primitive_wrapper_from_json_value(
        datetime_str, self.primitive_handler.date_time_cls)
    self.assertEqual(wrapper.string_value(), '1970-01-01T12:00:00.123456Z')
    self.assertEqual(wrapper.wrapped, expected)

    datetime_str = '1970-01-01T12:00:00.123456+00:00'
    expected = self.primitive_handler.new_date_time(
        value_us=43200123456,
        precision=_primitive_time_utils.DateTimePrecision.MICROSECOND,
        timezone='+00:00')
    wrapper = self.primitive_handler.primitive_wrapper_from_json_value(
        datetime_str, self.primitive_handler.date_time_cls)
    self.assertEqual(wrapper.string_value(), '1970-01-01T12:00:00.123456+00:00')
    self.assertEqual(wrapper.wrapped, expected)

    datetime_str = '1970-01-01T12:00:00.123000-00:00'
    expected = self.primitive_handler.new_date_time(
        value_us=43200123000,
        precision=_primitive_time_utils.DateTimePrecision.MICROSECOND,
        timezone='-00:00')
    wrapper = self.primitive_handler.primitive_wrapper_from_json_value(
        datetime_str, self.primitive_handler.date_time_cls)
    self.assertEqual(wrapper.string_value(), '1970-01-01T12:00:00.123000-00:00')
    self.assertEqual(wrapper.wrapped, expected)

  def test_print_date_time_with_year_precision_succeeds(self):
    primitive = self.primitive_handler.new_date_time(
        value_us=0,
        precision=_primitive_time_utils.DateTimePrecision.YEAR,
        timezone='Z')
    wrapper = self.primitive_handler.primitive_wrapper_from_primitive(primitive)
    self.assertEqual(wrapper.string_value(), '1970')

    primitive = self.primitive_handler.new_date_time(
        value_us=-36000000000,
        precision=_primitive_time_utils.DateTimePrecision.YEAR,
        timezone='Australia/Sydney')
    wrapper = self.primitive_handler.primitive_wrapper_from_primitive(primitive)
    self.assertEqual(wrapper.string_value(), '1970')

  def test_print_date_time_with_month_precision_succeeds(self):
    primitive = self.primitive_handler.new_date_time(
        value_us=0,
        precision=_primitive_time_utils.DateTimePrecision.MONTH,
        timezone='Z')
    wrapper = self.primitive_handler.primitive_wrapper_from_primitive(primitive)
    self.assertEqual(wrapper.string_value(), '1970-01')

    primitive = self.primitive_handler.new_date_time(
        value_us=-36000000000,
        precision=_primitive_time_utils.DateTimePrecision.MONTH,
        timezone='Australia/Sydney')
    wrapper = self.primitive_handler.primitive_wrapper_from_primitive(primitive)
    self.assertEqual(wrapper.string_value(), '1970-01')

  def test_print_date_time_with_day_precision_succeeds(self):
    primitive = self.primitive_handler.new_date_time(
        value_us=0,
        precision=_primitive_time_utils.DateTimePrecision.DAY,
        timezone='Z')
    wrapper = self.primitive_handler.primitive_wrapper_from_primitive(primitive)
    self.assertEqual(wrapper.string_value(), '1970-01-01')

    primitive = self.primitive_handler.new_date_time(
        value_us=-36000000000,
        precision=_primitive_time_utils.DateTimePrecision.DAY,
        timezone='Australia/Sydney')
    wrapper = self.primitive_handler.primitive_wrapper_from_primitive(primitive)
    self.assertEqual(wrapper.string_value(), '1970-01-01')

  def test_print_date_time_with_second_precision_succeeds(self):
    primitive = self.primitive_handler.new_date_time(
        value_us=1412827080000000,
        precision=_primitive_time_utils.DateTimePrecision.SECOND,
        timezone='+11:00')
    wrapper = self.primitive_handler.primitive_wrapper_from_primitive(primitive)
    self.assertEqual(wrapper.string_value(), '2014-10-09T14:58:00+11:00')

    primitive = self.primitive_handler.new_date_time(
        value_us=1412827080000000,
        precision=_primitive_time_utils.DateTimePrecision.SECOND,
        timezone='Australia/Sydney')
    wrapper = self.primitive_handler.primitive_wrapper_from_primitive(primitive)
    self.assertEqual(wrapper.string_value(), '2014-10-09T14:58:00+11:00')

  def test_print_date_time_with_millisecond_precision_succeeds(self):
    primitive = self.primitive_handler.new_date_time(
        value_us=43200123000,
        precision=_primitive_time_utils.DateTimePrecision.MILLISECOND,
        timezone='Z')
    wrapper = self.primitive_handler.primitive_wrapper_from_primitive(primitive)
    self.assertEqual(wrapper.string_value(), '1970-01-01T12:00:00.123Z')

    primitive = self.primitive_handler.new_date_time(
        value_us=43200123000,
        precision=_primitive_time_utils.DateTimePrecision.MILLISECOND,
        timezone='UTC')
    wrapper = self.primitive_handler.primitive_wrapper_from_primitive(primitive)
    self.assertEqual(wrapper.string_value(), '1970-01-01T12:00:00.123Z')

    primitive = self.primitive_handler.new_date_time(
        value_us=43200123000,
        precision=_primitive_time_utils.DateTimePrecision.MILLISECOND,
        timezone='-00:00')
    wrapper = self.primitive_handler.primitive_wrapper_from_primitive(primitive)
    self.assertEqual(wrapper.string_value(), '1970-01-01T12:00:00.123-00:00')

    primitive = self.primitive_handler.new_date_time(
        value_us=43200123000,
        precision=_primitive_time_utils.DateTimePrecision.MILLISECOND,
        timezone='+00:00')
    wrapper = self.primitive_handler.primitive_wrapper_from_primitive(primitive)
    self.assertEqual(wrapper.string_value(), '1970-01-01T12:00:00.123+00:00')

  def test_print_date_time_with_microsecond_precision_succeeds(self):
    primitive = self.primitive_handler.new_date_time(
        value_us=43200123456,
        precision=_primitive_time_utils.DateTimePrecision.MICROSECOND,
        timezone='Z')
    wrapper = self.primitive_handler.primitive_wrapper_from_primitive(primitive)
    self.assertEqual(wrapper.string_value(), '1970-01-01T12:00:00.123456Z')

    primitive = self.primitive_handler.new_date_time(
        value_us=43200123456,
        precision=_primitive_time_utils.DateTimePrecision.MICROSECOND,
        timezone='UTC')
    wrapper = self.primitive_handler.primitive_wrapper_from_primitive(primitive)
    self.assertEqual(wrapper.string_value(), '1970-01-01T12:00:00.123456Z')

    primitive = self.primitive_handler.new_date_time(
        value_us=43200123456,
        precision=_primitive_time_utils.DateTimePrecision.MICROSECOND,
        timezone='-00:00')
    wrapper = self.primitive_handler.primitive_wrapper_from_primitive(primitive)
    self.assertEqual(wrapper.string_value(), '1970-01-01T12:00:00.123456-00:00')

    primitive = self.primitive_handler.new_date_time(
        value_us=43200123456,
        precision=_primitive_time_utils.DateTimePrecision.MICROSECOND,
        timezone='+00:00')
    wrapper = self.primitive_handler.primitive_wrapper_from_primitive(primitive)
    self.assertEqual(wrapper.string_value(), '1970-01-01T12:00:00.123456+00:00')


class DateWrapperTest(absltest.TestCase, metaclass=abc.ABCMeta):
  """Tests the DateWrapper class on specific parsing/printing scenarios."""

  @property
  @abc.abstractmethod
  def primitive_handler(self) -> primitive_handler.PrimitiveHandler:
    pass

  def test_parse_date_with_year_precision_succeeds(self):
    date_str = '1971'
    expected = self.primitive_handler.new_date(
        value_us=31536000000000,
        precision=_primitive_time_utils.DateTimePrecision.YEAR,
        timezone='Z')
    wrapper = self.primitive_handler.primitive_wrapper_from_json_value(
        date_str, self.primitive_handler.date_cls)
    self.assertEqual(wrapper.string_value(), '1971')
    self.assertEqual(wrapper.wrapped, expected)

    expected = self.primitive_handler.new_date(
        value_us=31500000000000,
        precision=_primitive_time_utils.DateTimePrecision.YEAR,
        timezone='Australia/Sydney')
    wrapper = self.primitive_handler.primitive_wrapper_from_json_value(
        date_str,
        self.primitive_handler.date_cls,
        default_timezone='Australia/Sydney')
    self.assertEqual(wrapper.string_value(), '1971')
    self.assertEqual(wrapper.wrapped, expected)

  def test_parse_date_with_month_precision_succeeds(self):
    date_str = '1970-02'
    expected = self.primitive_handler.new_date(
        value_us=2678400000000,
        precision=_primitive_time_utils.DateTimePrecision.MONTH,
        timezone='Z')
    wrapper = self.primitive_handler.primitive_wrapper_from_json_value(
        date_str, self.primitive_handler.date_cls)
    self.assertEqual(wrapper.string_value(), '1970-02')
    self.assertEqual(wrapper.wrapped, expected)

    expected = self.primitive_handler.new_date(
        value_us=2642400000000,
        precision=_primitive_time_utils.DateTimePrecision.MONTH,
        timezone='Australia/Sydney')
    wrapper = self.primitive_handler.primitive_wrapper_from_json_value(
        date_str,
        self.primitive_handler.date_cls,
        default_timezone='Australia/Sydney')
    self.assertEqual(wrapper.string_value(), '1970-02')
    self.assertEqual(wrapper.wrapped, expected)

  def test_parse_date_with_day_precision_succeeds(self):
    date_str = '1970-01-01'
    expected = self.primitive_handler.new_date(
        value_us=0,
        precision=_primitive_time_utils.DateTimePrecision.DAY,
        timezone='Z')
    wrapper = self.primitive_handler.primitive_wrapper_from_json_value(
        date_str, self.primitive_handler.date_cls)
    self.assertEqual(wrapper.string_value(), '1970-01-01')
    self.assertEqual(wrapper.wrapped, expected)

    expected = self.primitive_handler.new_date(
        value_us=-36000000000,
        precision=_primitive_time_utils.DateTimePrecision.DAY,
        timezone='Australia/Sydney')
    wrapper = self.primitive_handler.primitive_wrapper_from_json_value(
        date_str,
        self.primitive_handler.date_cls,
        default_timezone='Australia/Sydney')
    self.assertEqual(wrapper.string_value(), '1970-01-01')
    self.assertEqual(wrapper.wrapped, expected)

    expected = self.primitive_handler.new_date(
        value_us=18000000000,
        precision=_primitive_time_utils.DateTimePrecision.DAY,
        timezone='-05:00')
    wrapper = self.primitive_handler.primitive_wrapper_from_json_value(
        date_str, self.primitive_handler.date_cls, default_timezone='-05:00')
    self.assertEqual(wrapper.string_value(), '1970-01-01')
    self.assertEqual(wrapper.wrapped, expected)

  def test_print_date_with_year_precision_succeeds(self):
    primitive = self.primitive_handler.new_date(
        value_us=0,
        precision=_primitive_time_utils.DateTimePrecision.YEAR,
        timezone='Z')
    wrapper = self.primitive_handler.primitive_wrapper_from_primitive(primitive)
    self.assertEqual(wrapper.string_value(), '1970')

    primitive = self.primitive_handler.new_date(
        value_us=-36000000000,
        precision=_primitive_time_utils.DateTimePrecision.YEAR,
        timezone='Australia/Sydney')
    wrapper = self.primitive_handler.primitive_wrapper_from_primitive(primitive)
    self.assertEqual(wrapper.string_value(), '1970')

  def test_print_date_with_month_precision_succeeds(self):
    primitive = self.primitive_handler.new_date(
        value_us=0,
        precision=_primitive_time_utils.DateTimePrecision.MONTH,
        timezone='Z')
    wrapper = self.primitive_handler.primitive_wrapper_from_primitive(primitive)
    self.assertEqual(wrapper.string_value(), '1970-01')

    primitive = self.primitive_handler.new_date(
        value_us=-36000000000,
        precision=_primitive_time_utils.DateTimePrecision.MONTH,
        timezone='Australia/Sydney')
    wrapper = self.primitive_handler.primitive_wrapper_from_primitive(primitive)
    self.assertEqual(wrapper.string_value(), '1970-01')

  def test_print_date_with_day_precision_succeeds(self):
    primitive = self.primitive_handler.new_date(
        value_us=0,
        precision=_primitive_time_utils.DateTimePrecision.DAY,
        timezone='Z')
    wrapper = self.primitive_handler.primitive_wrapper_from_primitive(primitive)
    self.assertEqual(wrapper.string_value(), '1970-01-01')

    primitive = self.primitive_handler.new_date(
        value_us=-36000000000,
        precision=_primitive_time_utils.DateTimePrecision.DAY,
        timezone='Australia/Sydney')
    wrapper = self.primitive_handler.primitive_wrapper_from_primitive(primitive)
    self.assertEqual(wrapper.string_value(), '1970-01-01')


class InstantWrapperTest(absltest.TestCase, metaclass=abc.ABCMeta):
  """Tests the InstantWrapper class on specific parsing/printing scenarios."""

  @property
  @abc.abstractmethod
  def primitive_handler(self) -> primitive_handler.PrimitiveHandler:
    pass

  def test_parse_instant_with_second_precision_succeeds(self):
    instant_str = '1970-01-01T00:00:00Z'
    expected = self.primitive_handler.new_instant(
        value_us=0,
        precision=_primitive_time_utils.TimePrecision.SECOND,
        timezone='Z')
    wrapper = self.primitive_handler.primitive_wrapper_from_json_value(
        instant_str, self.primitive_handler.instant_cls)
    self.assertEqual(wrapper.string_value(), '1970-01-01T00:00:00Z')
    self.assertEqual(wrapper.wrapped, expected)

    instant_str = '1970-01-01T00:00:00+00:00'
    expected = self.primitive_handler.new_instant(
        value_us=0,
        precision=_primitive_time_utils.TimePrecision.SECOND,
        timezone='+00:00')
    wrapper = self.primitive_handler.primitive_wrapper_from_json_value(
        instant_str, self.primitive_handler.instant_cls)
    self.assertEqual(wrapper.string_value(), '1970-01-01T00:00:00+00:00')
    self.assertEqual(wrapper.wrapped, expected)

    instant_str = '1970-01-01T00:00:00-00:00'
    expected = self.primitive_handler.new_instant(
        value_us=0,
        precision=_primitive_time_utils.TimePrecision.SECOND,
        timezone='-00:00')
    wrapper = self.primitive_handler.primitive_wrapper_from_json_value(
        instant_str, self.primitive_handler.instant_cls)
    self.assertEqual(wrapper.string_value(), '1970-01-01T00:00:00-00:00')
    self.assertEqual(wrapper.wrapped, expected)

  def test_parse_instant_with_millisecond_precision_succeeds(self):
    instant_str = '1970-01-01T00:00:00.123Z'
    expected = self.primitive_handler.new_instant(
        value_us=123000,
        precision=_primitive_time_utils.TimePrecision.MILLISECOND,
        timezone='Z')
    wrapper = self.primitive_handler.primitive_wrapper_from_json_value(
        instant_str, self.primitive_handler.instant_cls)
    self.assertEqual(wrapper.string_value(), '1970-01-01T00:00:00.123Z')
    self.assertEqual(wrapper.wrapped, expected)

    instant_str = '1970-01-01T00:00:00.123+00:00'
    expected = self.primitive_handler.new_instant(
        value_us=123000,
        precision=_primitive_time_utils.TimePrecision.MILLISECOND,
        timezone='+00:00')
    wrapper = self.primitive_handler.primitive_wrapper_from_json_value(
        instant_str, self.primitive_handler.instant_cls)
    self.assertEqual(wrapper.string_value(), '1970-01-01T00:00:00.123+00:00')
    self.assertEqual(wrapper.wrapped, expected)

    instant_str = '1970-01-01T00:00:00.123-00:00'
    expected = self.primitive_handler.new_instant(
        value_us=123000,
        precision=_primitive_time_utils.TimePrecision.MILLISECOND,
        timezone='-00:00')
    wrapper = self.primitive_handler.primitive_wrapper_from_json_value(
        instant_str, self.primitive_handler.instant_cls)
    self.assertEqual(wrapper.string_value(), '1970-01-01T00:00:00.123-00:00')
    self.assertEqual(wrapper.wrapped, expected)

  def test_parse_instant_with_microsecond_precision_succeeds(self):
    instant_str = '1970-01-01T00:00:00.123000Z'
    expected = self.primitive_handler.new_instant(
        value_us=123000,
        precision=_primitive_time_utils.TimePrecision.MICROSECOND,
        timezone='Z')
    wrapper = self.primitive_handler.primitive_wrapper_from_json_value(
        instant_str, self.primitive_handler.instant_cls)
    self.assertEqual(wrapper.string_value(), '1970-01-01T00:00:00.123000Z')
    self.assertEqual(wrapper.wrapped, expected)

    instant_str = '1970-01-01T00:00:00.123000+00:00'
    expected = self.primitive_handler.new_instant(
        value_us=123000,
        precision=_primitive_time_utils.TimePrecision.MICROSECOND,
        timezone='+00:00')
    wrapper = self.primitive_handler.primitive_wrapper_from_json_value(
        instant_str, self.primitive_handler.instant_cls)
    self.assertEqual(wrapper.string_value(), '1970-01-01T00:00:00.123000+00:00')
    self.assertEqual(wrapper.wrapped, expected)

    instant_str = '1970-01-01T00:00:00.123000-00:00'
    expected = self.primitive_handler.new_instant(
        value_us=123000,
        precision=_primitive_time_utils.TimePrecision.MICROSECOND,
        timezone='-00:00')
    wrapper = self.primitive_handler.primitive_wrapper_from_json_value(
        instant_str, self.primitive_handler.instant_cls)
    self.assertEqual(wrapper.string_value(), '1970-01-01T00:00:00.123000-00:00')
    self.assertEqual(wrapper.wrapped, expected)


class TimeWrapperTest(absltest.TestCase, metaclass=abc.ABCMeta):
  """Tests the TimeWrapper class on specific parsing/printing scenarios."""

  @property
  @abc.abstractmethod
  def primitive_handler(self) -> primitive_handler.PrimitiveHandler:
    pass

  def test_parse_time_with_second_precision_succeeds(self):
    timestamp = '12:00:00'
    expected = self.primitive_handler.new_time(
        value_us=43200000000,
        precision=_primitive_time_utils.TimePrecision.SECOND)
    wrapper = self.primitive_handler.primitive_wrapper_from_json_value(
        timestamp, self.primitive_handler.time_cls)
    self.assertEqual(wrapper.string_value(), '12:00:00')
    self.assertEqual(wrapper.wrapped, expected)

  def test_parse_time_with_millisecond_precision_succeeds(self):
    timestamp = '12:00:00.123'
    expected = self.primitive_handler.new_time(
        value_us=43200123000,
        precision=_primitive_time_utils.TimePrecision.MILLISECOND)
    wrapper = self.primitive_handler.primitive_wrapper_from_json_value(
        timestamp, self.primitive_handler.time_cls)
    self.assertEqual(wrapper.string_value(), '12:00:00.123')
    self.assertEqual(wrapper.wrapped, expected)

  def test_parse_time_with_microsecond_precision_succeeds(self):
    timestamp = '12:00:00.123000'
    expected = self.primitive_handler.new_time(
        value_us=43200123000,
        precision=_primitive_time_utils.TimePrecision.MICROSECOND)
    wrapper = self.primitive_handler.primitive_wrapper_from_json_value(
        timestamp, self.primitive_handler.time_cls)
    self.assertEqual(wrapper.string_value(), '12:00:00.123000')
    self.assertEqual(wrapper.wrapped, expected)

  def test_print_time_with_second_precision_succeeds(self):
    primitive = self.primitive_handler.new_time(
        value_us=43200000000,
        precision=_primitive_time_utils.TimePrecision.SECOND)
    wrapper = self.primitive_handler.primitive_wrapper_from_primitive(primitive)
    self.assertEqual(wrapper.string_value(), '12:00:00')

  def test_print_time_with_millisecond_precision_succeeds(self):
    primitive = self.primitive_handler.new_time(
        value_us=43200123000,
        precision=_primitive_time_utils.TimePrecision.MILLISECOND)
    wrapper = self.primitive_handler.primitive_wrapper_from_primitive(primitive)
    self.assertEqual(wrapper.string_value(), '12:00:00.123')

  def test_print_time_with_microsecond_precision_succeeds(self):
    primitive = self.primitive_handler.new_time(
        value_us=43200123000,
        precision=_primitive_time_utils.TimePrecision.MICROSECOND)
    wrapper = self.primitive_handler.primitive_wrapper_from_primitive(primitive)
    self.assertEqual(wrapper.string_value(), '12:00:00.123000')


class DecimalWrapperTest(absltest.TestCase, metaclass=abc.ABCMeta):
  """Tests the DecimalWrapper class on specific parsing/printing scenarios."""

  @property
  @abc.abstractmethod
  def primitive_handler(self) -> primitive_handler.PrimitiveHandler:
    pass

  def test_parse_decimal_with_positive_integer_succeeds(self):
    expected = self.primitive_handler.new_decimal(value='185')
    wrapper = self.primitive_handler.primitive_wrapper_from_json_value(
        decimal.Decimal('185'), self.primitive_handler.decimal_cls)
    self.assertEqual(wrapper.string_value(), '185')
    self.assertEqual(wrapper.wrapped, expected)

    expected = self.primitive_handler.new_decimal(value='100')
    wrapper = self.primitive_handler.primitive_wrapper_from_json_value(
        decimal.Decimal('100'), self.primitive_handler.decimal_cls)
    self.assertEqual(wrapper.string_value(), '100')
    self.assertEqual(wrapper.wrapped, expected)

  def test_parse_decimal_with_negative_integer_succeeds(self):
    expected = self.primitive_handler.new_decimal(value='-40')
    wrapper = self.primitive_handler.primitive_wrapper_from_json_value(
        decimal.Decimal('-40'), self.primitive_handler.decimal_cls)
    self.assertEqual(wrapper.string_value(), '-40')
    self.assertEqual(wrapper.wrapped, expected)

  def test_parse_decimal_with_positive_real_succeeds(self):
    expected = self.primitive_handler.new_decimal(value='0.0099')
    wrapper = self.primitive_handler.primitive_wrapper_from_json_value(
        decimal.Decimal('0.0099'), self.primitive_handler.decimal_cls)
    self.assertEqual(wrapper.string_value(), '0.0099')
    self.assertEqual(wrapper.wrapped, expected)

    expected = self.primitive_handler.new_decimal(value='100.00')
    wrapper = self.primitive_handler.primitive_wrapper_from_json_value(
        decimal.Decimal('100.00'), self.primitive_handler.decimal_cls)
    self.assertEqual(wrapper.string_value(), '100.00')
    self.assertEqual(wrapper.wrapped, expected)

  def test_parse_decimal_with_zero_succeeds(self):
    expected = self.primitive_handler.new_decimal(value='0')
    wrapper = self.primitive_handler.primitive_wrapper_from_json_value(
        decimal.Decimal('0'), self.primitive_handler.decimal_cls)
    self.assertEqual(wrapper.string_value(), '0')
    self.assertEqual(wrapper.wrapped, expected)

  def test_parse_decimal_with_high_precision_real_succeeds(self):
    expected = self.primitive_handler.new_decimal(value='1.00065022141624642')
    wrapper = self.primitive_handler.primitive_wrapper_from_json_value(
        decimal.Decimal('1.00065022141624642'),
        self.primitive_handler.decimal_cls)
    self.assertEqual(wrapper.string_value(), '1.00065022141624642')
    self.assertEqual(wrapper.wrapped, expected)

  def test_print_decimal_with_positive_integer_succeeds(self):
    primitive = self.primitive_handler.new_decimal(value='185')
    wrapper = self.primitive_handler.primitive_wrapper_from_primitive(primitive)
    self.assertEqual(wrapper.string_value(), '185')

    primitive = self.primitive_handler.new_decimal(value='100')
    wrapper = self.primitive_handler.primitive_wrapper_from_primitive(primitive)
    self.assertEqual(wrapper.string_value(), '100')

  def test_print_decimal_with_negative_integer_succeeds(self):
    primitive = self.primitive_handler.new_decimal(value='-40')
    wrapper = self.primitive_handler.primitive_wrapper_from_primitive(primitive)
    self.assertEqual(wrapper.string_value(), '-40')

  def test_print_decimal_with_positive_real_succeeds(self):
    primitive = self.primitive_handler.new_decimal(value='0.0099')
    wrapper = self.primitive_handler.primitive_wrapper_from_primitive(primitive)
    self.assertEqual(wrapper.string_value(), '0.0099')

    primitive = self.primitive_handler.new_decimal(value='100.00')
    wrapper = self.primitive_handler.primitive_wrapper_from_primitive(primitive)
    self.assertEqual(wrapper.string_value(), '100.00')

  def test_print_decimal_with_zero_succeeds(self):
    primitive = self.primitive_handler.new_decimal(value='0')
    wrapper = self.primitive_handler.primitive_wrapper_from_primitive(primitive)
    self.assertEqual(wrapper.string_value(), '0')

    primitive = self.primitive_handler.new_decimal(value='0.00')
    wrapper = self.primitive_handler.primitive_wrapper_from_primitive(primitive)
    self.assertEqual(wrapper.string_value(), '0.00')
