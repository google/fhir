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
"""Tests the primitive_handler module."""

import os

from absl.testing import absltest
from google.fhir import primitive_handler_test
from google.fhir.stu3 import primitive_handler

_PRIMITIVE_HANDLER = primitive_handler.PrimitiveHandler()
_VALIDATION_DIR = os.path.join('testdata', 'stu3', 'validation')


class PrimitiveWrapperPrimitiveHasNoValueTest(
    primitive_handler_test.PrimitiveWrapperPrimitiveHasNoValueTest):

  @property
  def primitive_handler(self) -> primitive_handler.PrimitiveHandler:
    return _PRIMITIVE_HANDLER


class PrimitiveWrapperProtoValidationTest(
    primitive_handler_test.PrimitiveWrapperProtoValidationTest):

  @property
  def primitive_handler(self) -> primitive_handler.PrimitiveHandler:
    return _PRIMITIVE_HANDLER

  @property
  def validation_dir(self) -> str:
    return _VALIDATION_DIR


class PrimitiveWrapperJsonValidationTest(
    primitive_handler_test.PrimitiveWrapperJsonValidationTest):

  @property
  def primitive_handler(self) -> primitive_handler.PrimitiveHandler:
    return _PRIMITIVE_HANDLER

  @property
  def validation_dir(self) -> str:
    return _VALIDATION_DIR

  def testValidateWrapped_withInvalidInstant_raises(self):
    # TODO: Re-enable once STU3 protos are re-generated for 3.0.2.
    pass


class DateTimeWrapperTest(primitive_handler_test.DateTimeWrapperTest):

  @property
  def primitive_handler(self) -> primitive_handler.PrimitiveHandler:
    return _PRIMITIVE_HANDLER


class DateWrapperTest(primitive_handler_test.DateWrapperTest):

  @property
  def primitive_handler(self) -> primitive_handler.PrimitiveHandler:
    return _PRIMITIVE_HANDLER


class InstantWrapperTest(primitive_handler_test.InstantWrapperTest):

  @property
  def primitive_handler(self) -> primitive_handler.PrimitiveHandler:
    return _PRIMITIVE_HANDLER


class TimeWrapperTest(primitive_handler_test.TimeWrapperTest):

  @property
  def primitive_handler(self) -> primitive_handler.PrimitiveHandler:
    return _PRIMITIVE_HANDLER


class DecimalWrapperTest(primitive_handler_test.DecimalWrapperTest):

  @property
  def primitive_handler(self) -> primitive_handler.PrimitiveHandler:
    return _PRIMITIVE_HANDLER


if __name__ == '__main__':
  absltest.main()
