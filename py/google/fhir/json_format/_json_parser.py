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
"""Functionality for parsing FHIR JSON into protobuf format."""

import threading
from typing import Any, Dict

from google.protobuf import any_pb2
from google.protobuf import descriptor
from google.protobuf import message
from google.fhir import extensions
from google.fhir import primitive_handler
from google.fhir import references
from google.fhir.utils import annotation_utils
from google.fhir.utils import proto_utils

_field_map_memos = {}
_field_map_memos_cv = threading.Condition(threading.Lock())


def _get_nested_choice_field_name(field: descriptor.FieldDescriptor,
                                  child_field_name: str) -> str:
  """Outputs the proper representation of field_name for a Choice type."""
  if child_field_name.startswith('_'):
    # For primitive extensions, prepend the leading underscore, e.g.:
    # value + _boolean = _valueBoolean
    return ('_' + field.json_name + child_field_name[1].upper() +
            child_field_name[1:])
  else:
    # Otherwise, just append together the JSON name, e.g.:
    # value + boolean = valueBoolean
    return (field.json_name + child_field_name[0].upper() +
            child_field_name[1:])


def _get_choice_field_name(field: descriptor.FieldDescriptor,
                           nested_field_name: str) -> str:
  """Returns the Choice type field name from a nested field name."""
  base_index = len(field.json_name)
  if nested_field_name.startswith('_'):
    # For primitive extensions, prepent the leading underscore, e.g.:
    # _valueBoolean = boolean
    return ('_' + nested_field_name[base_index + 1].lower() +
            nested_field_name[base_index + 2:])
  else:
    # Otherwise, just extract the field name, e.g.:
    # valueBoolean = boolean
    return (nested_field_name[base_index].lower() +
            nested_field_name[base_index + 1:])


def _get_field_map(
    desc: descriptor.Descriptor) -> Dict[str, descriptor.FieldDescriptor]:
  """Returns a mapping between field name and FieldDescriptor.

  Note that for FHIR "Choice" types, field names of primitive extensions are
  prepended with a leading underscore, e.g.: value + _boolean = _valueBoolean.

  Args:
    desc: The Descriptor of the Message whose mapping to return.

  Returns:
    A mapping between the field name and its corresponding FieldDescriptor.
  """
  # Early exit if we've already constructed the field mapping
  with _field_map_memos_cv:
    if desc in _field_map_memos:
      return _field_map_memos[desc]

  # Build field mapping
  field_map = {}
  for field in desc.fields:
    if (field.type == descriptor.FieldDescriptor.TYPE_MESSAGE and
        annotation_utils.is_choice_type_field(field)):
      inner_map = _get_field_map(field.message_type)
      for (child_field_name, _) in inner_map.items():
        choice_field_name = _get_nested_choice_field_name(
            field, child_field_name)
        field_map[choice_field_name] = field
    else:
      field_map[field.json_name] = field

      # FHIR JSON represents extensions to primitive fields as separate
      # standalone JSON objects, keyed by '_' + field_name
      if (field.type == descriptor.FieldDescriptor.TYPE_MESSAGE and
          annotation_utils.is_primitive_type(field.message_type)):
        field_map['_' + field.json_name] = field

  # Cache result
  with _field_map_memos_cv:
    _field_map_memos[desc] = field_map
    return _field_map_memos[desc]


