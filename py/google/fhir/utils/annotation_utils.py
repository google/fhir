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
"""A collection of functions for dealing with FHIR protobuf annotations."""

from typing import Any, List, Optional, Union

from google.protobuf import descriptor_pb2
from google.protobuf import descriptor
from google.protobuf import message
from proto.google.fhir.proto import annotations_pb2
from google.fhir.utils import proto_utils

MessageOrDescriptorBase = Union[message.Message, descriptor.DescriptorBase]

DescriptorOptions = Union[descriptor_pb2.MessageOptions,
                          descriptor_pb2.FieldOptions,
                          descriptor_pb2.EnumOptions,
                          descriptor_pb2.EnumValueOptions,
                          descriptor_pb2.OneofOptions,
                          descriptor_pb2.ServiceOptions,
                          descriptor_pb2.MethodOptions,
                          descriptor_pb2.FileOptions,]


def is_resource(message_or_descriptor: MessageOrDescriptorBase) -> bool:
  """Returns true if message_or_descriptor is of a resource type.

  Args:
    message_or_descriptor: A protobuf Message or DescriptorBase to examine.

  Returns:
    A Boolean indicating whether or not message_or_descriptor is a resource.

  Raises:
    ValueError: Unable to retrieve options for type: <type>.
  """
  return (get_value_for_annotation_extension(
      message_or_descriptor, annotations_pb2.structure_definition_kind) ==
          annotations_pb2.StructureDefinitionKindValue.KIND_RESOURCE)


def is_primitive_type(message_or_descriptor: MessageOrDescriptorBase) -> bool:
  """Returns true if message_or_descriptor is of a primitive type.

  Args:
    message_or_descriptor: A protobuf Message or DescriptorBase to examine.

  Returns:
    A Boolean indicating whether or not message_or_descriptor is a primitive.

  Raises:
    ValueError: Unable to retrieve options for type: <type>.
  """
  return (get_value_for_annotation_extension(
      message_or_descriptor, annotations_pb2.structure_definition_kind) ==
          annotations_pb2.StructureDefinitionKindValue.KIND_PRIMITIVE_TYPE)


def is_choice_type_field(field_descriptor: descriptor.FieldDescriptor) -> bool:
  """Returns true if field_descriptor describes a choice type.

  Args:
    field_descriptor: The FieldDescriptor to examine.

  Returns:
    A Boolean describing whether or not the field is a choice type.

  Raises:
    ValueError: Unable to retrieve options for type: <type>.
  """
  return (field_descriptor.type == descriptor.FieldDescriptor.TYPE_MESSAGE and
          bool(
              get_value_for_annotation_extension(
                  field_descriptor.message_type,
                  annotations_pb2.is_choice_type)))


def is_reference(message_or_descriptor: MessageOrDescriptorBase) -> bool:
  """Returns true if message_or_descriptor is of a reference type.

  Args:
    message_or_descriptor: A protobuf Message or DescriptorBase to examine.

  Returns:
    A Boolean indicating whether or not message_or_descriptor is a reference.

  Raises:
    ValueError: Unable to retrieve options for type: <type>.
  """
  return bool(
      get_value_for_annotation_extension(message_or_descriptor,
                                         annotations_pb2.fhir_reference_type))


def is_typed_reference_field(
    field_descriptor: descriptor.FieldDescriptor) -> bool:
  """Returns true if field_descriptor describes a referenced resource.

  This annotation is expected to be present on typed relative URLs stored in a
  FHIR Reference protobuf message.

  Args:
    field_descriptor: A FieldDescriptor to examine.

  Returns:
    A Boolean indicating whether or not message_or_descriptor is a referenced
    FHIR type.

  Raises:
    ValueError: Unable to retrieve options for type: <type>.
  """
  return (get_value_for_annotation_extension(
      field_descriptor, annotations_pb2.referenced_fhir_type) is not None)


