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
"""Functions and subclasses for validating and parsing FHIR primitives."""

import abc
import collections
import decimal
import json
import re
import threading
from typing import cast, Any, List, Optional, Pattern, Tuple, Type, TypeVar

from google.protobuf import descriptor
from google.protobuf import message

from google.fhir import codes
from google.fhir import extensions
from google.fhir import fhir_errors
from google.fhir.utils import annotation_utils
from google.fhir.utils import proto_utils

_T = TypeVar('_T', bound=message.Message)

_primitive_patterns: collections.defaultdict = collections.defaultdict(dict)
_primitive_patterns_cv = threading.Condition(threading.Lock())


def _pattern_for_primitive(
    desc: descriptor.Descriptor) -> Optional[Pattern[str]]:
  """Returns a compiled regex pattern for a given primitive."""
  with _primitive_patterns_cv:
    if desc.full_name not in _primitive_patterns:

      # If the primitive value has no associated pattern, early exit
      raw_str = annotation_utils.get_value_regex_for_primitive_type(desc)
      if raw_str is None:
        return None

      _primitive_patterns[desc.full_name] = re.compile(raw_str, re.ASCII)
    return _primitive_patterns[desc.full_name]


def no_value_primitive(primitive_cls: Type[_T]) -> _T:
  """Returns instance of primitive_cls with a PrimitiveHasNoValue extension."""
  extension_field = primitive_cls.DESCRIPTOR.fields_by_name.get('extension')
  if extension_field is None:
    raise ValueError("Invalid primitive. No 'extension' field exists on "
                     f"{primitive_cls.DESCRIPTOR.full_name}.")
  primitive_has_no_value = extensions.create_primitive_has_no_value(
      extension_field.message_type)
  return cast(Any, primitive_cls)(extension=[primitive_has_no_value])


def validate_primitive_json_representation(desc: descriptor.Descriptor,
                                           json_str: str):
  """Ensures that json_str matches the associated regex pattern, if one exists.

  Args:
    desc: The Descriptor of the FHIR primitive to validate.
    json_str: The JSON string to validate.

  Raises:
    fhir_errors.InvalidFhirError: Raised in the event that pattern is unable to
    be matched on json_str.
  """
  pattern = _pattern_for_primitive(desc)

  # Raise an exception if we're unable to detect the pattern
  if pattern is not None and pattern.fullmatch(json_str) is None:
    raise fhir_errors.InvalidFhirError(f'Unable to find pattern: {pattern!r}.')


def validate_primitive_without_value(fhir_primitive: message.Message):
  """Validates a Message which has the PrimitiveWithoutValue extension.

  Given that there is a PrimitiveWithoutValue extension present, there must be
  at least one other extension. Otherwise, there is truly no value set other
  than id and/or extension (non-value fields).

  Args:
    fhir_primitive: The FHIR primitive Message to validate.

  Raises:
    fhir_errors.InvalidFhirError: In the event that there is less than one
    extension present, or there are values set other than id and/or extension.
  """
  name = fhir_primitive.DESCRIPTOR.full_name

  # There must be at least one other FHIR extension
  if len(extensions.get_fhir_extensions(fhir_primitive)) < 2:
    raise fhir_errors.InvalidFhirError(
        f'{name!r} must have either extensions or a value present.')

  # ... Else truly no value set other than id and/or extension
  for field, _ in fhir_primitive.ListFields():  # Iterate set fields only
    if field.name not in extensions.NON_VALUE_FIELDS:
      raise fhir_errors.InvalidFhirError(
          f'{name!r} contains PrimitiveHasNoValue but {field.name!r} is set.')


class Context:
  """Dependency injection to pass necessary types to primitive wrappers."""

  def __init__(self, *, separator_stride_cls: Type[message.Message],
               code_cls: Type[message.Message], default_timezone: str):
    """Creates a new instance of primitive_wrappers.Context.

    Args:
      separator_stride_cls: The Base64BinarySeparatorStride type to use when
        parsing/printing Base64Binary FHIR primitives.
      code_cls: The Code type to use when parsing/printing profiled-Code
        primitives.
      default_timezone: The default timezone to use for date/time-like primitive
        parsing/printing.
    """
    self.separator_stride_cls = separator_stride_cls
    self.code_cls = code_cls
    self.default_timezone = default_timezone


