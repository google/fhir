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
"""Utilities to make it easier to work with proto reflection."""
from typing import Any, Type, Union

from google.protobuf import descriptor
from google.protobuf import message
from google.protobuf import message_factory

_factory = message_factory.MessageFactory()


def _field_descriptor_for_name(msg: message.Message,
                               field_name: str) -> descriptor.FieldDescriptor:
  """Returns the FieldDescriptor corresponding to field_name."""
  result = msg.DESCRIPTOR.fields_by_name.get(field_name)
  if result is None:
    raise ValueError('%s is not present on: %s.' %
                     (repr(field_name), msg.DESCRIPTOR.name))
  return result


def are_same_message_type(descriptor_a: descriptor.Descriptor,
                          descriptor_b: descriptor.Descriptor) -> bool:
  """Returns True if descriptor_a is the same type as descriptor_b."""
  return (descriptor_a == descriptor_b or
          descriptor_a.full_name == descriptor_b.full_name)


def is_message_type(msg: message.Message,
                    message_cls: Type[message.Message]) -> bool:
  """Returns True if message is of type message_cls."""
  return are_same_message_type(msg.DESCRIPTOR, message_cls.DESCRIPTOR)


def field_is_primitive(field_descriptor: descriptor.FieldDescriptor) -> bool:
  """Returns True if the field_descriptor is of a primitive type."""
  return field_descriptor.message_type is None


def field_is_repeated(field_descriptor: descriptor.FieldDescriptor) -> bool:
  """Indicates if the provided field_descriptor describes a repeated field."""
  return field_descriptor.label == descriptor.FieldDescriptor.LABEL_REPEATED


def field_content_length(msg: message.Message,
                         field: Union[descriptor.FieldDescriptor, str]) -> int:
  """Returns the size of the field.

  Args:
    msg: The Message whose fields to examine.
    field: The FieldDescriptor or name of the field to examine.

  Returns:
    The number of elements at the provided field. If field describes a singular
    protobuf field, this will return 1. If the field is not set, returns 0.
  """
  if isinstance(field, str):
    field = _field_descriptor_for_name(msg, field)

  if field_is_repeated(field):
    return len(getattr(msg, field.name))
  return 1 if msg.HasField(field.name) else 0


def field_is_set(msg: message.Message, field: Union[descriptor.FieldDescriptor,
                                                    str]) -> bool:
  """Returns True if the field is set.

  Args:
    msg: The Message whose fields to examine.
    field: The FieldDescriptor or name of the field to examine.

  Returns:
    True if field has been set.
  """
  return field_content_length(msg, field) > 0


# TODO: Add a copy=False default parameter to allow for
# deep-copying of reference types to the caller (right now, shallow copies are
# returned).
def get_value_at_field(msg: message.Message,
                       field: Union[descriptor.FieldDescriptor, str]) -> Any:
  """Returns the value at the field desribed by field.

  Args:
    msg: The message whose fields to examine.
    field: The FieldDescriptor or name of the field to retrieve.

  Returns:
    The value of msg at field.
  """
  if isinstance(field, str):
    field = _field_descriptor_for_name(msg, field)

  if field_is_repeated(field):
    return getattr(msg, field.name)[:]
  return getattr(msg, field.name)


def append_value_at_field(msg: message.Message,
                          field: Union[descriptor.FieldDescriptor,
                                       str], value: Any):
  """Appends the provided value to the repeated field.

  Args:
    msg: The message whose field to append to.
    field: The FieldDescriptor or name of the field to append to.
    value: The value to append.

  Raises:
    ValueError: In the event that the field is not repeated.
  """
  if isinstance(field, str):
    field = _field_descriptor_for_name(msg, field)

  if not field_is_repeated(field):
    raise ValueError('%s is not repeated. Unable to append: %s.' %
                     (field.name, repr(value)))
  getattr(msg, field.name).append(value)


def set_value_at_field(msg: message.Message,
                       field: Union[descriptor.FieldDescriptor,
                                    str], value: Any):
  """Sets value at the field.

  Args:
    msg: The message whose field to mutate.
    field: The FieldDescriptor or name of the field to mutate.
    value: The value to set.
  """
  if isinstance(field, str):
    field = _field_descriptor_for_name(msg, field)

  if field_is_repeated(field):
    if field_is_primitive(field):
      getattr(msg, field.name)[:] = value
    else:  # Composite Group or Message type
      del getattr(msg, field.name)[:]
      getattr(msg, field.name).extend(value)
  else:  # Singular field
    if field_is_primitive(field):
      setattr(msg, field.name, value)
    else:  # Composite Group or Message type
      getattr(msg, field.name).CopyFrom(value)