def field_is_required(field_descriptor: descriptor.FieldDescriptor) -> bool:
  """Returns true if field_desriptor is marked as 'required' by FHIR.

  Args:
    field_descriptor: A FieldDescriptor to examine.

  Returns:
    A Boolean indicating whether or not field_descriptor is required.

  Raises:
    ValueError: Unable to retrieve options for type: <type>.
  """
  return (get_value_for_annotation_extension(
      field_descriptor, annotations_pb2.validation_requirement) ==
          annotations_pb2.Requirement.REQUIRED_BY_FHIR)


def get_fixed_coding_system(
    message_or_descriptor: MessageOrDescriptorBase) -> Optional[str]:
  """Returns the value associated with the fhir_fixed_system annotation.

  Args:
    message_or_descriptor: A protobuf Message or DescriptorBase to examine.

  Returns:
    The string value associated with the fhir_fixed_system annotation, if one
    exists. Otherwise, returns None.

  Raises:
    ValueError: Unable to retrieve options for type: <type>.
  """
  return get_value_for_annotation_extension(message_or_descriptor,
                                            annotations_pb2.fhir_fixed_system)


def get_source_code_system(
    enum_value_descriptor: descriptor.EnumValueDescriptor) -> Optional[str]:
  """Returns the value associated with the source_code_system annotation.

  Args:
    enum_value_descriptor: An EnumValueDescriptor describing a FHIR Code for
      which to return a source code system.

  Returns:
    The string value of the source code system, if one exists. Otherwise,
    returns None.

  Raises:
    ValueError: Unable to retrieve options for type: <type>.
  """
  return get_value_for_annotation_extension(enum_value_descriptor,
                                            annotations_pb2.source_code_system)


def get_value_regex_for_primitive_type(
    message_or_descriptor: MessageOrDescriptorBase) -> Optional[str]:
  """Returns the value regex associated with a primitive type.

  Args:
    message_or_descriptor: A protobuf Message or DescriptorBase to examine.

  Returns:
    The value regex associated with a given primitive, or None if the provided
    message_or_descriptor is not a primitive and/or the extension does not
    exist.

  Raises:
    ValueError: Unable to retrieve options for type: <type>.
  """
  if is_primitive_type(message_or_descriptor):
    return get_value_for_annotation_extension(message_or_descriptor,
                                              annotations_pb2.value_regex)
  return None


def get_structure_definition_url(
    message_or_descriptor: MessageOrDescriptorBase) -> Optional[str]:
  """Returns the URL for the structure definition the message was built from.

  Args:
    message_or_descriptor: A protobuf Message or DescriptorBase to examine.

  Returns:
    The URL for the structure definition the corresponding message was built
    from. Otherwise returns None if the extension does not exist.

  Raises:
    ValueError: Unable to retrieve options for type: <type>.
  """
  return get_value_for_annotation_extension(
      message_or_descriptor, annotations_pb2.fhir_structure_definition_url)


def get_fhir_valueset_url(
    message_or_descriptor: MessageOrDescriptorBase) -> Optional[str]:
  """Returns a code's valueset identifier.

  Args:
    message_or_descriptor: A protobuf Message or DescriptorBase to examine.

  Returns:
    If the message being described is a Code constrained to a specific valueset,
    returns the valueset identifier. Otherwise returns None.

  Raises:
    ValueError: Unable to retrieve options for type: <type>.
  """
  return get_value_for_annotation_extension(message_or_descriptor,
                                            annotations_pb2.fhir_valueset_url)


def get_enum_valueset_url(
    enum_descriptor: descriptor.EnumDescriptor) -> Optional[str]:
  """If the descriptor is a ValueSet enum, returns the url for the CodeSystem.

  Args:
    enum_descriptor: The EnumDescriptor to examine.

  Returns:
    If the message being described is a ValueSet enum, returns the URL for the
    CodeSystem. Otherwise returns None.

  Raises:
    ValueError: Unable to retrieve options for type: <type>.
  """
  return get_value_for_annotation_extension(enum_descriptor,
                                            annotations_pb2.enum_valueset_url)


def get_enum_value_original_code(
    enum_value_descriptor: descriptor.EnumValueDescriptor) -> Optional[str]:
  """Returns the original name if the provided enum value had to be renamed.

  Args:
    enum_value_descriptor: The EnumValueDescriptor to examine.

  Returns:
    If the code had to be renamed to make a valid enum identifier, this function
    returns the original name. Otherwise returns None.

  Raises:
    ValueError: Unable to retrieve options for type: <type>.
  """
  return get_value_for_annotation_extension(enum_value_descriptor,
                                            annotations_pb2.fhir_original_code)


