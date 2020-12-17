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
"""PrimitiveWrapper subclass for Base64Binary primitives."""

import base64
import binascii

from typing import cast, Any, List, Tuple, Type, TypeVar

from google.protobuf import message
from google.fhir import extensions
from google.fhir import fhir_errors
from google.fhir.json_format.wrappers import _primitive_wrappers
from google.fhir.utils import proto_utils

Base64Binary = TypeVar('Base64Binary', bound=message.Message)
SeparatorStride = TypeVar('SeparatorStride', bound=message.Message)


def _separate_string(string: str, stride: int, separator: str) -> str:
  """Returns a separated string by separator at multiples of stride.

  For example, the input:
  * string: 'thequickbrownfoxjumpedoverthelazydog'
  * stride: 3
  * separator: '-'

  Would produce a return value of:
  'the-qui-ckb-row-nfo-xju-mpe-dov-ert-hel-azy-dog'

  Args:
    string: The string to split.
    stride: The interval to insert the separator at.
    separator: The string to insert at every stride interval.

  Returns:
    The original string with the separator present at every stride interval.
  """
  result = ''
  for (i, c) in enumerate(string):
    if (i > 0 and i % stride == 0):
      result += separator
    result += c
  return result


def _parse(json_str: str, primitive_cls: Type[Base64Binary], *,
           separator_stride_cls: Type[SeparatorStride]) -> Base64Binary:
  """Parses the json_str into a Base64Binary FHIR primitive protobuf message.

  Args:
    json_str: The raw JSON string to parse.
    primitive_cls: The type of FHIR primitive to parse into.
    separator_stride_cls: The type of Base64BinarySeparatorStride extension
      associated with primitive_cls.

  Returns:
    A FHIR primitive Base64Binary protobuf message.

  Raises:
    fhir_errors.InvalidFhirError: In the event that the provided json_str is
    not a valid base64-encoded string.
  """
  # Properly capture the FHIR-allowed white-space separator, if one exists.
  # This is a series of one or more spaces separating valid base64 encoded data.
  # This series is repeated at the same intervals throughout the entirety of the
  # json_str.
  #
  # For example, for a json_str of 'Zm9v  YmFy', this would result in:
  # json_str: 'Zm9v  YmFy'
  # separator: '  '
  # stride: 4
  result = primitive_cls()
  stride = json_str.find(' ')
  if stride != -1:
    end = stride
    while end < len(json_str) and json_str[end] == ' ':
      end += 1
    separator = json_str[stride:end]

    # Silencing the type checkers as pytype doesn't fully support structural
    # subtyping yet.
    # pylint: disable=line-too-long
    # See: https://mypy.readthedocs.io/en/stable/casts.html#casts-and-type-assertions.
    # pylint: enable=line-too-long
    # Soon: https://www.python.org/dev/peps/pep-0544/.
    separator_stride_extension = cast(Any, separator_stride_cls())
    separator_stride_extension.separator.value = separator
    separator_stride_extension.stride.value = stride
    extensions.add_message_to_extension(separator_stride_extension,
                                        result.extension.add())

    json_str = json_str.replace(separator, '')

  try:
    result.value = base64.b64decode(json_str, validate=True)
  except binascii.Error as e:
    raise fhir_errors.InvalidFhirError('Invalid base64-encoded string.') from e
  return result


class Base64BinaryWrapper(_primitive_wrappers.PrimitiveWrapper):
  """A wrapper around the Base64Binary FHIR primitive type."""

  _PARSABLE_TYPES: Tuple[Type[Any], ...] = (str,)

  @classmethod
  def from_json_str(
      cls, json_str: str, primitive_cls: Type[Base64Binary],
      context: _primitive_wrappers.Context) -> 'Base64BinaryWrapper':
    """See _primitive_wrappers.PrimitiveWrapper.from_json_str."""
    _primitive_wrappers.validate_primitive_json_representation(
        primitive_cls.DESCRIPTOR, json_str)
    base64_binary = _parse(
        json_str,
        primitive_cls,
        separator_stride_cls=context.separator_stride_cls)
    return cls(base64_binary, context)

  def __init__(self, wrapped: message.Message,
               context: _primitive_wrappers.Context):
    super(Base64BinaryWrapper, self).__init__(wrapped, context)
    self._separator_stride_cls = context.separator_stride_cls

  def _nonnull_string_value(self) -> str:
    separator_extensions: List[Any] = extensions.get_repeated_from_extensions(
        proto_utils.get_value_at_field(self.wrapped, 'extension'),
        self._separator_stride_cls)

    value = proto_utils.get_value_at_field(self.wrapped, 'value')
    encoded = base64.b64encode(value).decode(encoding='ascii')
    if separator_extensions:
      encoded = _separate_string(encoded, separator_extensions[0].stride.value,
                                 separator_extensions[0].separator.value)
    return encoded