class JsonParser:
  """A JSON to FHIR protobuf parser."""

  @classmethod
  def json_parser_with_default_timezone(
      cls, primitive_handler_: primitive_handler.PrimitiveHandler,
      default_timezone: str):
    """Returns a new parser initialized with the default_timezone."""
    return cls(primitive_handler_, default_timezone)

  def __init__(self, primitive_handler_: primitive_handler.PrimitiveHandler,
               default_timezone: str):
    """Initializes an instance of the FHIR JSON parser.

    Note that this is for *internal-use* only. External clients should leverage
    one of the available class constructors, such as:
    `JsonParser.json_parser_with_default_timezone(...)`.

    Args:
      primitive_handler_: Responsible for returning PrimitiveWrappers.
      default_timezone: The string representation of the timezone to default-to
        when parsing time-like values.
    """
    self.primitive_handler = primitive_handler_
    self.default_timezone = default_timezone

    # Mapping from field name to field on the appropriate ContainedResource
    self._resource_type_mapping = {
        field.message_type.name: field
        for field in primitive_handler_.contained_resource_cls.DESCRIPTOR.fields
    }

  def _parse_field_value(self, field: descriptor.FieldDescriptor,
                         json_value: Any) -> message.Message:
    """Returns a new Message described by the FieldDescriptor and json_value.

    Args:
      field: The FieldDescriptor of the Message instance to create.
      json_value: The JSON value representation to merge into the newly created
        Message.

    Returns:
      A new Message as described by the provided FieldDescriptor merged with the
      contents of json_value.
    """
    if field.type != descriptor.FieldDescriptor.TYPE_MESSAGE:
      raise ValueError('Error in FHIR proto definition, field: '
                       f'{field.full_name} is not a message.')
    if field.message_type.full_name == any_pb2.Any.DESCRIPTOR.full_name:
      contained = self.primitive_handler.new_contained_resource()
      self._merge_contained_resource(json_value, contained)

      any_message = any_pb2.Any()
      any_message.Pack(contained)
      return any_message
    else:
      target = proto_utils.create_message_from_descriptor(field.message_type)
      self.merge_value(json_value, target)
      return target

  def _merge_choice_field(self, json_value: Any,
                          choice_field: descriptor.FieldDescriptor,
                          field_name: str, parent: message.Message):
    """Creates a Message based on the choice_field Descriptor and json_value.

    The resulting message is merged into parent.

    Args:
      json_value: The JSON value to merge into a message of the type described
        by choice_field.
      choice_field: The field descriptor of the FHIR choice type on parent.
      field_name: The nested field name of the choice type, e.g.: _valueBoolean.
      parent: The parent Message to merge into.
    """
    choice_field_name = _get_choice_field_name(choice_field, field_name)
    choice_field_map = _get_field_map(choice_field.message_type)
    choice_value_field = choice_field_map.get(choice_field_name)
    if choice_value_field is None:
      raise ValueError(f'Cannot find {choice_field_name!r} on '
                       f'{choice_field.full_name}')

    choice_message = proto_utils.set_in_parent_or_add(parent, choice_field)
    self._merge_field(json_value, choice_value_field, choice_message)

  def _merge_field(self, json_value: Any, field: descriptor.FieldDescriptor,
                   parent: message.Message):
    """Merges the json_value into the provided field of the parent Message.

    Args:
      json_value: The JSON value to set.
      field: The FieldDescriptor of the field to set in parent.
      parent: The parent Message to set the value on.

    Raises:
      ValueError: In the event that a non-primitive field has already been set.
      ValueError: In the event that a oneof field has already been set.
    """
    # If the field is non-primitive, ensure that it hasn't been set yet. Note
    # that we allow primitive types to be set already, because FHIR represents
    # extensions to primitives as separate, subsequent JSON elements, with the
    # field prepended by an underscore
    if (not annotation_utils.is_primitive_type(field.message_type) and
        proto_utils.field_is_set(parent, field)):
      raise ValueError('Target field {} is already set.'.format(
          field.full_name))

    # Consider it an error if a oneof field is already set.
    #
    # Exception: When a primitive in a choice type has a value and an extension,
    # it will get set twice, once by the value (e.g. valueString) and once by an
    # extension (e.g., _valueString)
    if field.containing_oneof is not None:
      oneof_field = parent.DESCRIPTOR.oneofs_by_name[
          field.containing_oneof.name]
      if (annotation_utils.is_primitive_type(field.message_type) and
          oneof_field.full_name == field.full_name):
        raise ValueError(
            'Cannot set field {} since oneof field {} is already set.'.format(
                field.full_name, oneof_field.full_name))

    # Ensure that repeated fields get proper list assignment
    existing_field_size = proto_utils.field_content_length(parent, field)
    if proto_utils.field_is_repeated(field):
      if not isinstance(json_value, list):
        raise ValueError(
            f'Attempted to merge a repeated field, {field.name}, a json_value '
            f'with type {type(json_value)} instead of a list.')

      if existing_field_size != 0 and existing_field_size != len(json_value):
        raise ValueError(
            'Repeated primitive list length does not match extension list for '
            'field: {field.full_name!r}.')

    # Set the JSON values, taking care to clear the PRIMITIVE_HAS_NO_VALUE_URL
    # if we've already visited the field before.
    json_value = (
        json_value if proto_utils.field_is_repeated(field) else [json_value])
    for (i, value) in enumerate(json_value):
      parsed_value = self._parse_field_value(field, value)
      if existing_field_size > 0:
        field_value = proto_utils.get_value_at_field_index(parent, field, i)
        field_value.MergeFrom(parsed_value)
        extensions.clear_fhir_extensions_with_url(
            field_value, extensions.PRIMITIVE_HAS_NO_VALUE_URL)
      else:
        field_value = proto_utils.set_in_parent_or_add(parent, field)
        field_value.MergeFrom(parsed_value)

  def _merge_contained_resource(self, json_value: Dict[str, Any],
                                target: message.Message):
    """Merges json_value into a contained resource field within target."""
    # We handle contained resources in a special way, because despite internally
    # being a oneof, it is not actually a chosen-type in FHIR. The JSON field
    # name is just 'resource', which doesn't give any clues about which field in
    # the oneof to set. Instead, we need to inspect the JSON input to determine
    # its type. Then, merge into that specific fields in the resource oneof.
    resource_type = json_value.get('resourceType')
    contained_field = self._resource_type_mapping.get(resource_type)
    if contained_field is None:
      raise ValueError('Unable to retrieve ContainedResource field for '
                       f'{resource_type}.')
    contained_message = proto_utils.set_in_parent_or_add(
        target, contained_field)
    self._merge_message(json_value, contained_message)

  def _merge_message(self, json_value: Dict[str, Any], target: message.Message):
    """Merges the provided json object into the target Message."""
    target_descriptor = target.DESCRIPTOR
    if target_descriptor.name == 'ContainedResource':
      self._merge_contained_resource(json_value, target)
      return

    # For each field/value present in the JSON value, merge into target...
    field_map = _get_field_map(target_descriptor)
    for (field_name, value) in json_value.items():
      field = field_map.get(field_name)
      if field is not None:
        if annotation_utils.is_choice_type_field(field):
          self._merge_choice_field(value, field, field_name, target)
        else:
          self._merge_field(value, field, target)
      elif field_name == 'resourceType':
        if (not annotation_utils.is_resource(target_descriptor) or
            target_descriptor.name != value):
          raise ValueError(f'Error merging JSON resource into message of type '
                           f'{target_descriptor.name}.')
      else:
        raise ValueError(
            f'Unable to merge {field_name!r} into {target_descriptor.name}.')

  def merge_value(self, json_value: Any, target: message.Message):
    """Merges the provided json_value into the target Message.

    Args:
      json_value: A Python-native representation of JSON data.
      target: The target Message to merge the JSON data into.
    """
    target_descriptor = target.DESCRIPTOR
    if annotation_utils.is_primitive_type(target_descriptor):
      if isinstance(json_value, dict):
        # This is a primitive type extension. Merge the extension fields into
        # the empty target proto, and tag it as having no value.
        self._merge_message(json_value, target)

        extension_field = target_descriptor.fields_by_name.get('extension')
        if extension_field is None:
          raise ValueError("Invalid primitive. No 'extension' field exists on "
                           f"{target_descriptor.full_name}.")
        primitive_has_no_value = extensions.create_primitive_has_no_value(
            extension_field.message_type)
        proto_utils.append_value_at_field(target, extension_field,
                                          primitive_has_no_value)
      else:
        wrapper = self.primitive_handler.primitive_wrapper_from_json_value(
            json_value, type(target), default_timezone=self.default_timezone)
        wrapper.merge_into(target)
    elif annotation_utils.is_reference(target_descriptor):
      self._merge_message(json_value, target)
      references.split_if_relative_reference(target)
    else:
      if isinstance(json_value, dict):
        # The provided value must be another FHIR element
        self._merge_message(json_value, target)
      elif isinstance(json_value, (tuple, list)) and len(json_value) == 1:
        # Check if the target field is non-repeated, and we're trying to
        # populate it with a single-element array. This is considered valid, and
        # occurs when a profiled resource reduces the size of a repeated FHIR
        # field to a maximum of 1.
        self._merge_message(json_value[0], target)
      else:
        raise ValueError('Expected a JSON object for field of type: {}.'.format(
            target_descriptor.full_name))
