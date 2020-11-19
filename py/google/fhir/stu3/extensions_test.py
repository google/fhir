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
"""Test extensions functionality."""

import os
import sys
from typing import Type

from google.protobuf import message
from absl.testing import absltest
from proto.google.fhir.proto.stu3 import datatypes_pb2
from proto.google.fhir.proto.stu3 import extensions_pb2
from proto.google.fhir.proto.stu3 import fhirproto_extensions_pb2
from proto.google.fhir.proto.stu3 import ml_extensions_pb2
from proto.google.fhir.proto.stu3 import resources_pb2
from google.fhir import extensions
from google.fhir import extensions_test

try:
  from testdata.stu3.profiles import test_extensions_pb2
except ImportError:
  # TODO: Add test protos to PYTHONPATH during dist testing.
  pass  # Fall through


_EXTENSIONS_DIR = os.path.join('testdata', 'stu3', 'extensions')


class ExtensionsTest(extensions_test.ExtensionsTest):
  """Tests functionality provided by the extensions module."""

  @property
  def extension_cls(self) -> Type[message.Message]:
    return datatypes_pb2.Extension

  @property
  def testdata_dir(self) -> str:
    return _EXTENSIONS_DIR

  def testGetFhirExtensions_withNoExtensions_returnsEmptyList(self):
    """Tests get_fhir_extensions returns an empty list with no extensions."""
    patient = resources_pb2.Patient()
    self.assertEmpty(extensions.get_fhir_extensions(patient))

  def testGetFhirExtensions_withExtensions_returnsList(self):
    """Tests get_fhir_extensions returns a non-empty list if extensions exist."""
    patient = resources_pb2.Patient()
    patient.extension.add(
        url=datatypes_pb2.Uri(value='abcd'),
        value=datatypes_pb2.Extension.ValueX(
            boolean=datatypes_pb2.Boolean(value=True)))
    self.assertLen(extensions.get_fhir_extensions(patient), 1)

  def testClearFhirExtensions_withMultipleExtensions_succeeds(self):
    """Tests ClearFhirExtensions when a message has multiple extensions."""
    arbitrary_string = datatypes_pb2.String()
    arbitrary_string.extension.add(
        url=datatypes_pb2.Uri(value='first'),
        value=datatypes_pb2.Extension.ValueX(
            boolean=datatypes_pb2.Boolean(value=True)))
    arbitrary_string.extension.add(
        url=datatypes_pb2.Uri(value='second'),
        value=datatypes_pb2.Extension.ValueX(
            boolean=datatypes_pb2.Boolean(value=True)))
    arbitrary_string.extension.add(
        url=datatypes_pb2.Uri(value='third'),
        value=datatypes_pb2.Extension.ValueX(
            boolean=datatypes_pb2.Boolean(value=True)))
    self.assertLen(extensions.get_fhir_extensions(arbitrary_string), 3)

    # Remove middle extension
    extensions.clear_fhir_extensions_with_url(arbitrary_string, 'second')
    remaining_extensions = extensions.get_fhir_extensions(arbitrary_string)
    self.assertLen(remaining_extensions, 2)

    remaining_urls = [extension.url.value for extension in remaining_extensions]
    self.assertEqual(remaining_urls, ['first', 'third'])

  def testExtensionToMessage_withEventTrigger_succeeds(self):
    self.assert_extension_to_message_equals_golden(
        'trigger', ml_extensions_pb2.EventTrigger)

  def testMessageToExtension_withEventTrigger_succeeds(self):
    self.assert_message_to_extension_equals_golden(
        'trigger', ml_extensions_pb2.EventTrigger)

  def testExtensionToMessage_withEventLabel_succeeds(self):
    self.assert_extension_to_message_equals_golden('label',
                                                   ml_extensions_pb2.EventLabel)

  def testMessageToExtension_withEventLabel_succeeds(self):
    self.assert_message_to_extension_equals_golden('label',
                                                   ml_extensions_pb2.EventLabel)

  def testExtensionToMessage_withPrimitiveHasNoValue_succeeds(self):
    self.assert_extension_to_message_equals_golden(
        'primitive_has_no_value', fhirproto_extensions_pb2.PrimitiveHasNoValue)

  def testMessageToExtension_withPrimitiveHasNoValue_succeeds(self):
    self.assert_message_to_extension_equals_golden(
        'primitive_has_no_value', fhirproto_extensions_pb2.PrimitiveHasNoValue)

  def testExtensionToMessage_withEmptyPrimitiveHasNoValue_succeeds(self):
    self.assert_extension_to_message_equals_golden(
        'empty', fhirproto_extensions_pb2.PrimitiveHasNoValue)

  def testMessageToExtension_withEmptyPrimitiveHasNoValue_succeeds(self):
    self.assert_message_to_extension_equals_golden(
        'empty', fhirproto_extensions_pb2.PrimitiveHasNoValue)

  def testExtensionToMessage_withCapabilityStatementSearchParameterCombination_succeeds(
      self):
    self.assert_extension_to_message_equals_golden(
        'capability',
        extensions_pb2.CapabilityStatementSearchParameterCombination)

  def testMessageToExtension_withCapabilityStatementSearchParameterCombination_succeeds(
      self):
    self.assert_message_to_extension_equals_golden(
        'capability',
        extensions_pb2.CapabilityStatementSearchParameterCombination)

  @absltest.skipIf(
      'testdata' not in sys.modules,
      'google-fhir package does not build+install tertiary testdata protos.')
  def testExtensionToMessage_withDigitalMediaType_succeeds(self):
    self.assert_extension_to_message_equals_golden(
        'digital_media_type', test_extensions_pb2.DigitalMediaType)

  @absltest.skipIf(
      'testdata' not in sys.modules,
      'google-fhir package does not build+install tertiary testdata protos.')
  def testMessageToExtension_withDigitalMediaType_succeeds(self):
    self.assert_message_to_extension_equals_golden(
        'digital_media_type', test_extensions_pb2.DigitalMediaType)


if __name__ == '__main__':
  absltest.main()
