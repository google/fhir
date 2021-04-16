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
"""PrimitiveWrapper subclass for Decimal primitives."""

import decimal
import json

from typing import cast, Any, Tuple, Type, TypeVar

from google.protobuf import message
from google.fhir.json_format.wrappers import _primitive_wrappers

Decimal = TypeVar('Decimal', bound=message.Message)


def _parse(json_str: str, primitive_cls: Type[Decimal]) -> Decimal:
  """Parses the json_str into a Decimal FHIR primitive protobuf message.

  Args:
    json_str: The raw JSON string to parse.
    primitive_cls: The type of FHIR primitive to parse into.

  Returns:
    A FHIR primitive Decimal protobuf message.
  """
  # We don't parse floating point numbers with Python floating point or
  # integer types to avoid precision loss. Leveraging decimal.Decimal we can
  # effectively check the range of values for inf/NaN
  decimal_value = json.loads(
      json_str, parse_float=decimal.Decimal, parse_int=decimal.Decimal)
  if not isinstance(decimal_value, decimal.Decimal):
    raise ValueError('Invalid Decimal format')
  if not decimal_value.is_finite():
    raise ValueError('Decimal out of range.')

  return cast(Any, primitive_cls)(value=json_str)


class DecimalWrapper(_primitive_wrappers.PrimitiveWrapper):
  """A wrapper around the Decimal FHIR primitive type."""

  _PARSABLE_TYPES: Tuple[Type[Any], ...] = (decimal.Decimal,)

  @classmethod
  def from_json_str(cls, json_str: str, primitive_cls: Type[Decimal],
                    context: _primitive_wrappers.Context) -> 'DecimalWrapper':
    """See _primitive_wrappers.PrimitiveWrapper.from_json_str."""
    _primitive_wrappers.validate_primitive_json_representation(
        primitive_cls.DESCRIPTOR, json_str)
    decimal_ = _parse(json_str, primitive_cls)
    return cls(decimal_, context)

  def json_value(self) -> str:
    return self.string_value()