class PrimitiveWrapper(abc.ABC):
  """Abstract base class for wrapping FHIR primitives."""

  # Subclasses *must* override to ensure proper validation checks.
  _PARSABLE_TYPES: Optional[Tuple[Type[Any], ...]] = None

  @classmethod
  def from_primitive(cls, primitive: message.Message,
                     context: Context) -> 'PrimitiveWrapper':
    """Instantiates a new version of PrimitiveWrapper wrapping primitive.

    Args:
      primitive: The FHIR primitive message to wrap and validate.
      context: Related primitive information to use for printing/parsing a
        wrapped primitive.

    Returns:
      An instance of PrimitiveWrapper.
    """
    result = cls(primitive, context)
    result.validate_wrapped()
    return result

  @classmethod
  def from_json_value(cls, json_value: Optional[Any],
                      primitive_cls: Type[message.Message],
                      context: Context) -> 'PrimitiveWrapper':
    """Parses json_value into an instance of primitive_cls and wraps.

    Args:
      json_value: The optional raw json_value to parse and wrap.
      primitive_cls: The type of FHIR primitive message to create and validate.
      context: Related primitive information to use for printing/parsing a
        wrapped primitive.

    Returns:
      An instance of PrimitiveWrapper.
    """
    if isinstance(json_value, (list, tuple)):
      raise ValueError('Error, unable to wrap sequence.')

    if json_value is None or isinstance(json_value, (dict,)):
      return cls(no_value_primitive(primitive_cls), context)

    if not isinstance(json_value,
                      cast(Tuple[Type[Any], ...], cls._PARSABLE_TYPES)):
      raise fhir_errors.InvalidFhirError(
          'Unable to parse JSON. {type(json_value)} is invalid FHIR JSON.')

    return cls.from_json_str(str(json_value), primitive_cls, context)

  @classmethod
  @abc.abstractmethod
  def from_json_str(cls, json_str: str, primitive_cls: Type[message.Message],
                    context: Context) -> 'PrimitiveWrapper':
    """Serializes json_str into an instance of primitive_cls and wraps.

    Args:
      json_str: The string-representation of the raw json_value to serialize
        into primitive_cls and wrap.
      primitive_cls: The FHIR primitive class to serialize into and wrap.
      context: Related primitive information to use for printing/parsing a
        wrapped primitive.

    Returns:
      An instance of PrimitiveWrapper.
    """
    raise NotImplementedError('Subclasses *must* implement from_json_str.')

  def __init__(self, wrapped: message.Message, unused_context: Context):
    """Initializes a new PrimitiveWrapper with wrapped.

    Args:
      wrapped: The primitive message to wrap.
    """
    self.wrapped = wrapped

  def _nonnull_string_value(self) -> str:
    """Returns a non-null string representation of the wrapped primitive."""
    return str(proto_utils.get_value_at_field(self.wrapped, 'value'))

  def get_element(self) -> Optional[message.Message]:
    """Returns the raw Element underlying the wrapped primitive.

    Note that conversion-only extensions are removed prior to returning.
    """
    if not (proto_utils.field_is_set(self.wrapped, 'id') or
            proto_utils.field_is_set(self.wrapped, 'extension')):
      return None  # Early-exit if we can't populate an Element

    element = proto_utils.create_message_from_descriptor(
        self.wrapped.DESCRIPTOR)
    if proto_utils.field_is_set(self.wrapped, 'id'):
      proto_utils.copy_common_field(self.wrapped, element, 'id')

    extensions_list = cast(List[Any],
                           extensions.get_fhir_extensions(self.wrapped))
    for extension in extensions_list:
      if extension.url.value not in extensions.CONVERSION_ONLY_EXTENSION_URLS:
        proto_utils.append_value_at_field(element, 'extension', extension)

    return element

  def has_element(self) -> bool:
    """Returns True if the wrapped primitive has data in its `Element` fields.

    This method checks whether or not the wrapped primitive contains data in
    fields defined on the root Element type, like `id` or `extension`.
    """
    if proto_utils.field_is_set(self.wrapped, 'id'):
      return True

    # Return True if there exists an extension on the wrapped primitive whose
    # structure definition URL is something other than a conversion-only URL
    extensions_list = cast(List[Any],
                           extensions.get_fhir_extensions(self.wrapped))
    for extension in extensions_list:
      if extension.url.value not in extensions.CONVERSION_ONLY_EXTENSION_URLS:
        return True
    return False

  def has_value(self) -> bool:
    """Returns True if the wrapped primitive has a value.

    This method checks for the PrimitiveHasNoValue extension,
    *not* for the actual presence of a value field.
    """
    extensions_list = cast(List[Any],
                           extensions.get_fhir_extensions(self.wrapped))
    for extension in extensions_list:
      if (extension.url.value == extensions.PRIMITIVE_HAS_NO_VALUE_URL and
          extension.value.boolean.value):
        return False
    return True

  def json_value(self) -> str:
    """A JSON string representation of the wrapped primitive."""
    return json.dumps(self.string_value(), ensure_ascii=False)

  def merge_into(self, target: message.Message):
    """Merges the underlying wrapped primitive into target."""
    if not proto_utils.are_same_message_type(self.wrapped.DESCRIPTOR,
                                             target.DESCRIPTOR):
      raise ValueError(f'Type mismatch in merge_into. Attempted to merge '
                       f'{self.wrapped.DESCRIPTOR.full_name} into '
                       f'{target.DESCRIPTOR.full_name}.')
    target.MergeFrom(self.wrapped)

  def string_value(self) -> str:
    return self._nonnull_string_value() if self.has_value() else 'null'

  def validate_wrapped(self):
    """Validates the underlying wrapped value.

    Raises:
      fhir_errors.InvalidFhirError: In the event that the wrapped value doesn't
      conform to the FHIR standard.
    """
    if self.has_value():
      validate_primitive_json_representation(self.wrapped.DESCRIPTOR,
                                             self.string_value())
    else:  # No value
      validate_primitive_without_value(self.wrapped)


