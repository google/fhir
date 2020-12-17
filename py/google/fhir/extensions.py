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
"""Functions to manipulate FHIR extensions.

See also: https://www.hl7.org/fhir/extensibility.html.
"""

import threading
from typing import cast, Any, List, Type, TypeVar

from google.protobuf import descriptor
from google.protobuf import message
from proto.google.fhir.proto import annotations_pb2
from google.fhir import codes
from google.fhir import fhir_errors
from google.fhir.utils import annotation_utils
from google.fhir.utils import fhir_types
from google.fhir.utils import proto_utils

BINARY_SEPARATOR_STRIDE_URL = 'https://g.co/fhir/StructureDefinition/base64Binary-separatorStride'
PRIMITIVE_HAS_NO_VALUE_URL = 'https://g.co/fhir/StructureDefinition/primitiveHasNoValue'
CONVERSION_ONLY_EXTENSION_URLS = frozenset([
    BINARY_SEPARATOR_STRIDE_URL,
    PRIMITIVE_HAS_NO_VALUE_URL,
])

NON_VALUE_FIELDS = frozenset([
    'extension',
    'id',
])

_value_field_map = {}
_value_field_map_cv = threading.Condition(threading.Lock())

_T = TypeVar('_T', bound=message.Message)


def _get_value_field_mapping_for_extension(extension: message.Message):
  """Returns a mapping for each possible value of the extension.value field.

  The mapping is from the full field name of each possible value in the
  Extension.ValueX oneof field to its corresponding FieldDescriptor.

  Args:
    extension: The extension to examine and return a mapping for.

  Returns:
    A mapping from the Extension.ValueX oneof fields' full names to the
    associated FieldDescriptors.

  Raises:
    fhir_errors.InvalidFhirError: In the event that the provided extension
    isn't a valid FHIR extension.
  """
  with _value_field_map_cv:
    if extension.DESCRIPTOR.full_name in _value_field_map:
      return _value_field_map[extension.DESCRIPTOR.full_name]

  value_field = extension.DESCRIPTOR.fields_by_name.get('value')
  if not value_field:
    raise fhir_errors.InvalidFhirError(
        f'Extension: {extension.DESCRIPTOR.full_name} has no "value" field.')

  # Build mapping
  value_field_mapping = {
      field.message_type.full_name: field
      for field in value_field.message_type.fields
  }

  # Cache and return
  with _value_field_map_cv:
    _value_field_map[extension.DESCRIPTOR.full_name] = value_field_mapping
    return _value_field_map[extension.DESCRIPTOR.full_name]


def _verify_field_is_proto_message_type(field: descriptor.FieldDescriptor):
  """Verifies that the provided FieldDescriptor is a protobuf Message."""
  if field.type != descriptor.FieldDescriptor.TYPE_MESSAGE:
    raise ValueError(
        f'Encountered unexpected proto primitive: {field.full_name}. Should be '
        'a FHIR Message type.')


def _get_populated_extension_value_field(
    extension: message.Message) -> descriptor.FieldDescriptor:
  """Return the field descriptor for the oneof field that was set."""
  value = proto_utils.get_value_at_field(extension, 'value')
  field_name = value.WhichOneof('choice')
  if field_name is None:
    raise ValueError(
        f'No value set on extension: {extension.DESCRIPTOR.full_name}.')

  field = value.DESCRIPTOR.fields_by_name[field_name]
  _verify_field_is_proto_message_type(field)
  return field