def has_fhir_valueset_url(
    message_or_descriptor: MessageOrDescriptorBase) -> bool:
  """Returns True if message_or_descriptor has a FHIR valueset url."""
  return get_fhir_valueset_url(message_or_descriptor) is not None


def has_source_code_system(
    enum_value_descriptor: descriptor.EnumValueDescriptor) -> bool:
  """Returns True if enum_value_descriptor has a source code system."""
  return get_source_code_system(enum_value_descriptor) is not None


def is_profile_of(base: MessageOrDescriptorBase,
                  test: MessageOrDescriptorBase) -> bool:
  """Returns True if test is a profile of base.

  Args:
    base: A protobuf Message or DescriptorBase.
    test: A protobuf Message or DescriptorBase to validate against base.

  Returns:
    True if test is a profile of base, otherwise False.

  Raises:
    ValueError: Unable to retrieve options for type: <type>.
  """
  base_structure_def_url: Optional[str] = get_structure_definition_url(base)
  if base_structure_def_url is None:
    return False

  fhir_profile_base: Optional[List[str]] = get_value_for_annotation_extension(
      test, annotations_pb2.fhir_profile_base)
  if fhir_profile_base is None:
    return False

  return base_structure_def_url in fhir_profile_base


def get_fhir_version(
    message_or_descriptor: Union[message.Message, descriptor.Descriptor,
                                 descriptor.FileDescriptor]
) -> Optional[str]:
  """Returns the version of FHIR protos used in the corresponding file.

  Args:
    message_or_descriptor: A protobuf Message, Descriptor, or FileDescriptor to
      examine.

  Returns:
    The version of FHIR protos used in the corresponding file (e.g., STU3, R4),
    or None if the extension does not exist.

  Raises:
    ValueError: Unable to retrieve FhirVersion for type: <type>.
  """
  if isinstance(message_or_descriptor, message.Message):
    file_descriptor = message_or_descriptor.DESCRIPTOR.file
  elif isinstance(message_or_descriptor, descriptor.Descriptor):
    file_descriptor = message_or_descriptor.file
  elif isinstance(message_or_descriptor, descriptor.FileDescriptor):
    file_descriptor = message_or_descriptor
  else:
    raise ValueError('Cannot retrieve file_descriptor for type: {}'.format(
        type(message_or_descriptor)))
  return get_value_for_annotation_extension(file_descriptor,
                                            annotations_pb2.fhir_version)


def get_value_for_annotation_extension(
    message_or_descriptor: MessageOrDescriptorBase,
    extension_field: descriptor.FieldDescriptor) -> Optional[Any]:
  """Returns the value associated with the annotation extension field.

  Args:
    message_or_descriptor: A protobuf Message or DescriptorBase to examine.
    extension_field: A FieldDescriptor describing the annotation whose value to
      retrieve.

  Returns:
    The value associated with extension_field, if one exists. Otherwise returns
    None.

  Raises:
    ValueError: Unable to retrieve Options for type: <type>.
  """
  options = get_options(message_or_descriptor)
  if proto_utils.field_is_repeated(extension_field):
    return options.Extensions[extension_field]

  if options.HasExtension(extension_field):
    return options.Extensions[extension_field]
  return None


def get_options(
    message_or_descriptor: MessageOrDescriptorBase) -> DescriptorOptions:
  """Returns the underlying message options for message_or_descriptor.

  Args:
    message_or_descriptor: A protobuf Message or DescriptorBase to examine.

  Returns:
    The ProtoOptions for the given Message or DescriptorBase. The type of
    Descriptor, if provided, will determine the options returned. All options
    are derived from message.Message.

  Raises:
    ValueError: Unable to retrieve options for type: <type>.
  """
  if isinstance(message_or_descriptor, message.Message):
    return message_or_descriptor.DESCRIPTOR.GetOptions()
  return message_or_descriptor.GetOptions()