def get_value_at_field_index(msg: message.Message,
                             field: Union[descriptor.FieldDescriptor,
                                          str], index: int) -> Any:
  """Returns the value at index for the provided field.

  Calling this method on a singular field with an index of 0 is identicial to
  calling get_value_at_field. Providing an index other than 0 in this case
  raises
  an exception.

  Args:
    msg: The Message whose fields to examine.
    field: The FieldDescriptor or name of the field to examine.
    index: The index of the value to retrieve.

  Returns:
    The value at index for the field described by field.

  Raises:
    ValueError: Attempted to get index beyond size of field: <field>.
    ValueError: Attempted to get non-zero index on singular field: <field>.
  """
  if isinstance(field, str):
    field = _field_descriptor_for_name(msg, field)

  if field_is_repeated(field):
    if index >= field_content_length(msg, field):
      raise ValueError('Attempted to get index beyond size of field: %s.' %
                       field.name)
    return get_value_at_field(msg, field)[index]

  # Singular field
  if index != 0:
    raise ValueError('Attempted to get non-zero index on singular field: %s.' %
                     field.name)
  return get_value_at_field(msg, field)


def set_value_at_field_index(msg: message.Message,
                             field: Union[descriptor.FieldDescriptor,
                                          str], index: int, value: Any):
  """Sets value at index for the provided field.

  Args:
    msg: The Message whose field to set.
    field: The FieldDescriptor or name of the field to to set.
    index: The index of the field to set.
    value: The value to set.

  Raises:
    ValueError: Attempted to set index beyond size of field: <field>.
    ValueError: Attempted to set non-zero index on singular field: <field>.
  """
  if isinstance(field, str):
    field = _field_descriptor_for_name(msg, field)

  if field_is_repeated(field):
    if index >= field_content_length(msg, field):
      raise ValueError('Attempted to set index beyond size of field: %s.' %
                       field.name)
    if field_is_primitive(field):
      getattr(msg, field.name)[index] = value
    else:  # Repeated composite fields do not support item assignment
      new_values = getattr(msg, field.name)[:]
      new_values[index] = value
      set_value_at_field(msg, field, new_values)
  else:  # Singular field
    if index != 0:
      raise ValueError(
          'Attempted to set non-zero index on singular field: %s.' % field.name)
    set_value_at_field(msg, field, value)


def set_in_parent_or_add(
    msg: message.Message, field: Union[descriptor.FieldDescriptor,
                                       str]) -> message.Message:
  """Creates a new default instance of the type at field in message.

  If this field is repeated, this function is equivalent to calling <field>.add,
  otherwise, this function is equivalent to calling <field>.SetInParent and
  returning the reference.

  Args:
    msg: The parent message whose composite message to create and return.
    field: The FieldDescriptor or name of the field that the composite message
      is stored in.

  Returns:
    A reference to a newly created message of the type at field which is a child
    of parent_message.

  Raises:
    ValueError: If the provided FieldDescriptor is a primitive type.
  """
  if isinstance(field, str):
    field = _field_descriptor_for_name(msg, field)

  if field_is_primitive(field):
    raise ValueError('Expected a composite message type at: %s.' % field.name)

  if field_is_repeated(field):
    return getattr(msg, field.name).add()
  getattr(msg, field.name).SetInParent()
  return getattr(msg, field.name)


def get_message_class_from_descriptor(
    desc: descriptor.Descriptor) -> Type[message.Message]:
  """Returns the class of the message type corresponding to the descriptor."""
  return _factory.GetPrototype(desc)


def create_message_from_descriptor(desc: descriptor.Descriptor,
                                   **kwargs: Any) -> message.Message:
  """Instantiates a new Message based on a provided Descriptor.

  Args:
    desc: The Descriptor that describes the Message to instantiate.
    **kwargs: An optional list of key/value pairs to initialize with.

  Returns:
    A new Message based on the provided Descriptor.
  """
  return get_message_class_from_descriptor(desc)(**kwargs)


def copy_common_field(source_message: message.Message,
                      target_message: message.Message, field_name: str):
  """Copies field named field_name from source_message to target_message.

  Args:
    source_message: The message to copy values from.
    target_message: The message to copy values to.
    field_name: The common field to examine/copy from source_message to
      target_message.

  Raises:
    ValueError: <source> is not the same type as <target>. Unable to copy field
    <field>.
    ValueError: Field <field> is not present in both <source> and <target>.
    ValueError: Field <field> differs in type between <source> (<type>) and
    <target> (<type>).
    ValueError: Field <field> differs in size between <source> and <target>.
  """
  source_descriptor = source_message.DESCRIPTOR
  target_descriptor = target_message.DESCRIPTOR
  source_field = _field_descriptor_for_name(source_message, field_name)
  target_field = _field_descriptor_for_name(target_message, field_name)

  if source_field.type != target_field.type:
    raise ValueError(
        'Field {} differs in type between {} ({}) and {} ({}).'.format(
            field_name, source_descriptor.full_name, source_field.type,
            target_descriptor.full_name, target_field.type))

  if field_is_repeated(source_field) != field_is_repeated(target_field):
    raise ValueError('Field {} differs in size between {} and {}.'.format(
        field_name, source_descriptor.full_name, target_descriptor.full_name))

  if field_is_set(source_message, source_field):
    source_value = get_value_at_field(source_message, source_field)
    set_value_at_field(target_message, target_field, source_value)