def _add_extension_value_to_message(extension: message.Message,
                                    msg: message.Message,
                                    message_field: descriptor.FieldDescriptor):
  """Serialize the provided extension and add it to the message.

  Args:
    extension: The FHIR extension to serialize.
    msg: The message to add the serialized extension to.
    message_field: The field on the message to set.

  Raises:
    InvalidFhirError: In the event that the field to be set is not a singular
    message type, or if the provided extension is not singular (has nested
    extensions).
  """
  if message_field.type != descriptor.FieldDescriptor.TYPE_MESSAGE:
    raise fhir_errors.InvalidFhirError(
        f'{msg.DESCRIPTOR.full_name} is not a FHIR extension type.')

  extension_field = extension.DESCRIPTOR.fields_by_name['extension']
  if proto_utils.field_content_length(extension, extension_field) > 0:
    raise fhir_errors.InvalidFhirError('No child extensions should be set on '
                                       f'{extension.DESCRIPTOR.full_name}.')

  value_field = _get_populated_extension_value_field(extension)

  # If a choice type, need to assign the extension value to the correct field.
  if annotation_utils.is_choice_type_field(message_field):
    choice_message = proto_utils.get_value_at_field(msg, message_field)
    choice_descriptor = choice_message.DESCRIPTOR

    for choice_field in choice_descriptor.fields:
      if (value_field.message_type.full_name ==
          choice_field.message_type.full_name):
        _add_extension_value_to_message(extension, choice_message, choice_field)
        return

    raise ValueError(f'No field on Choice Type {choice_descriptor.full_name} '
                     f'for extension {extension.DESCRIPTOR.full_name}.')

  # If the target message is a bound Code type, we need to convert the generic
  # Code field from the extension into the target typed Code.
  if annotation_utils.has_fhir_valueset_url(message_field.message_type):
    typed_code = proto_utils.set_in_parent_or_add(msg, message_field)
    codes.copy_code(cast(Any, extension).value.code, typed_code)
    return

  # If the target message is bound to a Coding type, we must convert the generic
  # Coding field from the extension into the target typed Coding.
  if fhir_types.is_type_or_profile_of_coding(message_field.message_type):
    typed_coding = proto_utils.set_in_parent_or_add(msg, message_field)
    codes.copy_coding(cast(Any, extension).value.coding, typed_coding)
    return

  # Value types must match
  if not proto_utils.are_same_message_type(value_field.message_type,
                                           message_field.message_type):
    raise ValueError('Missing expected value of type '
                     f'{message_field.message_type.full_name} in extension '
                     f'{extension.DESCRIPTOR.full_name}.')

  value = proto_utils.get_value_at_field(
      cast(Any, extension).value, value_field)
  if proto_utils.field_is_repeated(message_field):
    proto_utils.append_value_at_field(msg, message_field, value)
  else:
    proto_utils.set_value_at_field(msg, message_field, value)


def add_extension_to_message(extension: message.Message, msg: message.Message):
  """Recursively parses extension and adds to message.

  Args:
    extension: The FHIR extension to serialize and add.
    msg: The message to add the extension onto

  Raises:
    InvalidFhirError: In the event that a value is set on the extension, but the
    corresponding message field to copy it to is repeated (extension values are
    singular only).
  """
  desc = msg.DESCRIPTOR
  fields_by_url = {
      get_inlined_extension_url(field): field
      for field in desc.fields
      if field.name != 'id'
  }

  # Copy the id field if present
  id_field = desc.fields_by_name.get('id')
  if proto_utils.field_is_set(extension, id_field):
    proto_utils.set_value_at_field(msg, id_field, cast(Any, extension).id)

  # Handle simple extensions (only one value present)
  if proto_utils.field_is_set(extension, 'value'):
    if len(fields_by_url) != 1:
      raise fhir_errors.InvalidFhirError(
          f'Expected a single field, found {len(fields_by_url)}; '
          f'{desc.full_name} is an invalid extension type.')

    field = list(fields_by_url.items())[0][1]
    if proto_utils.field_is_repeated(field):
      raise fhir_errors.InvalidFhirError(
          f'Expected {field.full_name} to be a singular field. '
          f'{desc.full_name} is an invalid extension type.')
    _add_extension_value_to_message(extension, msg, field)
    return

  # Else, iterate through all child extensions...
  child_extensions = proto_utils.get_value_at_field(extension, 'extension')
  for child_extension in child_extensions:
    field = fields_by_url.get(child_extension.url.value)
    if field is None:
      raise ValueError(f'Message of type: {desc.full_name} has no field '
                       f'with name: {child_extension.url.value}.')

    # Simple value type on child_extension...
    if proto_utils.field_is_set(child_extension, 'value'):
      _add_extension_value_to_message(child_extension, msg, field)
      continue

    # Recurse for nested composite messages...
    if not proto_utils.field_is_repeated(field):
      if proto_utils.field_is_set(msg, field):
        raise ValueError(f'Field: {field.full_name} is already set on message: '
                         f'{desc.full_name}.')

      if proto_utils.field_content_length(child_extension, 'extension') > 1:
        raise ValueError(
            f'Cardinality mismatch between field: {field.full_name} and '
            f'extension: {desc.full_name}.')

    child_message = proto_utils.set_in_parent_or_add(msg, field)
    add_extension_to_message(child_extension, child_message)


