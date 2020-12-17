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
from typing import cast, Any, Type

from google.protobuf import message
from absl.testing import absltest
from google.fhir import _primitive_time_utils
from google.fhir import extensions
from google.fhir import fhir_errors
from google.fhir import primitive_handler
from google.fhir.testing import testdata_utils
from google.fhir.utils import path_utils
from google.fhir.utils import proto_utils


class PrimitiveWrapperPrimitiveHasNoValueTest(
    absltest.TestCase, metaclass=abc.ABCMeta):
  """A suite of tests to ensure proper validation for PrimitiveHasNoValue."""

  @property
  @abc.abstractmethod
  def primitive_handler(self) -> primitive_handler.PrimitiveHandler:
    raise NotImplementedError('Subclasses *must* implement primitive_handler.')

  def testPrimitiveHasNoValue_withValidBase64Binary_succeeds(self):
    self.assert_set_valid_primitive_has_no_value_succeeds(
        self.primitive_handler.base64_binary_cls)

  def testPrimitiveHasNoValue_withInvalidBase64Binary_raises(self):
    self.assert_set_invalid_primitive_has_no_value_raises(
        self.primitive_handler.base64_binary_cls)

  def testPrimitiveHasNoValue_withValidBoolean_succeeds(self):
    self.assert_set_valid_primitive_has_no_value_succeeds(
        self.primitive_handler.boolean_cls)

  def testPrimitiveHasNoValue_withInvalidBoolean_raises(self):
    self.assert_set_invalid_primitive_has_no_value_raises(
        self.primitive_handler.boolean_cls)

  def testPrimitiveHasNoValue_withValidCode_succeeds(self):
    self.assert_set_valid_primitive_has_no_value_succeeds(
        self.primitive_handler.code_cls)

  def testPrimitiveHasNoValue_withInvalidCode_raises(self):
    self.assert_set_invalid_primitive_has_no_value_raises(
        self.primitive_handler.code_cls)

  def testPrimitiveHasNoValue_withValidDate_succeeds(self):
    self.assert_set_valid_primitive_has_no_value_succeeds(
        self.primitive_handler.date_cls)

  def testPrimitiveHasNoValue_withInvalidDate_raises(self):
    self.assert_set_invalid_primitive_has_no_value_raises(
        self.primitive_handler.date_cls)

  def testPrimitiveHasNoValue_withValidDateTime_succeeds(self):
    self.assert_set_valid_primitive_has_no_value_succeeds(
        self.primitive_handler.date_time_cls)

  def testPrimitiveHasNoValue_withInvalidDateTime_raises(self):
    self.assert_set_invalid_primitive_has_no_value_raises(
        self.primitive_handler.date_time_cls)

  def testPrimitiveHasNoValue_withValidDecimal_succeeds(self):
    self.assert_set_valid_primitive_has_no_value_succeeds(
        self.primitive_handler.decimal_cls)

  def testPrimitiveHasNoValue_withInvalidDecimal_raises(self):
    self.assert_set_invalid_primitive_has_no_value_raises(
        self.primitive_handler.decimal_cls)

  def testPrimitiveHasNoValue_withValidId_succeeds(self):
    self.assert_set_valid_primitive_has_no_value_succeeds(
        self.primitive_handler.id_cls)

  def testPrimitiveHasNoValue_withInvalidId_raises(self):
    self.assert_set_invalid_primitive_has_no_value_raises(
        self.primitive_handler.id_cls)

  def testPrimitiveHasNoValue_withValidInstant_succeeds(self):
    self.assert_set_valid_primitive_has_no_value_succeeds(
        self.primitive_handler.instant_cls)

  def testPrimitiveHasNoValue_withInvalidInstant_raises(self):
    self.assert_set_invalid_primitive_has_no_value_raises(
        self.primitive_handler.instant_cls)

  def testPrimitiveHasNoValue_withValidInteger_succeeds(self):
    self.assert_set_valid_primitive_has_no_value_succeeds(
        self.primitive_handler.integer_cls)

  def testPrimitiveHasNoValue_withInvalidInteger_raises(self):
    self.assert_set_invalid_primitive_has_no_value_raises(
        self.primitive_handler.integer_cls)

  def testPrimitiveHasNoValue_withValidMarkdown_succeeds(self):
    self.assert_set_valid_primitive_has_no_value_succeeds(
        self.primitive_handler.markdown_cls)

  def testPrimitiveHasNoValue_withInvalidMarkdown_raises(self):
    self.assert_set_invalid_primitive_has_no_value_raises(
        self.primitive_handler.markdown_cls)

  def testPrimitiveHasNoValue_withValidOid_succeeds(self):
    self.assert_set_valid_primitive_has_no_value_succeeds(
        self.primitive_handler.oid_cls)

  def testPrimitiveHasNoValue_withInvalidOid_raises(self):
    self.assert_set_invalid_primitive_has_no_value_raises(
        self.primitive_handler.oid_cls)

  def testPrimitiveHasNoValue_withValidPositiveInt_succeeds(self):
    self.assert_set_valid_primitive_has_no_value_succeeds(
        self.primitive_handler.positive_int_cls)

  def testPrimitiveHasNoValue_withInvalidPositiveInt_raises(self):
    self.assert_set_invalid_primitive_has_no_value_raises(
        self.primitive_handler.positive_int_cls)

  def testPrimitiveHasNoValue_withValidString_succeeds(self):
    self.assert_set_valid_primitive_has_no_value_succeeds(
        self.primitive_handler.string_cls)

  def testPrimitiveHasNoValue_withInvalidString_raises(self):
    self.assert_set_invalid_primitive_has_no_value_raises(
        self.primitive_handler.string_cls)

  def testPrimitiveHasNoValue_withValidTime_succeeds(self):
    self.assert_set_valid_primitive_has_no_value_succeeds(
        self.primitive_handler.time_cls)

  def testPrimitiveHasNoValue_withInvalidTime_raises(self):
    self.assert_set_invalid_primitive_has_no_value_raises(
        self.primitive_handler.time_cls)

  def testPrimitiveHasNoValue_withValidUnsignedInt_succeeds(self):
    self.assert_set_valid_primitive_has_no_value_succeeds(
        self.primitive_handler.unsigned_int_cls)

  def testPrimitiveHasNoValue_withInvalidUnsignedInt_raises(self):
    self.assert_set_invalid_primitive_has_no_value_raises(
        self.primitive_handler.unsigned_int_cls)

  def testPrimitiveHasNoValue_withValidUri_succeeds(self):
    self.assert_set_valid_primitive_has_no_value_succeeds(
        self.primitive_handler.uri_cls)

  def _set_primitive_has_no_value_extension(self, primitive: message.Message):
    """Sets the PrimitiveHasNoValue FHIR extension on the provided primitive."""
    extensions_field = primitive.DESCRIPTOR.fields_by_name['extension']
    primitive_has_no_value = extensions.create_primitive_has_no_value(
        extensions_field.message_type)
    proto_utils.set_value_at_field(primitive, 'extension',
                                   [primitive_has_no_value])

  def assert_set_valid_primitive_has_no_value_succeeds(
      self, primitive_cls: Type[message.Message]):
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
      self.fail('PrimitiveHasNoValue validation failed for {}: {}.'.format(
          primitive.DESCRIPTOR.full_name, e))

  def assert_set_invalid_primitive_has_no_value_raises(
      self, primitive_cls: Type[message.Message]):
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
    raise NotImplementedError('Subclasses *must* implement primitive_handler.')

  @property
  @abc.abstractmethod
  def validation_dir(self) -> str:
    raise NotImplementedError('Subclasses *must* implement validation_dir')

  def testValidateWrapped_withValidBase64Binary_succeeds(self):
    self.assert_validation_of_valid_primitive_succeeds(
        self.primitive_handler.base64_binary_cls)

  def testValidateWrapped_withValidBoolean_succeeds(self):
    self.assert_validation_of_valid_primitive_succeeds(
        self.primitive_handler.boolean_cls)

  def testValidateWrapped_withValidCode_succeeds(self):
    self.assert_validation_of_valid_primitive_succeeds(
        self.primitive_handler.code_cls)

  def testValidateWrapped_withInvalidCode_raises(self):
    self.assert_validation_of_invalid_primitive_raises(
        self.primitive_handler.code_cls)

  def testValidateWrapped_withValidDate_succeeds(self):
    self.assert_validation_of_valid_primitive_succeeds(
        self.primitive_handler.date_cls)

  def testValidateWrapped_withInvalidDate_raises(self):
    self.assert_validation_of_invalid_primitive_raises(
        self.primitive_handler.date_cls)

  def testValidateWrapped_withValidDateTime_succeeds(self):
    self.assert_validation_of_valid_primitive_succeeds(
        self.primitive_handler.date_time_cls)

  def testValidateWrapped_withInvalidDateTime_raises(self):
    self.assert_validation_of_invalid_primitive_raises(
        self.primitive_handler.date_time_cls)

  def testValidateWrapped_withValidDecimal_succeeds(self):
    self.assert_validation_of_valid_primitive_succeeds(
        self.primitive_handler.decimal_cls)

  def testValidateWrapped_withInvalidDecimal_raises(self):
    self.assert_validation_of_invalid_primitive_raises(
        self.primitive_handler.decimal_cls)

  def testValidateWrapped_withValidId_succeeds(self):
    self.assert_validation_of_valid_primitive_succeeds(
        self.primitive_handler.id_cls)

  def testValidateWrapped_withInvalidId_raises(self):
    self.assert_validation_of_invalid_primitive_raises(
        self.primitive_handler.id_cls)

  def testValidateWrapped_withValidInstant_succeeds(self):
    self.assert_validation_of_valid_primitive_succeeds(
        self.primitive_handler.instant_cls)

  def testValidateWrapped_withInvalidInstant_raises(self):
    self.assert_validation_of_invalid_primitive_raises(
        self.primitive_handler.instant_cls)

  def testValidateWrapped_withValidInteger_succeeds(self):
    self.assert_validation_of_valid_primitive_succeeds(
        self.primitive_handler.integer_cls)

  def testValidateWrapped_withValidMarkdown_succeeds(self):
    self.assert_validation_of_valid_primitive_succeeds(
        self.primitive_handler.markdown_cls)

  def testValidateWrapped_withValidOid_succeeds(self):
    self.assert_validation_of_valid_primitive_succeeds(
        self.primitive_handler.oid_cls)

  def testValidateWrapped_withInvalidOid_raises(self):
    self.assert_validation_of_invalid_primitive_raises(
        self.primitive_handler.oid_cls)

  def testValidateWrapped_withValidPositiveInt_succeeds(self):
    self.assert_validation_of_valid_primitive_succeeds(
        self.primitive_handler.positive_int_cls)

  def testValidateWrapped_withInvalidPositiveInt_raises(self):
    self.assert_validation_of_invalid_primitive_raises(
        self.primitive_handler.positive_int_cls)

  def testValidateWrapped_withValidString_succeeds(self):
    self.assert_validation_of_valid_primitive_succeeds(
        self.primitive_handler.string_cls)

  def testValidateWrapped_withValidTime_succeeds(self):
    self.assert_validation_of_valid_primitive_succeeds(
        self.primitive_handler.time_cls)

  def testValidateWrapped_withInvalidTime_raises(self):
    self.assert_validation_of_invalid_primitive_raises(
        self.primitive_handler.time_cls)

  def testValidateWrapped_withValidUnsignedInt_succeeds(self):
    self.assert_validation_of_valid_primitive_succeeds(
        self.primitive_handler.unsigned_int_cls)

  def testValidateWrapped_withValidUri_succeeds(self):
    self.assert_validation_of_valid_primitive_succeeds(
        self.primitive_handler.uri_cls)

  def testValidateWrapped_withValidXhtml_succeeds(self):
    self.assert_validation_of_valid_primitive_succeeds(
        self.primitive_handler.xhtml_cls)

  def assert_validation_of_valid_primitive_succeeds(
      self, primitive_cls: Type[message.Message]):
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
        self.fail('{} did not represent valid FHIR: {}.'.format(filename, e))

  def assert_validation_of_invalid_primitive_raises(
      self, primitive_cls: Type[message.Message]):
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
    raise NotImplementedError('Subclasses *must* implement primitive_handler.')

  @property
  @abc.abstractmethod
  def validation_dir(self) -> str:
    raise NotImplementedError('Subclasses *must* implement validation_dir')

  def testValidateWrapped_withValidBase64Binary_succeeds(self):
    self.assert_json_validation_with_valid_primitive_succeeds(
        self.primitive_handler.base64_binary_cls)

  def testValidateWrapped_withInvalidBase64Binary_raises(self):
    self.assert_json_validation_with_invalid_primitive_raises(
        self.primitive_handler.base64_binary_cls)

  def testValidateWrapped_withValidBoolean_succeeds(self):
    self.assert_json_validation_with_valid_primitive_succeeds(
        self.primitive_handler.boolean_cls)

  def testValidateWrapped_withValidCode_succeeds(self):
    self.assert_json_validation_with_valid_primitive_succeeds(
        self.primitive_handler.code_cls)

  def testValidateWrapped_withInvalidCode_raises(self):
    self.assert_json_validation_with_invalid_primitive_raises(
        self.primitive_handler.code_cls)

  def testValidateWrapped_withValidDate_succeeds(self):
    self.assert_json_validation_with_valid_primitive_succeeds(
        self.primitive_handler.date_cls)

  def testValidateWrapped_withInvalidDate_raises(self):
    self.assert_json_validation_with_invalid_primitive_raises(
        self.primitive_handler.date_cls)

  def testValidateWrapped_withValidDateTime_succeeds(self):
    self.assert_json_validation_with_valid_primitive_succeeds(
        self.primitive_handler.date_time_cls)

  def testValidateWrapped_withInvalidDateTime_raises(self):
    self.assert_json_validation_with_invalid_primitive_raises(
        self.primitive_handler.date_time_cls)

  def testValidateWrapped_withValidDecimal_succeeds(self):
    self.assert_json_validation_with_valid_primitive_succeeds(
        self.primitive_handler.decimal_cls)

  def testValidateWrapped_withInvalidDecimal_raises(self):
    self.assert_json_validation_with_invalid_primitive_raises(
        self.primitive_handler.decimal_cls)

  def testValidateWrapped_withValidId_succeeds(self):
    self.assert_json_validation_with_valid_primitive_succeeds(
        self.primitive_handler.id_cls)

  def testValidateWrapped_withInvalidId_raises(self):
    self.assert_json_validation_with_invalid_primitive_raises(
        self.primitive_handler.id_cls)

  def testValidateWrapped_withValidInstant_succeeds(self):
    self.assert_json_validation_with_valid_primitive_succeeds(
        self.primitive_handler.instant_cls)

  def testValidateWrapped_withInvalidInstant_raises(self):
    self.assert_json_validation_with_invalid_primitive_raises(
        self.primitive_handler.instant_cls)

  def testValidateWrapped_withValidInteger_succeeds(self):
    self.assert_json_validation_with_valid_primitive_succeeds(
        self.primitive_handler.integer_cls)

  def testValidateWrapped_withValidMarkdown_succeeds(self):
    self.assert_json_validation_with_valid_primitive_succeeds(
        self.primitive_handler.markdown_cls)

  def testValidateWrapped_withValidOid_succeeds(self):
    self.assert_json_validation_with_valid_primitive_succeeds(
        self.primitive_handler.oid_cls)

  def testValidateWrapped_withInvalidOid_raises(self):
    self.assert_json_validation_with_invalid_primitive_raises(
        self.primitive_handler.oid_cls)

  def testValidateWrapped_withValidPositiveInt_succeeds(self):
    self.assert_json_validation_with_valid_primitive_succeeds(
        self.primitive_handler.positive_int_cls)

  def testValidateWrapped_withInvalidPositiveInt_raises(self):
    self.assert_json_validation_with_invalid_primitive_raises(
        self.primitive_handler.positive_int_cls)

  def testValidateWrapped_withValidString_succeeds(self):
    self.assert_json_validation_with_valid_primitive_succeeds(
        self.primitive_handler.string_cls)

  def testValidateWrapped_withValidTime_succeeds(self):
    self.assert_json_validation_with_valid_primitive_succeeds(
        self.primitive_handler.time_cls)

  def testValidateWrapped_withInvalidTime_raises(self):
    self.assert_json_validation_with_invalid_primitive_raises(
        self.primitive_handler.time_cls)

  def testValidateWrapped_withValidUnsignedInt_succeeds(self):
    self.assert_json_validation_with_valid_primitive_succeeds(
        self.primitive_handler.unsigned_int_cls)

  def testValidateWrapped_withValidUri_succeeds(self):
    self.assert_json_validation_with_valid_primitive_succeeds(
        self.primitive_handler.uri_cls)

  def testValidateWrapped_withValidXhtml_succeeds(self):
    self.assert_json_validation_with_valid_primitive_succeeds(
        self.primitive_handler.xhtml_cls)

  def assert_json_validation_with_valid_primitive_succeeds(
      self, primitive_cls: Type[message.Message]):
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
      self, primitive_cls: Type[message.Message]):
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
    raise NotImplementedError('Subclasses *must* implement primitive_handler.')

  def testParseDateTime_withYearPrecision_succeeds(self):
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

  def testParseDateTime_withMonthPrecision_succeeds(self):
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

  def testParseDateTime_withDayPrecision_succeeds(self):
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

  def testParseDateTime_withSecondPrecision_succeeds(self):
    datetime_str = '2014-10-09T14:58:00+11:00'
    expected = self.primitive_handler.new_date_time(
        value_us=1412827080000000,
        precision=_primitive_time_utils.DateTimePrecision.SECOND,
        timezone='+11:00')
    wrapper = self.primitive_handler.primitive_wrapper_from_json_value(
        datetime_str, self.primitive_handler.date_time_cls)
    self.assertEqual(wrapper.string_value(), '2014-10-09T14:58:00+11:00')
    self.assertEqual(wrapper.wrapped, expected)

  def testParseDateTime_withMillisecondPrecision_succeeds(self):
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

  def testParseDateTime_withMicrosecondPrecision_succeeds(self):
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

  def testPrintDateTime_withYearPrecision_succeeds(self):
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

  def testPrintDateTime_withMonthPrecision_succeeds(self):
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

  def testPrintDateTime_withDayPrecision_succeeds(self):
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

  def testPrintDateTime_withSecondPrecision_succeeds(self):
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

  def testPrintDateTime_withMillisecondPrecision_succeeds(self):
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

  def testPrintDateTime_withMicrosecondPrecision_succeeds(self):
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
    raise NotImplementedError('Subclasses *must* implement primitive_handler.')

  def testParseDate_withYearPrecision_succeeds(self):
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

  def testParseDate_withMonthPrecision_succeeds(self):
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

  def testParseDate_withDayPrecision_succeeds(self):
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

  def testPrintDate_withYearPrecision_succeeds(self):
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

  def testPrintDate_withMonthPrecision_succeeds(self):
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

  def testPrintDate_withDayPrecision_succeeds(self):
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
    raise NotImplementedError('Subclasses *must* implement primitive_handler.')

  def testParseInstant_withSecondPrecision_succeeds(self):
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

  def testParseInstant_withMillisecondPrecision_succeeds(self):
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

  def testParseInstant_withMicrosecondPrecision_succeeds(self):
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
    raise NotImplementedError('Subclasses *must* implement primitive_handler.')

  def testParseTime_withSecondPrecision_succeeds(self):
    timestamp = '12:00:00'
    expected = self.primitive_handler.new_time(
        value_us=43200000000,
        precision=_primitive_time_utils.TimePrecision.SECOND)
    wrapper = self.primitive_handler.primitive_wrapper_from_json_value(
        timestamp, self.primitive_handler.time_cls)
    self.assertEqual(wrapper.string_value(), '12:00:00')
    self.assertEqual(wrapper.wrapped, expected)

  def testParseTime_withMillisecondPrecision_succeeds(self):
    timestamp = '12:00:00.123'
    expected = self.primitive_handler.new_time(
        value_us=43200123000,
        precision=_primitive_time_utils.TimePrecision.MILLISECOND)
    wrapper = self.primitive_handler.primitive_wrapper_from_json_value(
        timestamp, self.primitive_handler.time_cls)
    self.assertEqual(wrapper.string_value(), '12:00:00.123')
    self.assertEqual(wrapper.wrapped, expected)

  def testParseTime_withMicrosecondPrecision_succeeds(self):
    timestamp = '12:00:00.123000'
    expected = self.primitive_handler.new_time(
        value_us=43200123000,
        precision=_primitive_time_utils.TimePrecision.MICROSECOND)
    wrapper = self.primitive_handler.primitive_wrapper_from_json_value(
        timestamp, self.primitive_handler.time_cls)
    self.assertEqual(wrapper.string_value(), '12:00:00.123000')
    self.assertEqual(wrapper.wrapped, expected)

  def testPrintTime_withSecondPrecision_succeeds(self):
    primitive = self.primitive_handler.new_time(
        value_us=43200000000,
        precision=_primitive_time_utils.TimePrecision.SECOND)
    wrapper = self.primitive_handler.primitive_wrapper_from_primitive(primitive)
    self.assertEqual(wrapper.string_value(), '12:00:00')

  def testPrintTime_withMillisecondPrecision_succeeds(self):
    primitive = self.primitive_handler.new_time(
        value_us=43200123000,
        precision=_primitive_time_utils.TimePrecision.MILLISECOND)
    wrapper = self.primitive_handler.primitive_wrapper_from_primitive(primitive)
    self.assertEqual(wrapper.string_value(), '12:00:00.123')

  def testPrintTime_withMicrosecondPrecision_succeeds(self):
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
    raise NotImplementedError('Subclasses *must* implement primitive_handler.')

  def testParseDecimal_withPositiveInteger_succeeds(self):
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

  def testParseDecimal_withNegativeInteger_succeeds(self):
    expected = self.primitive_handler.new_decimal(value='-40')
    wrapper = self.primitive_handler.primitive_wrapper_from_json_value(
        decimal.Decimal('-40'), self.primitive_handler.decimal_cls)
    self.assertEqual(wrapper.string_value(), '-40')
    self.assertEqual(wrapper.wrapped, expected)

  def testParseDecimal_withPositiveReal_succeeds(self):
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

  def testParseDecimal_withZero_succeeds(self):
    expected = self.primitive_handler.new_decimal(value='0')
    wrapper = self.primitive_handler.primitive_wrapper_from_json_value(
        decimal.Decimal('0'), self.primitive_handler.decimal_cls)
    self.assertEqual(wrapper.string_value(), '0')
    self.assertEqual(wrapper.wrapped, expected)

  def testParseDecimal_withHighPrecisionReal_succeeds(self):
    expected = self.primitive_handler.new_decimal(value='1.00065022141624642')
    wrapper = self.primitive_handler.primitive_wrapper_from_json_value(
        decimal.Decimal('1.00065022141624642'),
        self.primitive_handler.decimal_cls)
    self.assertEqual(wrapper.string_value(), '1.00065022141624642')
    self.assertEqual(wrapper.wrapped, expected)

  def testPrintDecimal_withPositiveInteger_succeeds(self):
    primitive = self.primitive_handler.new_decimal(value='185')
    wrapper = self.primitive_handler.primitive_wrapper_from_primitive(primitive)
    self.assertEqual(wrapper.string_value(), '185')

    primitive = self.primitive_handler.new_decimal(value='100')
    wrapper = self.primitive_handler.primitive_wrapper_from_primitive(primitive)
    self.assertEqual(wrapper.string_value(), '100')

  def testPrintDecimal_withNegativeInteger_succeeds(self):
    primitive = self.primitive_handler.new_decimal(value='-40')
    wrapper = self.primitive_handler.primitive_wrapper_from_primitive(primitive)
    self.assertEqual(wrapper.string_value(), '-40')

  def testPrintDecimal_withPositiveReal_succeeds(self):
    primitive = self.primitive_handler.new_decimal(value='0.0099')
    wrapper = self.primitive_handler.primitive_wrapper_from_primitive(primitive)
    self.assertEqual(wrapper.string_value(), '0.0099')

    primitive = self.primitive_handler.new_decimal(value='100.00')
    wrapper = self.primitive_handler.primitive_wrapper_from_primitive(primitive)
    self.assertEqual(wrapper.string_value(), '100.00')

  def testPrintDecimal_withZero_succeeds(self):
    primitive = self.primitive_handler.new_decimal(value='0')
    wrapper = self.primitive_handler.primitive_wrapper_from_primitive(primitive)
    self.assertEqual(wrapper.string_value(), '0')

    primitive = self.primitive_handler.new_decimal(value='0.00')
    wrapper = self.primitive_handler.primitive_wrapper_from_primitive(primitive)
    self.assertEqual(wrapper.string_value(), '0.00')
