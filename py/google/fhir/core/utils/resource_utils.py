#
# Copyright 2021 Google LLC
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
"""A collection of functions for dealing with FHIR resources."""

from typing import Any, List, Type, TypeVar, cast

from google.protobuf import message
from google.fhir.core.utils import fhir_types
from google.fhir.core.utils import path_utils
from google.fhir.core.utils import proto_utils

_T = TypeVar('_T', bound=message.Message)


def extract_resources_from_bundle(bundle: message.Message, *,
                                  resource_type: Type[_T]) -> List[_T]:
  """Returns a list of resources of type `resource_type` from `bundle`.

  Args:
    bundle: The FHIR Bundle to examine.
    resource_type: The message type of the resource to return.

  Returns:
    A list of resources of type `resource_type` belonging to the bundle.

  Raises:
    TypeError: In the event that `bundle` is not of type "Bundle".
    ValueError: In the event that a field corresponding to the "snake_case" name
    of `resource_type` does not exist on `Bundle.Entry`.
  """
  # TODO(b/148949073): Replace with templated/consolidated `fhir_types` call.
  if not fhir_types.is_type_or_profile_of(
      'http://hl7.org/fhir/StructureDefinition/Bundle', bundle):
    raise TypeError(
        f'{bundle.DESCRIPTOR.name} is not a type or profile of Bundle.')

  contained_resource_field = path_utils.camel_case_to_snake_case(
      resource_type.DESCRIPTOR.name)
  return [
      getattr(entry.resource, contained_resource_field)
      for entry in cast(Any, bundle).entry
      if entry.resource.HasField(contained_resource_field)
  ]


def get_contained_resource(
    contained_resource: message.Message) -> message.Message:
  """Returns the resource instance contained within `contained_resource`.

  Args:
    contained_resource: The containing `ContainedResource` instance.

  Returns:
    The resource contained by `contained_resource`.

  Raises:
    TypeError: In the event that `contained_resource` is not of type
    `ContainedResource`.
    ValueError: In the event that the oneof on `contained_resource` is not set.
  """
  # TODO(b/154059162): Use an annotation here.
  if contained_resource.DESCRIPTOR.name != 'ContainedResource':
    raise TypeError('Expected `ContainedResource` but got: '
                    f'{type(contained_resource)}.')
  oneof_field = contained_resource.WhichOneof('oneof_resource')
  if oneof_field is None:
    raise ValueError('`ContainedResource` oneof not set.')
  return proto_utils.get_value_at_field(contained_resource, oneof_field)