def extension_to_message(extension: message.Message,
                         message_cls: Type[_T]) -> _T:
  """Serializes a provided FHIR extension into a message of type message_cls.

  This function is a convenience wrapper around add_extension_to_message.

  Args:
    extension: The FHIR extension to serialize.
    message_cls: The type of protobuf message to serialize extension to.

  Returns:
    A message of type message_cls.
  """
  msg = message_cls()
  add_extension_to_message(extension, msg)
  return msg


def _add_fields_to_extension(msg: message.Message, extension: message.Message):
  """Adds the fields from message to extension."""
  for field in msg.DESCRIPTOR.fields:
    _verify_field_is_proto_message_type(field)

    # Add submessages to nested extensions; singular fields have a length of 1
    for i in range(proto_utils.field_content_length(msg, field)):
      child_extension = proto_utils.set_in_parent_or_add(extension, 'extension')
      cast(Any, child_extension).url.value = get_inlined_extension_url(field)
      value = proto_utils.get_value_at_field_index(msg, field, i)
      _add_value_to_extension(value, child_extension,
                              annotation_utils.is_choice_type_field(field))


def _add_value_to_extension(msg: message.Message, extension: message.Message,
                            is_choice_type: bool):
  """Adds the fields from msg to a generic Extension.

  Attempts are first made to set the "value" field of the generic Extension
  based on the type of field set on message. If this fails, checks are made
  against the generic Code and Coding types, and finally we fall back to adding
  the message's fields as sub-extensions.

  Args:
    msg: The message whose values to add to extension.
    extension: The generic Extension to populate.
    is_choice_type: Whether or not the provided message represents a "choice"
    type.
  """
  if is_choice_type:
    oneofs = msg.DESCRIPTOR.oneofs
    if not oneofs:
      raise fhir_errors.InvalidFhirError(
          f'Choice type is missing a oneof: {msg.DESCRIPTOR.full_name}')
    value_field_name = msg.WhichOneof(oneofs[0].name)
    if value_field_name is None:
      raise ValueError('Choice type has no value set: '
                       f'{msg.DESCRIPTOR.full_name}')
    value_field = msg.DESCRIPTOR.fields_by_name[value_field_name]
    _verify_field_is_proto_message_type(value_field)
    _add_value_to_extension(
        proto_utils.get_value_at_field(msg, value_field), extension, False)
  else:
    # Try to set the message directly as a datatype value on the extension.
    # E.g., put the message of type Boolean into the value.boolean field
    value_field_mapping = _get_value_field_mapping_for_extension(extension)
    value_field = value_field_mapping.get(msg.DESCRIPTOR.full_name)
    if value_field is not None:
      proto_utils.set_value_at_field(
          cast(Any, extension).value, value_field, msg)
    elif annotation_utils.has_fhir_valueset_url(msg):
      codes.copy_code(msg, cast(Any, extension).value.code)
    elif fhir_types.is_type_or_profile_of_coding(msg):
      codes.copy_coding(msg, cast(Any, extension).value.coding)
    else:  # Fall back to adding individual fields as sub-extensions
      _add_fields_to_extension(msg, extension)


def add_message_to_extension(msg: message.Message, extension: message.Message):
  """Adds the contents of msg to extension.

  Args:
    msg: A FHIR profile of Extension, whose contents should be added to the
      generic extension.
    extension: The generic Extension to populate.
  """
  if not fhir_types.is_profile_of_extension(msg):
    raise ValueError(f'Message: {msg.DESCRIPTOR.full_name} is not a valid '
                     'FHIR Extension profile.')

  if not fhir_types.is_extension(extension):
    raise ValueError(f'Extension: {extension.DESCRIPTOR.full_name} is not a '
                     'valid FHIR Extension.')

  cast(Any,
       extension).url.value = annotation_utils.get_structure_definition_url(
           msg.DESCRIPTOR)

  # Copy over the id field if present
  if proto_utils.field_is_set(msg, 'id'):
    proto_utils.copy_common_field(msg, extension, 'id')

  # Copy the vlaue fields from message into the extension
  value_fields = [
      field for field in msg.DESCRIPTOR.fields
      if field.name not in NON_VALUE_FIELDS
  ]
  if not value_fields:
    raise ValueError(f'Extension has no value fields: {msg.DESCRIPTOR.name}.')

  # Add fields to the extension. If there is a single value field, a simple
  # value assignment will suffice. Otherwise, we need to loop over all fields
  # and add them as child extensions
  if (len(value_fields) == 1 and
      not proto_utils.field_is_repeated(value_fields[0])):
    value_field = value_fields[0]
    _verify_field_is_proto_message_type(value_field)
    if proto_utils.field_is_set(msg, value_field):
      value = proto_utils.get_value_at_field(msg, value_field)
      _add_value_to_extension(
          value, extension, annotation_utils.is_choice_type_field(value_field))
    else:
      # TODO: Invalid FHIR; throw an error here?
      pass
  else:  # Add child extensions...
    _add_fields_to_extension(msg, extension)


