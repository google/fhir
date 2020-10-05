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

import abc
import os
from typing import Type

from google.protobuf import message
from absl.testing import absltest
from google.fhir import extensions
from google.fhir.testing import testdata_utils


class ExtensionsTest(absltest.TestCase, metaclass=abc.ABCMeta):
  """An abstract base class for testing the extensions module."""

  @property
  @abc.abstractmethod
  def extension_cls(self) -> Type[message.Message]:
    raise NotImplementedError('Subclasses *must* implement extension_cls.')

  @property
  @abc.abstractmethod
  def testdata_dir(self) -> str:
    raise NotImplementedError('Subclasses *must* implement testdata_dir.')

  def assert_extension_to_message_equals_golden(
      self, name: str, message_cls: Type[message.Message]):
    """Exercises the extensions.extension_to_message functionality."""
    msg = testdata_utils.read_protos(
        os.path.join(self.testdata_dir, name + '.message.prototxt'),
        message_cls)[0]
    extension = testdata_utils.read_protos(
        os.path.join(self.testdata_dir, name + '.extension.prototxt'),
        self.extension_cls)[0]

    output = extensions.extension_to_message(extension, message_cls)
    self.assertEqual(output, msg)

  def assert_message_to_extension_equals_golden(
      self, name: str, message_cls: Type[message.Message]):
    """Exercises the extensions.message_to_extension functionality."""
    msg = testdata_utils.read_protos(
        os.path.join(self.testdata_dir, name + '.message.prototxt'),
        message_cls)[0]
    extension = testdata_utils.read_protos(
        os.path.join(self.testdata_dir, name + '.extension.prototxt'),
        self.extension_cls)[0]

    output = extensions.message_to_extension(msg, self.extension_cls)
    self.assertEqual(output, extension)
