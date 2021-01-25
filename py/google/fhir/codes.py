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
"""Functions to manipulate FHIR code systems.

See also: https://www.hl7.org/fhir/codesystem.html.
"""

import collections
import threading
from typing import cast, Optional, TypeVar

from google.protobuf import descriptor
from google.protobuf import message
from proto.google.fhir.proto import annotations_pb2
from google.fhir import fhir_errors
from google.fhir.utils import annotation_utils
from google.fhir.utils import fhir_types
from google.fhir.utils import proto_utils

_T = TypeVar('_T', bound=message.Message)

_CODE_TYPES = frozenset([
    descriptor.FieldDescriptor.TYPE_ENUM,
    descriptor.FieldDescriptor.TYPE_STRING,
])

_memos = collections.defaultdict(dict)
_memos_cv = threading.Condition(threading.Lock())


def _get_enum_value_descriptor_memo(
    enum_descriptor: descriptor.EnumDescriptor,
    code_string: str) -> Optional[descriptor.EnumValueDescriptor]:
  """Returns the EnumValueDescriptor from the shared memoization mapping."""
  with _memos_cv:
    return _memos.get(enum_descriptor.full_name, {}).get(code_string)


def _set_enum_value_descriptor_memo(
    enum_descriptor: descriptor.EnumDescriptor, code_string: str,
    value_descriptor: descriptor.EnumValueDescriptor):
  """Sets an EnumValueDescriptor in the shared memoization mapping."""
  with _memos_cv:
    _memos[enum_descriptor.full_name][code_string] = value_descriptor


def enum_value_descriptor_to_code_string(
    enum_value_descriptor: descriptor.EnumValueDescriptor) -> str:
  """Returns the code string describing the enum value.

  Args:
    enum_value_descriptor: The EnumValueDescriptor to convert.

  Returns:
    The code string describing the enum value.
  """
  original_code = annotation_utils.get_enum_value_original_code(
      enum_value_descriptor)
  return (original_code if original_code is not None else
          enum_value_descriptor.name.lower().replace('_', '-'))


def code_string_to_enum_value_descriptor(
    code_string: str, enum_descriptor: descriptor.EnumDescriptor
) -> descriptor.EnumValueDescriptor:
  """Returns an EnumValueDescriptor for a provided EnumDescriptor and raw code.

  Args:
    code_string: A raw string representation of the code to retrieve.
    enum_descriptor: The EnumDescriptor the desired EnumValueDescriptor belongs
      to.

  Returns:
    An instance of EnumValueDescriptor that the code_string represents.

  Raises:
    fhir_errors.InvalidFhirError: In the event that a conversion from
    code_string was unsuccessful.
  """
  # Check the shared memos mapping
  value_descriptor = _get_enum_value_descriptor_memo(enum_descriptor,
                                                     code_string)
  if value_descriptor is not None:
    return value_descriptor

  # Make minor substitutions to the raw value, and search for value descriptor.
  # If found, update the shared memo mapping.
  fhir_case_code_string = code_string.upper().replace('-', '_')
  value_descriptor = enum_descriptor.values_by_name.get(fhir_case_code_string)
  if value_descriptor is not None:
    _set_enum_value_descriptor_memo(enum_descriptor, code_string,
                                    value_descriptor)
    return value_descriptor

  # Finally, some codes had to be renamed to make them valid enum values.
  # Iterate through all target enum values, and look for the FHIR original code
  # extension value.
  for value_descriptor in enum_descriptor.values:
    if (value_descriptor.GetOptions().HasExtension(
        annotations_pb2.fhir_original_code) and
        value_descriptor.GetOptions().Extensions[
            annotations_pb2.fhir_original_code] == code_string):
      _set_enum_value_descriptor_memo(enum_descriptor, code_string,
                                      value_descriptor)
      return value_descriptor

  raise fhir_errors.InvalidFhirError(
      f'Failed to convert {code_string!r} to {enum_descriptor.full_name}. No '
      f'matching enum found.')


def copy_coding(source: message.Message, target: message.Message):
  """Copies all fields from source to target "Coding" messages.

  Args:
    source: The FHIR coding instance to copy from.
    target: The FHIR coding instance to copy to.

  Raises:
    InvalidFhirError: In the event that source or target is not a type/profile
    of Coding.
  """
  if not fhir_types.is_type_or_profile_of_coding(source.DESCRIPTOR):
    raise fhir_errors.InvalidFhirError(f'Source: {source.DESCRIPTOR.full_name} '
                                       'is not a type or profile of Coding.')

  if not fhir_types.is_type_or_profile_of_coding(target.DESCRIPTOR):
    raise fhir_errors.InvalidFhirError(f'Target: {target.DESCRIPTOR.full_name} '
                                       'is not a type or profile of Coding.')

  if proto_utils.are_same_message_type(source.DESCRIPTOR, target.DESCRIPTOR):
    target.CopyFrom(source)
    return

  # Copy fields present in both profiled and unprofiled codings.
  proto_utils.copy_common_field(source, target, 'id')
  proto_utils.copy_common_field(source, target, 'extension')
  proto_utils.copy_common_field(source, target, 'version')
  proto_utils.copy_common_field(source, target, 'display')
  proto_utils.copy_common_field(source, target, 'user_selected')

  # Copy the "code" field from source to target
  source_code = proto_utils.get_value_at_field(source, 'code')
  copy_code(source_code, proto_utils.set_in_parent_or_add(target, 'code'))

  target_system_field = target.DESCRIPTOR.fields_by_name.get('system')

  # TODO: This will fail if there is a target system field,
  # *and* a source system field, since in this case the source code will not
  # contain the system information, the containing Coding would.  In general,
  # it's not quite right to get the system from Code, since unprofiled codes
  # don't contain system information.  In practice, this isn't a problem,
  # because the only kind of profiled Codings we currently support are
  # Codings with typed Codes (which contain source information) but this is
  # not neccessary according to FHIR spec.
  if target_system_field is not None:
    source_system_str = get_system_for_code(source_code)
    target_system_uri = proto_utils.set_in_parent_or_add(
        target, target_system_field)
    proto_utils.set_value_at_field(target_system_uri, 'value',
                                   source_system_str)