def message_to_extension(msg: message.Message, extension_cls: Type[_T]) -> _T:
  """Converts an Extension profile into a generic Extension type.

  Args:
    msg: The Message to convert.
    extension_cls: The type of FHIR Extension to convert to.

  Returns:
    A an instance of extension_cls.
  """
  extension = extension_cls()
  add_message_to_extension(msg, extension)
  return extension


def clear_fhir_extensions_with_url(msg: message.Message, url: str):
  """Removes FHIR extensions that have the provided url."""
  fhir_extensions = get_fhir_extensions(msg)
  updated_fhir_extensions = [
      extension for extension in fhir_extensions
      if cast(Any, extension).url.value != url
  ]
  proto_utils.set_value_at_field(msg, 'extension', updated_fhir_extensions)


def get_fhir_extensions(msg: message.Message) -> List[message.Message]:
  """Returns a list of FHIR extensions from the provided FHIR primitive.

  Args:
    msg: The Message to extract FHIR extensions from.

  Returns:
    A list of Extension values found on the instance.

  Raises:
    ValueError: If the provided FHIR primitive doesn't have an extension
    attribute.
  """
  extension_field = msg.DESCRIPTOR.fields_by_name.get('extension')

  if extension_field is None:
    raise ValueError(f'Message of type {msg.DESCRIPTOR.name} does not have '
                     'an extension attribute.')
  return proto_utils.get_value_at_field(msg, extension_field)


def get_inlined_extension_url(field: descriptor.FieldDescriptor) -> str:
  """Returns the FHIR inlined extension URL for a field.

  Args:
    field: The FieldDescriptor to examine.

  Returns:
    The FHIR inlined extension URL, if one exists, otherwise returns the camel-
    case name of the FieldDescriptor.
  """
  options = annotation_utils.get_options(field)
  if options.HasExtension(annotations_pb2.fhir_inlined_extension_url):
    return options.Extensions[annotations_pb2.fhir_inlined_extension_url]
  return field.camelcase_name


def get_repeated_from_extensions(extension_list: List[message.Message],
                                 repeated_cls: Type[_T]) -> List[_T]:
  """Extracts matching extensions from extension_list and serializes to protos.

  Args:
    extension_list: The list of FHIR extensions to examine.
    repeated_cls: The type of message to serialize to.

  Returns:
    A list of protos of instance repeated_cls representing the extensions within
    extension_list.
  """
  result = []
  if not extension_list:
    return result  # Early-exit

  url = annotation_utils.get_structure_definition_url(repeated_cls.DESCRIPTOR)
  for extension in extension_list:
    if cast(Any, extension).url.value == url:
      msg = extension_to_message(extension, repeated_cls)
      result.append(msg)

  return result


def create_primitive_has_no_value(
    desc: descriptor.Descriptor) -> message.Message:
  """Returns a PrimitiveHasNoValue extension provided a descriptor."""
  primitive_has_no_value = proto_utils.create_message_from_descriptor(desc)

  # Silencing the type checkers as pytype doesn't fully support structural
  # subtyping yet.
  # pylint: disable=line-too-long
  # See: https://mypy.readthedocs.io/en/stable/casts.html#casts-and-type-assertions.
  # pylint: enable=line-too-long
  # Soon: https://www.python.org/dev/peps/pep-0544/.
  cast(Any, primitive_has_no_value).url.value = PRIMITIVE_HAS_NO_VALUE_URL
  cast(Any, primitive_has_no_value).value.boolean.value = True

  return primitive_has_no_value
