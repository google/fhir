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
"""Performs basic FHIR constraint and resource validation."""

from google.protobuf import any_pb2
from google.protobuf import descriptor
from google.protobuf import message
from proto.google.fhir.proto import annotations_pb2
from google.fhir import _primitive_time_utils
from google.fhir import fhir_errors
from google.fhir import primitive_handler
from google.fhir.utils import annotation_utils
from google.fhir.utils import fhir_types
from google.fhir.utils import proto_utils


def _validate_reference_field(parent: message.Message,
                              field: descriptor.FieldDescriptor):
  """Ensure that the provided reference field is valid.

  Args:
    parent: The containing Message.
    field: The reference field descriptor.

  Raises:
    fhir_errors.InvalidFhirError: In the event of an empty reference (no
    extensions, no identifier, no display).
  """
  oneof = field.message_type.oneofs[0]

  # Singular fields have a length of 1
  for i in range(proto_utils.field_content_length(parent, field)):
    reference = proto_utils.get_value_at_field_index(parent, field, i)
    reference_field_name = reference.WhichOneof(oneof.name)

    if reference_field_name is None:
      if not (reference.extension or reference.HasField('identifier') or
              reference.HasField('display')):
        raise fhir_errors.InvalidFhirError(
            f'`{reference.DESCRIPTOR.name}` is an empty reference.')
      # There's no reference field, but there is other data. This is valid.
      return

    field_options = field.GetOptions()
    if not field_options.Extensions[annotations_pb2.valid_reference_type]:
      # The reference field does not have restrictions, so any value is fine.
      return

    if reference.HasField('uri') or reference.HasField('fragment'):
      # Uri and Fragment references are untyped.
      return

    # There are no reference annotations for DSTU2; skip validation
    if annotation_utils.get_fhir_version(
        reference) == annotations_pb2.FhirVersion.DSTU2:
      return

    reference_field = reference.DESCRIPTOR.fields_by_name[reference_field_name]
    if annotation_utils.is_typed_reference_field(reference_field):
      # Ensure that the reference type is listed as "valid"
      reference_type = reference_field.GetOptions().Extensions[
          annotations_pb2.referenced_fhir_type]
      is_allowed = False
      for valid_type in field_options.Extensions[
          annotations_pb2.valid_reference_type]:
        if valid_type == reference_type or valid_type == 'Resource':
          is_allowed = True
          break

      if not is_allowed:
        raise fhir_errors.InvalidFhirError(
            f'Message `{parent.DESCRIPTOR.full_name}` contains an invalid '
            f'reference type: `{reference_type}` set at: '
            f'`{reference_field_name}`.')


def _validate_period(period: message.Message, base_name: str):
  """Validates that a timelike period has a valid start and end value."""
  if not (proto_utils.field_is_set(period, 'start') and
          proto_utils.field_is_set(period, 'end')):
    return  # Early exit; either start or end field is not present

  # Check whether start time is greater than end time. Note that, if it is,
  # that's not necessarily invalid, since the precisions can be different. So we
  # need to compare the end time at the upper bound of the end element.
  #
  # Example: If the start time is "Tuesday at noon", and the end time is "some
  # time Tuesday", this is valid even though the timestamp used for "some time
  # Tuesday" is Tuesday 00:00, since the precision for the start is higher than
  # the end.
  #
  # Also note the GetUpperBoundFromTimelikeElement is always greater than the
  # time itself by exactly one time unit, and hence start needs to be strictly
  # less than the upper bound of end, so as not to allow ranges like [Tuesday,
  # Monday] to be valid.
  start: message.Message = proto_utils.get_value_at_field(period, 'start')
  end: message.Message = proto_utils.get_value_at_field(period, 'end')
  end_precision = proto_utils.get_value_at_field(end, 'precision')

  start_dt_value = _primitive_time_utils.get_date_time_value(start)
  end_upper_bound = _primitive_time_utils.get_upper_bound(
      end, _primitive_time_utils.DateTimePrecision(end_precision))
  if start_dt_value >= end_upper_bound:
    raise fhir_errors.InvalidFhirError(
        f'`{base_name}` start time is later than end time.')


def _validate_fhir_constraints(
    msg: message.Message, base_name: str,
    primitive_handler_: primitive_handler.PrimitiveHandler):
  """Iterates over fields of the provided message and validates constraints.

  Args:
    msg: The message to validate.
    base_name: The root message name for recursive validation of nested message
      fields.
    primitive_handler_: Responsible for returning PrimitiveWrappers.

  Raises:
    fhir_errors.InvalidFhirError: In the event that a field is found to be
    violating FHIR constraints or a required oneof is not set.
  """
  if annotation_utils.is_primitive_type(msg):
    # Validation is implicitly done on the primitive type during wrapping
    _ = primitive_handler_.primitive_wrapper_from_primitive(msg)
    return

  if proto_utils.is_message_type(msg, any_pb2.Any):
    # We do not validate "Any" constrained resources.
    # TODO: Potentially unpack the correct type and validate?
    return

  # Enumerate and validate fields of the message
  for field in msg.DESCRIPTOR.fields:
    field_name = f'{base_name}.{field.json_name}'
    _validate_field(msg, field, field_name, primitive_handler_)

  # Also verify that oneof fields are set. Note that optional choice-types
  # should have the containing message unset - if the containing message is set,
  # it should have a value set as well
  for oneof in msg.DESCRIPTOR.oneofs:
    if (msg.WhichOneof(oneof.name) is None and
        not oneof.GetOptions().HasExtension(
            annotations_pb2.fhir_oneof_is_optional)):
      raise fhir_errors.InvalidFhirError(f'Empty oneof: `{oneof.full_name}`.')


def _validate_field(msg: message.Message, field: descriptor.FieldDescriptor,
                    field_name: str,
                    primitive_handler_: primitive_handler.PrimitiveHandler):
  """Validates that required fields are set, and performs basic temporal checks.

  Args:
    msg: The Message that the field belongs to.
    field: The FieldDescriptor of the field to examine.
    field_name: The name of the field.
    primitive_handler_: Responsible for returning PrimitiveWrappers.

  Raises:
    fhir_errors.InvalidFhirError: In the event that a required field is not set
    or if temporal requirements are not met.
  """
  if annotation_utils.field_is_required(field) and not proto_utils.field_is_set(
      msg, field):
    raise fhir_errors.InvalidFhirError(
        f'Required field `{field.full_name}` is missing.')

  if annotation_utils.is_reference(field.message_type):
    _validate_reference_field(msg, field)
    return

  if field.type == descriptor.FieldDescriptor.TYPE_MESSAGE:
    # Returns the first value only for a singular field
    for i in range(proto_utils.field_content_length(msg, field)):
      submessage = proto_utils.get_value_at_field_index(msg, field, i)
      _validate_fhir_constraints(submessage, field_name, primitive_handler_)

      # Run extra validation for some types, until FHIRPath validation covers
      # these as well
      if fhir_types.is_period(submessage):
        _validate_period(submessage, field_name)


def validate_resource(resource: message.Message,
                      primitive_handler_: primitive_handler.PrimitiveHandler):
  """Performs basic FHIR constraint validation on the provided resource.

  This API works for all supported versions of FHIR, but requires a primitive
  handler to be passed as an argument.
  If the FHIR version being used is known ahead of time, version-specific APIs
  such as `google.fhir.r4 resource_validation` should be used instead.

  Args:
    resource: the resource proto to validate
    primitive_handler_: Version-specific logic
  """
  _validate_fhir_constraints(resource, resource.DESCRIPTOR.name,
                             primitive_handler_)
