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
"""Utilities for traversing and manipulating reference types."""

import re
from typing import Optional

from google.protobuf import descriptor
from google.protobuf import message
from google.fhir.utils import annotation_utils
from google.fhir.utils import path_utils
from google.fhir.utils import proto_utils

_FRAGMENT_REFERENCE_PATTERN = re.compile(r'^#[A-Za-z0-9.-]{1,64}$')
_INTERNAL_REFERENCE_PATTERN = re.compile(
    r'^(?P<resource_type>[0-9A-Za-z_]+)/(?P<resource_id>[A-Za-z0-9.-]{1,64})'
    r'(?:/_history/(?P<version>[A-Za-z0-9.-]{1,64}))?$')


def _validate_reference(reference: message.Message):
  """Raises a ValueError if the provided Message is not a FHIR reference."""
  if not annotation_utils.is_reference(reference):
    raise ValueError(
        f'Message {reference.DESCRIPTOR.name} is not a FHIR reference.')


def get_reference_id_field_for_resource(
    reference: message.Message,
    resource_type: str) -> descriptor.FieldDescriptor:
  """Returns the reference ID field for a provided resource type."""
  _validate_reference(reference)
  field_name = path_utils.camel_case_to_snake_case(resource_type) + '_id'
  field = reference.DESCRIPTOR.fields_by_name.get(field_name)
  if field is None:
    raise ValueError(f'Resource type {resource_type!r} is not valid for a '
                     f'reference. Field {field_name!r} does not exist.')
  return field


def populate_typed_reference_id(reference_id: message.Message, resource_id: str,
                                version: Optional[str]):
  """Sets the resource_id and optionally, version, on the reference."""
  reference_id_value_field = reference_id.DESCRIPTOR.fields_by_name['value']
  proto_utils.set_value_at_field(reference_id, reference_id_value_field,
                                 resource_id)
  if version is not None:
    history_field = reference_id.DESCRIPTOR.fields_by_name.get('history')
    if history_field is None:
      raise ValueError('Not a valid ReferenceId message: '
                       f"{reference_id.DESCRIPTOR.full_name}. Field 'history' "
                       'does not exist.')
    history = proto_utils.set_in_parent_or_add(reference_id, history_field)
    history_value_field = history.DESCRIPTOR.fields_by_name['value']
    proto_utils.set_value_at_field(history, history_value_field, version)


def split_if_relative_reference(reference: message.Message):
  """If possible, parses a `Reference` `uri` into more structured fields.

  This is only possible for two forms of reference uris:
  * Relative references of the form $TYPE/$ID, e.g., "Patient/1234"
    In this case, this will be parsed to a proto of the form:
    {patient_id: {value: "1234"}}
  * Fragments of the form "#$FRAGMENT", e.g., "#vs1".  In this case, this would
    be parsed into a proto of the form:
    {fragment: {value: "vs1"} }

  If the reference URI matches one of these schemas, the `uri` field will be
  cleared, and the appropriate structured fields set. Otherwise, the reference
  will be unchanged.

  Args:
    reference: The FHIR reference to potentially split.

  Raises:
    ValueError: If the message is not a valid FHIR Reference proto.
  """
  _validate_reference(reference)
  uri_field = reference.DESCRIPTOR.fields_by_name.get('uri')
  if not proto_utils.field_is_set(reference, uri_field):
    return  # No URI to split

  uri = proto_utils.get_value_at_field(reference, uri_field)

  internal_match = re.fullmatch(_INTERNAL_REFERENCE_PATTERN, uri.value)
  if internal_match is not None:
    # Note that we make the reference_id off of the reference before adding it,
    # since adding the reference_id would destroy the uri field, as they are
    # both in the same oneof. This allows us to copy fields from uri to
    # reference_id without making an extra copy.
    reference_id_field = get_reference_id_field_for_resource(
        reference, internal_match.group('resource_type'))
    reference_id = proto_utils.create_message_from_descriptor(
        reference_id_field.message_type)
    populate_typed_reference_id(reference_id,
                                internal_match.group('resource_id'),
                                internal_match.group('version'))

    proto_utils.copy_common_field(uri, reference_id, 'id')
    proto_utils.copy_common_field(uri, reference_id, 'extension')
    proto_utils.set_value_at_field(reference, reference_id_field, reference_id)
    return

  fragment_match = re.fullmatch(_FRAGMENT_REFERENCE_PATTERN, uri.value)
  if fragment_match is not None:
    # Note that we make the fragment off of the reference before adding it,
    # since adding the fragment would destroy the uri field, as they are both in
    # the same oneof. This allows us to copy fields from uri to fragment without
    # making an extra copy.
    fragment_field = reference.DESCRIPTOR.fields_by_name['fragment']
    fragment = proto_utils.create_message_from_descriptor(
        fragment_field.message_type)

    value_field = fragment.DESCRIPTOR.fields_by_name['value']
    proto_utils.set_value_at_field(fragment, value_field, uri.value[1:])
    proto_utils.copy_common_field(uri, fragment, 'id')
    proto_utils.copy_common_field(uri, fragment, 'extension')
    proto_utils.set_value_at_field(reference, fragment_field, fragment)
    return


def reference_to_string(reference: message.Message) -> str:
  """Returns a reference URI for a typed reference message."""
  _validate_reference(reference)

  # Early-exit if URI or fragment is set
  if proto_utils.field_is_set(reference, 'uri'):
    uri = proto_utils.get_value_at_field(reference, 'uri')
    return uri.value
  elif proto_utils.field_is_set(reference, 'fragment'):
    fragment = proto_utils.get_value_at_field(reference, 'fragment')
    return f'#{fragment.value}'

  set_oneof = reference.WhichOneof('reference')
  if set_oneof is None:
    raise ValueError(f'Reference is not set on: {reference.DESCRIPTOR.name}.')

  # Convert to CamelCase
  prefix = path_utils.snake_case_to_camel_case(set_oneof, upper=True)

  # Cull trailing 'Id'
  if prefix.endswith('Id'):
    prefix = prefix[:-2]

  reference_id = proto_utils.get_value_at_field(reference, set_oneof)
  reference_string = f'{prefix}/{reference_id.value}'
  if proto_utils.field_is_set(reference_id, 'history'):
    reference_string += f'/_history/{reference_id.history.value}'
  return reference_string
