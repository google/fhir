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
"""Convenience functions for examining message type."""
from proto.google.fhir.proto import annotations_pb2
from google.fhir.utils import annotation_utils

_CODE_STRUCTURE_DEFINITION_URL = 'http://hl7.org/fhir/StructureDefinition/code'
_CODING_STRUCTURE_DEFINITION_URL = 'http://hl7.org/fhir/StructureDefinition/Coding'
_DATETIME_STRUCTURE_DEFINITION_URL = 'http://hl7.org/fhir/StructureDefinition/dateTime'
_EXTENSION_STRUCTURE_DEFINITION_URL = 'http://hl7.org/fhir/StructureDefinition/Extension'
_PERIOD_STRUCTURE_DEFINITION_URL = 'http://hl7.org/fhir/StructureDefinition/Period'
_PATIENT_STRUCTURE_DEFINITION_URL = 'http://hl7.org/fhir/StructureDefinition/Patient'

# TODO: Look into templating/consolidating


def is_type_or_profile_of(
    url: str,
    message_or_descriptor: annotation_utils.MessageOrDescriptorBase) -> bool:
  """Whether message_or_descriptor is of type url *or* is a profile of url.

  Args:
    url: The FHIR structure definition URL to compare against.
    message_or_descriptor: The Message or Descriptor to examine.

  Returns:
    True if message_or_descriptor has a structure definition URL of url, or if
    it is a profile with a base structure definition URL of url.
  """
  return (is_type(url, message_or_descriptor) or
          is_profile_of(url, message_or_descriptor))


def is_profile_of(
    url: str,
    message_or_descriptor: annotation_utils.MessageOrDescriptorBase) -> bool:
  """Returns True if message_or_descriptor is a profile of url.

  Args:
    url: The FHIR structure definition URL to compare against.
    message_or_descriptor: The Message or Descriptor to examine.

  Returns:
    True if message_or_descriptor's fhir_profile_base extension list contains
    url.
  """
  options = annotation_utils.get_options(message_or_descriptor)
  return url in options.Extensions[annotations_pb2.fhir_profile_base]


def is_type(
    url: str,
    message_or_descriptor: annotation_utils.MessageOrDescriptorBase) -> bool:
  """Returns True if message_or_descriptor has a structure definition of url.

  Args:
    url: The FHIR structure definition URL to compare against.
    message_or_descriptor: The Message or Descriptor to examine.

  Returns:
    True if message_or_descriptor has a structure definition equal to url.
  """
  return (annotation_utils.get_structure_definition_url(message_or_descriptor)
          == url)


def is_code(
    message_or_descriptor: annotation_utils.MessageOrDescriptorBase) -> bool:
  """Returns True if message_or_descriptor is a FHIR Code."""
  return is_type(_CODE_STRUCTURE_DEFINITION_URL, message_or_descriptor)


def is_profile_of_code(
    message_or_descriptor: annotation_utils.MessageOrDescriptorBase) -> bool:
  """Returns True if message_or_descriptor is a profile of a FHIR Code."""
  # TODO: Remove valueset URL check once STU3 protos are upgraded
  return (is_profile_of(_CODE_STRUCTURE_DEFINITION_URL, message_or_descriptor)
          or annotation_utils.has_fhir_valueset_url(message_or_descriptor))


def is_type_or_profile_of_code(
    message_or_descriptor: annotation_utils.MessageOrDescriptorBase) -> bool:
  """Returns True if message_or_descriptor is a Code or is a profile of Code."""
  return is_code(message_or_descriptor) or is_profile_of_code(
      message_or_descriptor)


def is_coding(
    message_or_descriptor: annotation_utils.MessageOrDescriptorBase) -> bool:
  """Returns True if message_or_descriptor is a FHIR Coding type."""
  return is_type(_CODING_STRUCTURE_DEFINITION_URL, message_or_descriptor)


def is_profile_of_coding(
    message_or_descriptor: annotation_utils.MessageOrDescriptorBase) -> bool:
  """Returns True if message_or_descriptor is a profile of the Coding type."""
  return is_profile_of(_CODING_STRUCTURE_DEFINITION_URL, message_or_descriptor)


def is_type_or_profile_of_coding(
    message_or_descriptor: annotation_utils.MessageOrDescriptorBase) -> bool:
  """Returns True if message_or_descriptor is Coding/is a profile of Coding."""
  return is_type_or_profile_of(_CODING_STRUCTURE_DEFINITION_URL,
                               message_or_descriptor)


def is_extension(
    message_or_descriptor: annotation_utils.MessageOrDescriptorBase) -> bool:
  """Returns True if message_or_descriptor is a FHIR Extension."""
  return is_type(_EXTENSION_STRUCTURE_DEFINITION_URL, message_or_descriptor)


def is_profile_of_extension(
    message_or_descriptor: annotation_utils.MessageOrDescriptorBase) -> bool:
  """Returns True if message_or_descriptor is a profile of Extension."""
  return is_profile_of(_EXTENSION_STRUCTURE_DEFINITION_URL,
                       message_or_descriptor)


def is_type_or_profile_of_extension(
    message_or_descriptor: annotation_utils.MessageOrDescriptorBase) -> bool:
  """Returns True if message_or_descriptor is Extension/profile of Extension."""
  return is_type_or_profile_of(_EXTENSION_STRUCTURE_DEFINITION_URL,
                               message_or_descriptor)


def is_period(
    message_or_descriptor: annotation_utils.MessageOrDescriptorBase) -> bool:
  """Returns True if message_or_descriptor is a FHIR Period type."""
  return is_type(_PERIOD_STRUCTURE_DEFINITION_URL, message_or_descriptor)


def is_date_time(
    message_or_descriptor: annotation_utils.MessageOrDescriptorBase) -> bool:
  """Returns True if message_or_descriptor is a FHIR DateTime type."""
  return is_type(_DATETIME_STRUCTURE_DEFINITION_URL, message_or_descriptor)


def is_type_or_profile_of_patient(
    message_or_descriptor: annotation_utils.MessageOrDescriptorBase) -> bool:
  """Returns True if message_or_descriptor is type or a profile of Patient."""
  return is_type_or_profile_of(_PATIENT_STRUCTURE_DEFINITION_URL,
                               message_or_descriptor)