def copy_code(source: message.Message, target: message.Message):
  """Adds all fields from source to target.

  Args:
    source: The FHIR Code instance to copy from.
    target: The target FHIR Code instance to copy to.
  """
  if not fhir_types.is_type_or_profile_of_code(source.DESCRIPTOR):
    raise fhir_errors.InvalidFhirError(f'Source: {source.DESCRIPTOR.full_name} '
                                       'is not type or profile of Code.')

  if not fhir_types.is_type_or_profile_of_code(target.DESCRIPTOR):
    raise fhir_errors.InvalidFhirError(f'Target: {target.DESCRIPTOR.full_name} '
                                       'is not type or profile of Code.')

  if proto_utils.are_same_message_type(source.DESCRIPTOR, target.DESCRIPTOR):
    target.CopyFrom(source)
    return

  source_value_field = source.DESCRIPTOR.fields_by_name.get('value')
  target_value_field = target.DESCRIPTOR.fields_by_name.get('value')
  if source_value_field is None or target_value_field is None:
    raise fhir_errors.InvalidFhirError('Unable to copy code from '
                                       f'{source.DESCRIPTOR.full_name} '
                                       f'to {target.DESCRIPTOR.full_name}.')

  proto_utils.copy_common_field(source, target, 'id')
  proto_utils.copy_common_field(source, target, 'extension')

  # Handle specialized codes
  if (source_value_field.type not in _CODE_TYPES or
      target_value_field.type not in _CODE_TYPES):
    raise ValueError(f'Unable to copy from {source.DESCRIPTOR.full_name} '
                     f'to {target.DESCRIPTOR.full_name}. Must have a field '
                     'of TYPE_ENUM or TYPE_STRING.')

  source_value = proto_utils.get_value_at_field(source, source_value_field)
  if source_value_field.type == target_value_field.type:
    # Perform a simple assignment if value_field types are equivalent
    proto_utils.set_value_at_field(target, target_value_field, source_value)
  else:
    # Otherwise, we need to transform the value prior to assignment...
    if source_value_field.type == descriptor.FieldDescriptor.TYPE_STRING:
      source_enum_value = code_string_to_enum_value_descriptor(
          source_value, target_value_field.enum_type)
      proto_utils.set_value_at_field(target, target_value_field,
                                     source_enum_value.number)
    elif source_value_field.type == descriptor.FieldDescriptor.TYPE_ENUM:
      source_string_value = enum_value_descriptor_to_code_string(
          source_value_field.enum_type.values_by_number[source_value])
      proto_utils.set_value_at_field(target, target_value_field,
                                     source_string_value)
    else:  # Should never hit
      raise ValueError('Unexpected generic value field type: '
                       f'{source_value_field.type}. Must be a field of '
                       'TYPE_ENUM or TYPE_STRING in order to copy.')


def get_system_for_code(code: message.Message) -> str:
  """Returns the code system associated with the provided Code."""
  system_field = code.DESCRIPTOR.fields_by_name.get('system')
  if system_field is not None:
    return proto_utils.get_value_at_field(code, system_field)

  fixed_coding_system = annotation_utils.get_fixed_coding_system(code)
  if fixed_coding_system is not None:
    # The entire profiled coding can only be from a single system. Use that.
    return fixed_coding_system

  # There is no single system for the whole coding. Look for the coding system
  # annotation on the enum.
  enum_field = code.DESCRIPTOR.fields_by_name.get('value')
  if (enum_field is None or
      enum_field.type != descriptor.FieldDescriptor.TYPE_ENUM):
    raise fhir_errors.InvalidFhirError(
        f'Invalid profiled Coding: {code.DESCRIPTOR.full_name}; missing system '
        'information on string code.')

  enum_value = proto_utils.get_value_at_field(code, enum_field)
  enum_value_descriptor = enum_field.enum_type.values_by_number[enum_value]
  if not annotation_utils.has_source_code_system(enum_value_descriptor):
    raise fhir_errors.InvalidFhirError(
        f'Invalid profiled Coding: {code.DESCRIPTOR.full_name}; missing system '
        'information on enum code')
  return cast(str,
              annotation_utils.get_source_code_system(enum_value_descriptor))


def get_code_as_string(code: message.Message) -> str:
  """Returns the string representation of a FHIR code."""
  if not fhir_types.is_type_or_profile_of_code(code):
    raise ValueError(
        f'Invalid type for get_code_as_string: {code.DESCRIPTOR.full_name}')

  value_field = code.DESCRIPTOR.fields_by_name.get('value')
  if value_field is None:
    raise ValueError(
        f'Invalid code type for get_code_as_string: {code.DESCRIPTOR.full_name}'
    )

  value = proto_utils.get_value_at_field(code, value_field)
  if value_field.type == descriptor.FieldDescriptor.TYPE_STRING:
    return value
  elif value_field.type == descriptor.FieldDescriptor.TYPE_ENUM:
    return enum_value_descriptor_to_code_string(
        value_field.enum_type.values_by_number[value])
  else:
    raise fhir_errors.InvalidFhirError(
        f'Invalid value field type: {value_field.type!r} for code: '
        f'{code.DESCRIPTOR.full_name}')