class BooleanWrapper(PrimitiveWrapper):
  """A wrapper around the Boolean FHIR primitive type."""

  _PARSABLE_TYPES: Tuple[Type[Any], ...] = (bool,)

  @classmethod
  def from_json_str(cls, json_str: str, primitive_cls: Type[message.Message],
                    context: Context) -> 'BooleanWrapper':
    """See PrimitiveWrapper.from_json_str."""
    boolean = cast(Any, primitive_cls)(value=json_str.lower() == 'true')
    return cls(boolean, context)

  def _nonnull_string_value(self) -> str:
    return str(proto_utils.get_value_at_field(self.wrapped, 'value')).lower()

  def json_value(self) -> str:
    return self.string_value()


class CodeWrapper(PrimitiveWrapper):
  """A wrapper around the Code FHIR primitive type."""

  _PARSABLE_TYPES: Tuple[Type[Any], ...] = (str,)

  @classmethod
  def from_json_str(cls, json_str: str, primitive_cls: Type[message.Message],
                    context: Context) -> 'CodeWrapper':
    """See PrimitiveWrapper.from_json_str."""
    validate_primitive_json_representation(primitive_cls.DESCRIPTOR, json_str)
    code = cast(Any, context.code_cls)(value=json_str)
    return cls(code, context)

  def __init__(self, wrapped: message.Message, context: Context):
    """Converts wrapped into an instance of context.code_cls and wraps."""
    if not proto_utils.are_same_message_type(wrapped.DESCRIPTOR,
                                             context.code_cls.DESCRIPTOR):
      tmp = context.code_cls()
      codes.copy_code(wrapped, tmp)
      wrapped = tmp
    super(CodeWrapper, self).__init__(wrapped, context)

  def merge_into(self, target: message.Message):
    """See PrimitiveWrapper.merge_into."""
    codes.copy_code(self.wrapped, target)


class StringLikePrimitiveWrapper(PrimitiveWrapper):
  """A subclass of PrimitiveWrapper for wrapping string-derived primitives."""

  _PARSABLE_TYPES: Tuple[Type[Any], ...] = (str,)

  @classmethod
  def from_json_str(cls, json_str: Optional[Any],
                    primitive_cls: Type[message.Message],
                    context: Context) -> 'StringLikePrimitiveWrapper':
    """See PrimitiveWrapper.from_json_str."""
    validate_primitive_json_representation(primitive_cls.DESCRIPTOR, json_str)
    primitive = cast(Any, primitive_cls)(value=json_str)
    return cls(primitive, context)


class XhtmlWrapper(StringLikePrimitiveWrapper):
  """A wrapper around the Xhtml FHIR primitive type."""

  PARSABLE_TYPES: Tuple[Type[Any], ...] = (str,)

  @classmethod
  def from_json_value(cls, json_value: Optional[Any],
                      primitive_cls: Type[message.Message],
                      context: Context) -> 'XhtmlWrapper':
    """See PrimitiveWrapper.from_json_value."""
    if json_value is None or isinstance(json_value, (dict,)):
      raise ValueError(
          f'Invalid input for class: {primitive_cls!r}.')  # Disallow None

    if not isinstance(json_value, cast(Tuple[Type[Any], ...],
                                       cls.PARSABLE_TYPES)):
      raise fhir_errors.InvalidFhirError(
          f'Unable to parse Xhtml. {type(json_value)} is invalid.')

    return cast(XhtmlWrapper,
                cls.from_json_str(str(json_value), primitive_cls, context))

  def get_element(self) -> Optional[message.Message]:
    """Return Element of the primitive. Xhtml cannot have extensions."""
    if not proto_utils.field_is_set(self.wrapped, 'id'):
      return None
    element = type(self.wrapped)()
    proto_utils.copy_common_field(self.wrapped, element, 'id')
    return element

  def has_element(self) -> bool:
    """Returns True if the Xhtml primitive has an 'id' field."""
    return proto_utils.field_is_set(self.wrapped, 'id')

  def has_value(self) -> bool:
    """Always returns True.

    Xhtml doesn't have an extension attribute, and is always considered to have
    a value present.

    Returns:
      True for all wrapped Xhtml primitives.
    """
    return True


class IntegerLikePrimitiveWrapper(PrimitiveWrapper):
  """A subclass of PrimitiveWrapper for wrapping integer-derived primitives."""

  _PARSABLE_TYPES: Tuple[Type[Any], ...] = (
      int,
      decimal.Decimal,
  )

  @classmethod
  def from_json_str(cls, json_str: str, primitive_cls: Type[message.Message],
                    context: Context) -> 'IntegerLikePrimitiveWrapper':
    """See PrimitiveWrapper.from_json_str."""
    validate_primitive_json_representation(primitive_cls.DESCRIPTOR, json_str)
    primitive = cast(Any, primitive_cls)(value=int(json_str))
    return cls(primitive, context)

  def _nonnull_string_value(self) -> str:
    return str(proto_utils.get_value_at_field(self.wrapped, 'value'))

  def json_value(self) -> str:
    return self.string_value()
