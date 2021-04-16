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
"""PrimitiveWrapper subclass for Instant primitives."""

import datetime
from typing import Any, Callable, Dict, Tuple, Type, TypeVar

from google.protobuf import message
from google.fhir import _primitive_time_utils
from google.fhir import fhir_errors
from google.fhir.json_format.wrappers import _primitive_wrappers
from google.fhir.utils import proto_utils

_FORMAT_FUNCS: Dict[_primitive_time_utils.TimePrecision,
                    Callable[[datetime.datetime], str]] = {
                        _primitive_time_utils.TimePrecision.SECOND:
                            lambda dt: dt.isoformat(timespec='seconds'),
                        _primitive_time_utils.TimePrecision.MILLISECOND:
                            lambda dt: dt.isoformat(timespec='milliseconds'),
                        _primitive_time_utils.TimePrecision.MICROSECOND:
                            lambda dt: dt.isoformat(timespec='microseconds'),
                    }

Instant = TypeVar('Instant', bound=message.Message)


def _parse(json_str: str, primitive_cls: Type[Instant]) -> Instant:
  """Parses the json_str into an Instant FHIR primitive.

  Args:
    json_str: The raw JSON string to parse.
    primitive_cls: The FHIR primitive to parse into.

  Returns:
    A FHIR primitive Instant.

  Raises:
    fhir_errors.InvalidFhirError: In the event that no FHIR primitive Instant
    format was able to properly parse the json_str.
  """
  datetime_str, timezone_str = _primitive_time_utils.split_timezone(json_str)
  try:
    dt = datetime.datetime.strptime(datetime_str, '%Y-%m-%dT%H:%M:%S')
    return _primitive_time_utils.build_date_like(
        dt, timezone_str, _primitive_time_utils.TimePrecision.SECOND,
        primitive_cls)
  except ValueError:
    pass  # Fall through

  try:
    dt = datetime.datetime.strptime(datetime_str, '%Y-%m-%dT%H:%M:%S.%f')
    if (_primitive_time_utils.PRECISION_PATTERN_MILLISECOND.search(datetime_str)
        is not None):
      return _primitive_time_utils.build_date_like(
          dt, timezone_str, _primitive_time_utils.TimePrecision.MILLISECOND,
          primitive_cls)
    elif (
        _primitive_time_utils.PRECISION_PATTERN_MICROSECOND.search(datetime_str)
        is not None):
      return _primitive_time_utils.build_date_like(
          dt, timezone_str, _primitive_time_utils.TimePrecision.MICROSECOND,
          primitive_cls)
  except ValueError:
    pass  # Fall through

  raise fhir_errors.InvalidFhirError('Invalid Instant.')


class InstantWrapper(_primitive_wrappers.PrimitiveWrapper):
  """A wrapper around the Instant FHIR primitive type."""

  _PARSABLE_TYPES: Tuple[Type[Any], ...] = (str,)

  @classmethod
  def from_json_str(cls, json_str: str, primitive_cls: Type[Instant],
                    context: _primitive_wrappers.Context) -> 'InstantWrapper':
    """See PrimitiveWrapper.from_json_str."""
    _primitive_wrappers.validate_primitive_json_representation(
        primitive_cls.DESCRIPTOR, json_str)
    instant = _parse(json_str, primitive_cls)
    return cls(instant, context)

  def _nonnull_string_value(self) -> str:
    timezone: str = proto_utils.get_value_at_field(self.wrapped, 'timezone')
    if not timezone:
      raise fhir_errors.InvalidFhirError('Instant missing timezone.')

    precision: int = proto_utils.get_value_at_field(self.wrapped, 'precision')
    f = _FORMAT_FUNCS.get(precision)
    if f is None:
      raise fhir_errors.InvalidFhirError('Invalid precision on Instant.')
    value_us: int = proto_utils.get_value_at_field(self.wrapped, 'value_us')
    tzinfo = _primitive_time_utils.timezone_info_for_timezone(timezone)
    delta = datetime.timedelta(microseconds=value_us)
    datetime_value = (_primitive_time_utils.UNIX_EPOCH +
                      delta).astimezone(tzinfo)

    datetime_str = f(datetime_value)
    return _primitive_time_utils.restore_utc_timezone(datetime_str, timezone)
