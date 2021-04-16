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
"""PrimitiveWrapper subclass for DateTime primitives."""

import datetime
from typing import Any, Callable, Dict, Tuple, Type, TypeVar

from google.protobuf import message
from google.fhir import _primitive_time_utils
from google.fhir import fhir_errors
from google.fhir.json_format.wrappers import _primitive_wrappers
from google.fhir.utils import proto_utils

_FORMAT_FUNCS: Dict[_primitive_time_utils.DateTimePrecision,
                    Callable[[datetime.datetime], str]] = {
                        _primitive_time_utils.DateTimePrecision.YEAR:
                            lambda dt: dt.strftime('%Y'),
                        _primitive_time_utils.DateTimePrecision.MONTH:
                            lambda dt: dt.strftime('%Y-%m'),
                        _primitive_time_utils.DateTimePrecision.DAY:
                            lambda dt: dt.strftime('%Y-%m-%d'),
                        _primitive_time_utils.DateTimePrecision.SECOND:
                            lambda dt: dt.isoformat(timespec='seconds'),
                        _primitive_time_utils.DateTimePrecision.MILLISECOND:
                            lambda dt: dt.isoformat(timespec='milliseconds'),
                        _primitive_time_utils.DateTimePrecision.MICROSECOND:
                            lambda dt: dt.isoformat(timespec='microseconds'),
                    }

DateTime = TypeVar('DateTime', bound=message.Message)


def _parse(json_str: str, primitive_cls: Type[DateTime], *,
           default_timezone: str) -> DateTime:
  """Parses the json_str into a DateTime FHIR primitive.

  Args:
    json_str: The raw JSON string to parse.
    primitive_cls: The FHIR primitive to parse into.
    default_timezone: The default timezone to use when parsing in the event that
      no timezone information is present.

  Returns:
    A FHIR primitive DateTime.

  Raises:
    fhir_errors.InvalidFhirError: In the event that no FHIR primitive DateTime
    format was able to properly parse the json_str.
  """
  try:
    dt = datetime.datetime.strptime(json_str, '%Y')
    return _primitive_time_utils.build_date_like(
        dt, default_timezone, _primitive_time_utils.DateTimePrecision.YEAR,
        primitive_cls)
  except ValueError:
    pass  # Fall through

  try:
    dt = datetime.datetime.strptime(json_str, '%Y-%m')
    return _primitive_time_utils.build_date_like(
        dt, default_timezone, _primitive_time_utils.DateTimePrecision.MONTH,
        primitive_cls)
  except ValueError:
    pass  # Fall through

  try:
    dt = datetime.datetime.strptime(json_str, '%Y-%m-%d')
    return _primitive_time_utils.build_date_like(
        dt, default_timezone, _primitive_time_utils.DateTimePrecision.DAY,
        primitive_cls)
  except ValueError:
    pass  # Fall through

  # Attempt to parse DateTime with provided time and timezone offset...
  datetime_str, timezone_str = _primitive_time_utils.split_timezone(json_str)
  try:
    dt = datetime.datetime.strptime(datetime_str, '%Y-%m-%dT%H:%M:%S')
    return _primitive_time_utils.build_date_like(
        dt, timezone_str, _primitive_time_utils.DateTimePrecision.SECOND,
        primitive_cls)
  except ValueError:
    pass  # Fall through

  try:
    dt = datetime.datetime.strptime(datetime_str, '%Y-%m-%dT%H:%M:%S.%f')
    if (_primitive_time_utils.PRECISION_PATTERN_MILLISECOND.search(datetime_str)
        is not None):
      return _primitive_time_utils.build_date_like(
          dt, timezone_str, _primitive_time_utils.DateTimePrecision.MILLISECOND,
          primitive_cls)
    elif (
        _primitive_time_utils.PRECISION_PATTERN_MICROSECOND.search(datetime_str)
        is not None):
      return _primitive_time_utils.build_date_like(
          dt, timezone_str, _primitive_time_utils.DateTimePrecision.MICROSECOND,
          primitive_cls)
  except ValueError:
    pass  # Fall through

  raise fhir_errors.InvalidFhirError('Invalid DateTime.')


class DateTimeWrapper(_primitive_wrappers.PrimitiveWrapper):
  """A wrapper around the DateTime FHIR primitive type."""

  _PARSABLE_TYPES: Tuple[Type[Any], ...] = (str,)

  @classmethod
  def from_json_str(cls, json_str: str, primitive_cls: Type[message.Message],
                    context: _primitive_wrappers.Context) -> 'DateTimeWrapper':
    """See _primitive_wrappers.PrimitiveWrapper.from_json_str."""
    _primitive_wrappers.validate_primitive_json_representation(
        primitive_cls.DESCRIPTOR, json_str)
    date = _parse(
        json_str, primitive_cls, default_timezone=context.default_timezone)
    return cls(date, context)

  def _nonnull_string_value(self) -> str:
    timezone: str = proto_utils.get_value_at_field(self.wrapped, 'timezone')
    if not timezone:
      raise fhir_errors.InvalidFhirError('DateTime missing timezone.')

    precision: int = proto_utils.get_value_at_field(self.wrapped, 'precision')
    f = _FORMAT_FUNCS.get(precision)
    if f is None:
      raise fhir_errors.InvalidFhirError('Invalid Precision on DateTime')

    dt_str = f(_primitive_time_utils.get_date_time_value(self.wrapped))
    return _primitive_time_utils.restore_utc_timezone(dt_str, timezone)
